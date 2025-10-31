// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * TAS55x8 ASoC codec driver
 *
 * Copyright (c) 2026 Li Ming <25062566@qq.com>
 *
 * TODO:
 *  - implement DAPM and input muxing
 *  - implement modulation limit
 *  - implement non-default PWM start
 *
 * Note that this chip has a very unusual register layout, specifically
 * because the registers are of unequal size, and multi-byte registers
 * require bulk writes to take effect. Regmap does not support that kind
 * of devices.
 *
 * Currently, the driver does not touch any of the registers >= 0x20, so
 * it doesn't matter because the entire map can be accessed as 8-bit
 * array. In case more features will be added in the future
 * that require access to higher registers, the entire regmap H/W I/O
 * routines have to be open-coded.
 */


#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/tas55x8.h>


/*
 * Default tas55x8 power-up configuration
 */
static const struct reg_default tas55x8_reg_defaults[] = {
    {0x04, (0x03 & ~0x10)}, /*disable DAP automute */
    {0x14, 0x0f}, /*0x00: Set input automute threshold less than -90dBFS
                    0x0F: Set input automute and output automute delay to 178.8 m. */
    {0x15, 0xf7}, /*0xF0: Set PWM automute threshold -42dB below input automute threshold.
                    0x07: Set back-end reset period 800 ms. */
}

struct reg_value{
	unsigned uint8_t value[20];
}

static int tas55x8_register_size(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CLOCK_CONTROL_REG ... DELAY_CH8_BD_MODE_REG:
		return 1;
	case BANK_SWITCHING_CMD_REG ... CH8_BQ2_REG :
	case LOUDNESS_LOG2_GAIN_REG:
	case LOUDNESS_GAIN_REG ... LOUDNESS_OFFSET_REG:
	case DRC1_CONTROL_REG ... DRC2_CONTROL_REG:
	case ENERGY_MANAGER_WEIGHTING_CH1_REG ... ENERGY_MANAGER_LOW_THRESHOLD_SUBWOOFER_REG:
	case ASRC_STATUS_REG  ... ASRC_MODE_CONTROL_REG:
	case AUTO_MUTE_BEHAVIOUR:
	case VOLUME_TREBLE_BASS_SLEW_RATES_REG ... GENERAL_CONTROL_REG:
	case R_DOLBY_COEFLR_REG ... THD_MANAGER_POST_REG:
	case KHZ192_IMAGE_SELECT_REG:
		return 4;
	case DRC1_ENERGY_REG ... DRC1_THRESHOLD_REG:
	case DRC1_OFFSET_REG:
	case DRC2_ENERGY_REG ... DRC2_THRESHOLD_REG:
	case DRC2_OFFSET_REG:
	case RC_BYPASS1_REG ... OUTPUT_TO_PWM6_REG:
	case SDIN5_INPUT1_MIX_REG ... SDIN5_INPUT8_MIX_REG:
		return 8;
	case DRC1_SLOPE_REG:
	case DRC2_SLOPE_REG:
	case OUTPUT_TO_PWM7_REG ... OUTPUT_TO_PWM8_REG:
		return 12;
	case DRC1_ATTACK_DECAY_REG:
	case DRC2_ATTACK_DECAY_REG:
	case ENERGY_MANAGER_AVERAGING_REG:
	case KHZ192_PROCESS_FLOW_OUTPUT_MIX1_REG ... KHZ192_PROCESS_FLOW_OUTPUT_MIX4_REG:
	case KHZ192_DOLBY_DOWNMIX_COEF1_REG ... KHZ192_DOLBY_DOWNMIX_COEF2_REG:
		return 16;
	case PSVC_VOLUME_BIQUAD:
		return 20;
	}

	dev_err(dev, "Unsupported register address: %d\n", reg);
	return 0;
}

static bool tas55x8_accessible_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case 0xbf:
	case 0xc0 ... 0xc2:
	case 0xc6 ... 0xcb:
	case 0xcd ... 0xce:
	case 0xe1 ... 0xe2:
	case 0xeb:
	case 0xf8 ... 0xf9:
	case 0xfd:
	case 0xff:
		return false;
	default:
		return true;
	}
}

static bool tas55x8_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case GENERAL_STATUS_REG:
	case ERROR_STATUS_REG:
		return true;
	}

	return false;
}

static bool tas55x8_writeable_reg(struct device *dev, unsigned int reg)
{
	return tas55x8_accessible_reg(dev, reg) && (reg != GENERAL_STATUS_REG);
}

static int tas55x8_reg_write(void *context, unsigned int reg,
			      unsigned int value)
{
	struct i2c_client *client = context;
	unsigned int i, size;
	int ret;

	size = tas55x8_register_size(&client->dev, reg);

	uint8_t buf[(size+1)];

	if (size == 0)
		return -EINVAL;

	buf[0] = reg;

	for (i = size; i >= 1; --i) {
		buf[i] = value;
		value >>= 8;
	}

	ret = i2c_master_send(client, buf, size + 1);
	if (ret == size + 1)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

static int tas55x8_reg_read(void *context, unsigned int reg,
			     unsigned int *value)
{
	struct i2c_client *client = context;
	struct i2c_msg msgs[2];
	unsigned int size;
	unsigned int i;
	int ret;

	size = tas55x8_register_size(&client->dev, reg);

	uint8_t send_buf, recv_buf[size];

	if (size == 0)
		return -EINVAL;

	send_buf = reg;

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = &send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = size;
	msgs[1].buf = recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
		return ret;
	else if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*value = 0;

	for (i = 0; i < size; i++) {
		*value <<= 8;
		*value |= recv_buf[i];
	}

	return 0;
}

static const char * const supply_names[] = {
	"dvdd", "avdd"
};

struct tas55x8_private {
	//struct regmap	*regmap;
	struct reg_value *regvalue[255];
	unsigned int	mclk, sclk;
	unsigned int	format;
	bool		deemph;
	unsigned int	charge_period;
	unsigned int	pwm_start_mid_z;
	/* Current sample rate for de-emphasis control */
	int		rate;
	/* GPIO driving Reset pin, if any */
	struct gpio_desc *reset;
	struct		regulator_bulk_data supplies[ARRAY_SIZE(supply_names)];
};

static int tas55x8_deemph[] = { 0, 32000, 44100, 48000, 88200, 96000, 176400, 192000};

static int tas55x8_set_deemph(struct snd_soc_component *component)
{
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);
	int i, val = 0;

	if (priv->deemph) {
		for (i = 0; i < ARRAY_SIZE(tas55x8_deemph); i++) {
			if (tas55x8_deemph[i] == priv->rate) {
				val = i;
				break;
			}
		}
	}

	return regmap_update_bits(priv->regmap, tas55x8_SYS_CONTROL_1,
				  tas55x8_DEEMPH_MASK, val);
}

static int tas55x8_get_deemph(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = priv->deemph;

	return 0;
}

static int tas55x8_put_deemph(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);

	priv->deemph = ucontrol->value.integer.value[0];

	return tas55x8_set_deemph(component);
}


static int tas55x8_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_component *component = codec_dai->component;
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);

	switch (clk_id) {
	case tas55x8_CLK_IDX_MCLK: // tas55x8_CLK_IDX_MCLK = 0
		priv->mclk = freq;
		break;
	case tas55x8_CLK_IDX_SCLK: //tas55x8_CLK_IDX_SCLK = 1
		priv->sclk = freq;
		break;
	}

	return 0;
}

static int tas55x8_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int format)
{
	struct snd_soc_component *component = codec_dai->component;
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);

	/* The tas55x8 can only be slave to all clocks */
	if ((format & SND_SOC_DAIFMT_CLOCK_PROVIDER_MASK) != SND_SOC_DAIFMT_CBC_CFC) {
		dev_err(component->dev, "Invalid clocking mode\n");
		return -EINVAL;
	}

	/* we need to refer to the data format from hw_params() */
	priv->format = format;

	return 0;
}

static const int tas55x8_sample_rates[] = {
	32000, 38000, 44100, 48000, 88200, 96000, 176400, 192000
};

static const int tas55x8_ratios[] = {
	64, 128, 192, 256, 384, 512
};

static int index_in_array(const int *array, int len, int needle)
{
	int i;

	for (i = 0; i < len; i++)
		if (array[i] == needle)
			return i;

	return -ENOENT;
}

static int tas55x8_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);
	int val;
	int ret;

	priv->rate = params_rate(params);

	/* Look up the sample rate and refer to the offset in the list */
	val = index_in_array(tas55x8_sample_rates,
			     ARRAY_SIZE(tas55x8_sample_rates), priv->rate);

	if (val < 0) {
		dev_err(component->dev, "Invalid sample rate\n");
		return -EINVAL;
	}

	ret = regmap_update_bits(priv->regmap, tas55x8_CLOCK_CONTROL,
				 tas55x8_CLOCK_RATE_MASK,
				 tas55x8_CLOCK_RATE(val));
	if (ret < 0)
		return ret;

	/* MCLK / Fs ratio */
	val = index_in_array(tas55x8_ratios, ARRAY_SIZE(tas55x8_ratios),
			     priv->mclk / priv->rate);
	if (val < 0) {
		dev_err(component->dev, "Invalid MCLK / Fs ratio\n");
		return -EINVAL;
	}

	ret = regmap_update_bits(priv->regmap, tas55x8_CLOCK_CONTROL,
				 tas55x8_CLOCK_RATIO_MASK,
				 tas55x8_CLOCK_RATIO(val));
	if (ret < 0)
		return ret;


	ret = regmap_update_bits(priv->regmap, tas55x8_CLOCK_CONTROL,
				 tas55x8_CLOCK_SCLK_RATIO_48,
				 (priv->sclk == 48 * priv->rate) ? 
					tas55x8_CLOCK_SCLK_RATIO_48 : 0);
	if (ret < 0)
		return ret;

	/*
	 * The chip has a very unituitive register mapping and muxes information
	 * about data format and sample depth into the same register, but not on
	 * a logical bit-boundary. Hence, we have to refer to the format passed
	 * in the set_dai_fmt() callback and set up everything from here.
	 *
	 * First, determine the 'base' value, using the format ...
	 */
	switch (priv->format & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_RIGHT_J:
		val = 0x00;
		break;
	case SND_SOC_DAIFMT_I2S:
		val = 0x03;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		val = 0x06;
		break;
	default:
		dev_err(component->dev, "Invalid DAI format\n");
		return -EINVAL;
	}

	/* ... then add the offset for the sample bit depth. */
	switch (params_width(params)) {
    case 16:
		val += 0;
        break;
	case 20:
		val += 1;
		break;
	case 24:
		val += 2;
		break;
	default:
		dev_err(component->dev, "Invalid bit width\n");
		return -EINVAL;
	}

	ret = regmap_write(priv->regmap, tas55x8_SERIAL_DATA_IF, val);
	if (ret < 0)
		return ret;

	/* clock is considered valid now */
	ret = regmap_update_bits(priv->regmap, tas55x8_CLOCK_CONTROL,
				 tas55x8_CLOCK_VALID, tas55x8_CLOCK_VALID);
	if (ret < 0)
		return ret;

	return tas55x8_set_deemph(component);
}

static int tas55x8_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_component *component = dai->component;
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);
	unsigned int val = 0;

	if (mute)
		val = tas55x8_SOFT_MUTE_ALL;

	return regmap_write(priv->regmap, tas55x8_SOFT_MUTE, val);
}

static void tas55x8_reset(struct tas55x8_private *priv)
{
	if (priv->reset) {
		/* Reset codec - minimum assertion time is 400ns */
		gpiod_set_value_cansleep(priv->reset, 1);
		udelay(1);
		gpiod_set_value_cansleep(priv->reset, 0);

		/* Codec needs ~15ms to wake up */
		msleep(15);
	}
}

/* charge period values in microseconds */
static const int tas55x8_charge_period[] = {
	  13000,  16900,   23400,   31200,   41600,   54600,   72800,   96200,
	 130000, 156000,  234000,  312000,  416000,  546000,  728000,  962000,
	1300000, 169000, 2340000, 3120000, 4160000, 5460000, 7280000, 9620000,
};

static int tas55x8_init(struct device *dev, struct tas55x8_private *priv)
{
	int ret, i;

	/*
	 * If any of the channels is configured to start in Mid-Z mode,
	 * configure 'part 1' of the PWM starts to use Mid-Z, and tell
	 * all configured mid-z channels to start under 'part 1'.
	 */
	if (priv->pwm_start_mid_z)
		regmap_write(priv->regmap, tas55x8_PWM_START,
			     tas55x8_PWM_START_MIDZ_FOR_START_1 |
				priv->pwm_start_mid_z);

	/* lookup and set split-capacitor charge period */
	if (priv->charge_period == 0) {
		regmap_write(priv->regmap, tas55x8_SPLIT_CAP_CHARGE, 0);
	} else {
		i = index_in_array(tas55x8_charge_period,
				   ARRAY_SIZE(tas55x8_charge_period),
				   priv->charge_period);
		if (i >= 0)
			regmap_write(priv->regmap, tas55x8_SPLIT_CAP_CHARGE,
				     i + 0x08);
		else
			dev_warn(dev,
				 "Invalid split-cap charge period of %d ns.\n",
				 priv->charge_period);
	}

	/* enable factory trim */
	ret = regmap_write(priv->regmap, tas55x8_OSC_TRIM, 0x00);
	if (ret < 0)
		return ret;

	/* start all channels */
	ret = regmap_write(priv->regmap, tas55x8_SYS_CONTROL_2, 0x20);
	if (ret < 0)
		return ret;

	/* mute all channels for now */
	ret = regmap_write(priv->regmap, tas55x8_SOFT_MUTE,
			   tas55x8_SOFT_MUTE_ALL);
	if (ret < 0)
		return ret;

	return 0;
}

/* tas55x8 controls */
static const DECLARE_TLV_DB_SCALE(tas55x8_dac_tlv, -10350, 50, 1);

static const struct snd_kcontrol_new tas55x8_controls[] = {
	SOC_SINGLE_TLV("Master Playback Volume", tas55x8_MASTER_VOL,
		       0, 0xff, 1, tas55x8_dac_tlv),
	SOC_DOUBLE_R_TLV("Channel 1/2 Playback Volume",
			 tas55x8_CHANNEL_VOL(0), tas55x8_CHANNEL_VOL(1),
			 0, 0xff, 1, tas55x8_dac_tlv),
	SOC_DOUBLE_R_TLV("Channel 3/4 Playback Volume",
			 tas55x8_CHANNEL_VOL(2), tas55x8_CHANNEL_VOL(3),
			 0, 0xff, 1, tas55x8_dac_tlv),
	SOC_DOUBLE_R_TLV("Channel 5/6 Playback Volume",
			 tas55x8_CHANNEL_VOL(4), tas55x8_CHANNEL_VOL(5),
			 0, 0xff, 1, tas55x8_dac_tlv),
	SOC_SINGLE_BOOL_EXT("De-emphasis Switch", 0,
			    tas55x8_get_deemph, tas55x8_put_deemph),
};

/* Input mux controls */
static const char *tas55x8_dapm_sdin_texts[] =
{
	"SDIN1-L", "SDIN1-R", "SDIN2-L", "SDIN2-R",
	"SDIN3-L", "SDIN3-R", "Ground (0)", "nc"
};

static const struct soc_enum tas55x8_dapm_input_mux_enum[] = {
	SOC_ENUM_SINGLE(tas55x8_INPUT_MUX, 20, 8, tas55x8_dapm_sdin_texts),
	SOC_ENUM_SINGLE(tas55x8_INPUT_MUX, 16, 8, tas55x8_dapm_sdin_texts),
	SOC_ENUM_SINGLE(tas55x8_INPUT_MUX, 12, 8, tas55x8_dapm_sdin_texts),
	SOC_ENUM_SINGLE(tas55x8_INPUT_MUX, 8,  8, tas55x8_dapm_sdin_texts),
	SOC_ENUM_SINGLE(tas55x8_INPUT_MUX, 4,  8, tas55x8_dapm_sdin_texts),
	SOC_ENUM_SINGLE(tas55x8_INPUT_MUX, 0,  8, tas55x8_dapm_sdin_texts),
};

static const struct snd_kcontrol_new tas55x8_dapm_input_mux_controls[] = {
	SOC_DAPM_ENUM("Channel 1 input", tas55x8_dapm_input_mux_enum[0]),
	SOC_DAPM_ENUM("Channel 2 input", tas55x8_dapm_input_mux_enum[1]),
	SOC_DAPM_ENUM("Channel 3 input", tas55x8_dapm_input_mux_enum[2]),
	SOC_DAPM_ENUM("Channel 4 input", tas55x8_dapm_input_mux_enum[3]),
	SOC_DAPM_ENUM("Channel 5 input", tas55x8_dapm_input_mux_enum[4]),
	SOC_DAPM_ENUM("Channel 6 input", tas55x8_dapm_input_mux_enum[5]),
};

/* Output mux controls */
static const char *tas55x8_dapm_channel_texts[] =
	{ "Channel 1 Mux", "Channel 2 Mux", "Channel 3 Mux",
	  "Channel 4 Mux", "Channel 5 Mux", "Channel 6 Mux" };

static const struct soc_enum tas55x8_dapm_output_mux_enum[] = {
	SOC_ENUM_SINGLE(tas55x8_PWM_OUTPUT_MUX, 20, 6, tas55x8_dapm_channel_texts),
	SOC_ENUM_SINGLE(tas55x8_PWM_OUTPUT_MUX, 16, 6, tas55x8_dapm_channel_texts),
	SOC_ENUM_SINGLE(tas55x8_PWM_OUTPUT_MUX, 12, 6, tas55x8_dapm_channel_texts),
	SOC_ENUM_SINGLE(tas55x8_PWM_OUTPUT_MUX, 8,  6, tas55x8_dapm_channel_texts),
	SOC_ENUM_SINGLE(tas55x8_PWM_OUTPUT_MUX, 4,  6, tas55x8_dapm_channel_texts),
	SOC_ENUM_SINGLE(tas55x8_PWM_OUTPUT_MUX, 0,  6, tas55x8_dapm_channel_texts),
};

static const struct snd_kcontrol_new tas55x8_dapm_output_mux_controls[] = {
	SOC_DAPM_ENUM("PWM1 Output", tas55x8_dapm_output_mux_enum[0]),
	SOC_DAPM_ENUM("PWM2 Output", tas55x8_dapm_output_mux_enum[1]),
	SOC_DAPM_ENUM("PWM3 Output", tas55x8_dapm_output_mux_enum[2]),
	SOC_DAPM_ENUM("PWM4 Output", tas55x8_dapm_output_mux_enum[3]),
	SOC_DAPM_ENUM("PWM5 Output", tas55x8_dapm_output_mux_enum[4]),
	SOC_DAPM_ENUM("PWM6 Output", tas55x8_dapm_output_mux_enum[5]),
};

static const struct snd_soc_dapm_widget tas55x8_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("SDIN1-L"),
	SND_SOC_DAPM_INPUT("SDIN1-R"),
	SND_SOC_DAPM_INPUT("SDIN2-L"),
	SND_SOC_DAPM_INPUT("SDIN2-R"),
	SND_SOC_DAPM_INPUT("SDIN3-L"),
	SND_SOC_DAPM_INPUT("SDIN3-R"),
	SND_SOC_DAPM_INPUT("SDIN4-L"),
	SND_SOC_DAPM_INPUT("SDIN4-R"),

	SND_SOC_DAPM_OUTPUT("PWM1"),
	SND_SOC_DAPM_OUTPUT("PWM2"),
	SND_SOC_DAPM_OUTPUT("PWM3"),
	SND_SOC_DAPM_OUTPUT("PWM4"),
	SND_SOC_DAPM_OUTPUT("PWM5"),
	SND_SOC_DAPM_OUTPUT("PWM6"),

	SND_SOC_DAPM_MUX("Channel 1 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_input_mux_controls[0]),
	SND_SOC_DAPM_MUX("Channel 2 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_input_mux_controls[1]),
	SND_SOC_DAPM_MUX("Channel 3 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_input_mux_controls[2]),
	SND_SOC_DAPM_MUX("Channel 4 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_input_mux_controls[3]),
	SND_SOC_DAPM_MUX("Channel 5 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_input_mux_controls[4]),
	SND_SOC_DAPM_MUX("Channel 6 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_input_mux_controls[5]),

	SND_SOC_DAPM_MUX("PWM1 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_output_mux_controls[0]),
	SND_SOC_DAPM_MUX("PWM2 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_output_mux_controls[1]),
	SND_SOC_DAPM_MUX("PWM3 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_output_mux_controls[2]),
	SND_SOC_DAPM_MUX("PWM4 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_output_mux_controls[3]),
	SND_SOC_DAPM_MUX("PWM5 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_output_mux_controls[4]),
	SND_SOC_DAPM_MUX("PWM6 Mux", SND_SOC_NOPM, 0, 0,
			 &tas55x8_dapm_output_mux_controls[5]),
};

static const struct snd_soc_dapm_route tas55x8_dapm_routes[] = {
	/* SDIN inputs -> channel muxes */
	{ "Channel 1 Mux", "SDIN1-L", "SDIN1-L" },
	{ "Channel 1 Mux", "SDIN1-R", "SDIN1-R" },
	{ "Channel 1 Mux", "SDIN2-L", "SDIN2-L" },
	{ "Channel 1 Mux", "SDIN2-R", "SDIN2-R" },
	{ "Channel 1 Mux", "SDIN3-L", "SDIN3-L" },
	{ "Channel 1 Mux", "SDIN3-R", "SDIN3-R" },

	{ "Channel 2 Mux", "SDIN1-L", "SDIN1-L" },
	{ "Channel 2 Mux", "SDIN1-R", "SDIN1-R" },
	{ "Channel 2 Mux", "SDIN2-L", "SDIN2-L" },
	{ "Channel 2 Mux", "SDIN2-R", "SDIN2-R" },
	{ "Channel 2 Mux", "SDIN3-L", "SDIN3-L" },
	{ "Channel 2 Mux", "SDIN3-R", "SDIN3-R" },

	{ "Channel 2 Mux", "SDIN1-L", "SDIN1-L" },
	{ "Channel 2 Mux", "SDIN1-R", "SDIN1-R" },
	{ "Channel 2 Mux", "SDIN2-L", "SDIN2-L" },
	{ "Channel 2 Mux", "SDIN2-R", "SDIN2-R" },
	{ "Channel 2 Mux", "SDIN3-L", "SDIN3-L" },
	{ "Channel 2 Mux", "SDIN3-R", "SDIN3-R" },

	{ "Channel 3 Mux", "SDIN1-L", "SDIN1-L" },
	{ "Channel 3 Mux", "SDIN1-R", "SDIN1-R" },
	{ "Channel 3 Mux", "SDIN2-L", "SDIN2-L" },
	{ "Channel 3 Mux", "SDIN2-R", "SDIN2-R" },
	{ "Channel 3 Mux", "SDIN3-L", "SDIN3-L" },
	{ "Channel 3 Mux", "SDIN3-R", "SDIN3-R" },

	{ "Channel 4 Mux", "SDIN1-L", "SDIN1-L" },
	{ "Channel 4 Mux", "SDIN1-R", "SDIN1-R" },
	{ "Channel 4 Mux", "SDIN2-L", "SDIN2-L" },
	{ "Channel 4 Mux", "SDIN2-R", "SDIN2-R" },
	{ "Channel 4 Mux", "SDIN3-L", "SDIN3-L" },
	{ "Channel 4 Mux", "SDIN3-R", "SDIN3-R" },

	{ "Channel 5 Mux", "SDIN1-L", "SDIN1-L" },
	{ "Channel 5 Mux", "SDIN1-R", "SDIN1-R" },
	{ "Channel 5 Mux", "SDIN2-L", "SDIN2-L" },
	{ "Channel 5 Mux", "SDIN2-R", "SDIN2-R" },
	{ "Channel 5 Mux", "SDIN3-L", "SDIN3-L" },
	{ "Channel 5 Mux", "SDIN3-R", "SDIN3-R" },

	{ "Channel 6 Mux", "SDIN1-L", "SDIN1-L" },
	{ "Channel 6 Mux", "SDIN1-R", "SDIN1-R" },
	{ "Channel 6 Mux", "SDIN2-L", "SDIN2-L" },
	{ "Channel 6 Mux", "SDIN2-R", "SDIN2-R" },
	{ "Channel 6 Mux", "SDIN3-L", "SDIN3-L" },
	{ "Channel 6 Mux", "SDIN3-R", "SDIN3-R" },

	/* Channel muxes -> PWM muxes */
	{ "PWM1 Mux", "Channel 1 Mux", "Channel 1 Mux" },
	{ "PWM2 Mux", "Channel 1 Mux", "Channel 1 Mux" },
	{ "PWM3 Mux", "Channel 1 Mux", "Channel 1 Mux" },
	{ "PWM4 Mux", "Channel 1 Mux", "Channel 1 Mux" },
	{ "PWM5 Mux", "Channel 1 Mux", "Channel 1 Mux" },
	{ "PWM6 Mux", "Channel 1 Mux", "Channel 1 Mux" },

	{ "PWM1 Mux", "Channel 2 Mux", "Channel 2 Mux" },
	{ "PWM2 Mux", "Channel 2 Mux", "Channel 2 Mux" },
	{ "PWM3 Mux", "Channel 2 Mux", "Channel 2 Mux" },
	{ "PWM4 Mux", "Channel 2 Mux", "Channel 2 Mux" },
	{ "PWM5 Mux", "Channel 2 Mux", "Channel 2 Mux" },
	{ "PWM6 Mux", "Channel 2 Mux", "Channel 2 Mux" },

	{ "PWM1 Mux", "Channel 3 Mux", "Channel 3 Mux" },
	{ "PWM2 Mux", "Channel 3 Mux", "Channel 3 Mux" },
	{ "PWM3 Mux", "Channel 3 Mux", "Channel 3 Mux" },
	{ "PWM4 Mux", "Channel 3 Mux", "Channel 3 Mux" },
	{ "PWM5 Mux", "Channel 3 Mux", "Channel 3 Mux" },
	{ "PWM6 Mux", "Channel 3 Mux", "Channel 3 Mux" },

	{ "PWM1 Mux", "Channel 4 Mux", "Channel 4 Mux" },
	{ "PWM2 Mux", "Channel 4 Mux", "Channel 4 Mux" },
	{ "PWM3 Mux", "Channel 4 Mux", "Channel 4 Mux" },
	{ "PWM4 Mux", "Channel 4 Mux", "Channel 4 Mux" },
	{ "PWM5 Mux", "Channel 4 Mux", "Channel 4 Mux" },
	{ "PWM6 Mux", "Channel 4 Mux", "Channel 4 Mux" },

	{ "PWM1 Mux", "Channel 5 Mux", "Channel 5 Mux" },
	{ "PWM2 Mux", "Channel 5 Mux", "Channel 5 Mux" },
	{ "PWM3 Mux", "Channel 5 Mux", "Channel 5 Mux" },
	{ "PWM4 Mux", "Channel 5 Mux", "Channel 5 Mux" },
	{ "PWM5 Mux", "Channel 5 Mux", "Channel 5 Mux" },
	{ "PWM6 Mux", "Channel 5 Mux", "Channel 5 Mux" },

	{ "PWM1 Mux", "Channel 6 Mux", "Channel 6 Mux" },
	{ "PWM2 Mux", "Channel 6 Mux", "Channel 6 Mux" },
	{ "PWM3 Mux", "Channel 6 Mux", "Channel 6 Mux" },
	{ "PWM4 Mux", "Channel 6 Mux", "Channel 6 Mux" },
	{ "PWM5 Mux", "Channel 6 Mux", "Channel 6 Mux" },
	{ "PWM6 Mux", "Channel 6 Mux", "Channel 6 Mux" },

	/* The PWM muxes are directly connected to the PWM outputs */
	{ "PWM1", NULL, "PWM1 Mux" },
	{ "PWM2", NULL, "PWM2 Mux" },
	{ "PWM3", NULL, "PWM3 Mux" },
	{ "PWM4", NULL, "PWM4 Mux" },
	{ "PWM5", NULL, "PWM5 Mux" },
	{ "PWM6", NULL, "PWM6 Mux" },

};

static const struct snd_soc_dai_ops tas55x8_dai_ops = {
	.hw_params	= tas55x8_hw_params,
	.set_sysclk	= tas55x8_set_dai_sysclk,
	.set_fmt	= tas55x8_set_dai_fmt,
	.mute_stream	= tas55x8_mute_stream,
};

static struct snd_soc_dai_driver tas55x8_dai = {
	.name = "tas55x8-hifi",
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 2,
		.channels_max	= 6,
		.rates		= tas55x8_PCM_RATES,
		.formats	= tas55x8_PCM_FORMATS,
	},
	.ops = &tas55x8_dai_ops,
};

#ifdef CONFIG_PM
static int tas55x8_soc_suspend(struct snd_soc_component *component)
{
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);
	int ret;

	/* Shut down all channels */
	ret = regmap_write(priv->regmap, tas55x8_SYS_CONTROL_2, 0x60);
	if (ret < 0)
		return ret;

	regulator_bulk_disable(ARRAY_SIZE(priv->supplies), priv->supplies);

	return 0;
}

static int tas55x8_soc_resume(struct snd_soc_component *component)
{
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(priv->supplies), priv->supplies);
	if (ret < 0)
		return ret;

	tas55x8_reset(priv);
	regcache_mark_dirty(priv->regmap);

	ret = tas55x8_init(component->dev, priv);
	if (ret < 0)
		return ret;

	ret = regcache_sync(priv->regmap);
	if (ret < 0)
		return ret;

	return 0;
}
#else
#define tas55x8_soc_suspend	NULL
#define tas55x8_soc_resume	NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_OF
static const struct of_device_id tas55x8_dt_ids[] = {
	{ .compatible = "ti,tas55x8", },
	{ }
};
MODULE_DEVICE_TABLE(of, tas55x8_dt_ids);
#endif

static int tas55x8_probe(struct snd_soc_component *component)
{
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);
	int i, ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(priv->supplies), priv->supplies);
	if (ret < 0) {
		dev_err(component->dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	priv->pwm_start_mid_z = 0;
	priv->charge_period = 1300000; /* hardware default is 1300 ms */

	if (of_match_device(of_match_ptr(tas55x8_dt_ids), component->dev)) {
		struct device_node *of_node = component->dev->of_node;

		of_property_read_u32(of_node, "ti,charge-period",
				     &priv->charge_period);

		for (i = 0; i < 6; i++) {
			char name[25];

			snprintf(name, sizeof(name),
				 "ti,mid-z-channel-%d", i + 1);

			if (of_property_read_bool(of_node, name))
				priv->pwm_start_mid_z |= 1 << i;
		}
	}

	tas55x8_reset(priv);
	ret = tas55x8_init(component->dev, priv);
	if (ret < 0)
		goto exit_disable_regulators;

	/* set master volume to 0 dB */
	ret = regmap_write(priv->regmap, tas55x8_MASTER_VOL, 0x30);
	if (ret < 0)
		goto exit_disable_regulators;

	return 0;

exit_disable_regulators:
	regulator_bulk_disable(ARRAY_SIZE(priv->supplies), priv->supplies);

	return ret;
}

static void tas55x8_remove(struct snd_soc_component *component)
{
	struct tas55x8_private *priv = snd_soc_component_get_drvdata(component);

	if (priv->reset) {
		/* Set codec to the reset state */
		gpiod_set_value_cansleep(priv->reset, 1);
	}

	regulator_bulk_disable(ARRAY_SIZE(priv->supplies), priv->supplies);
};

static const struct snd_soc_component_driver soc_component_dev_tas55x8 = {
	.probe			= tas55x8_probe,
	.remove			= tas55x8_remove,
	.suspend		= tas55x8_soc_suspend,
	.resume			= tas55x8_soc_resume,
	.controls		= tas55x8_controls,
	.num_controls		= ARRAY_SIZE(tas55x8_controls),
	.dapm_widgets		= tas55x8_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(tas55x8_dapm_widgets),
	.dapm_routes		= tas55x8_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(tas55x8_dapm_routes),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
};

static const struct i2c_device_id tas55x8_i2c_id[] = {
	{ "tas55x8" },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tas55x8_i2c_id);

static const struct regmap_config tas55x8_regmap = {
	.reg_bits		= 8,
	.val_bits		= 32, //4 bytes, or 5 * 4 Bytes = 20 Bytes = 160 bits
	.max_register		= MAX_REGISTER,
	.reg_defaults		= tas55x8_reg_defaults,
	.num_reg_defaults	= ARRAY_SIZE(tas55x8_reg_defaults),
	.cache_type		= REGCACHE_RBTREE,
	.volatile_reg		= tas55x8_volatile_reg,
	.writeable_reg		= tas55x8_writeable_reg,
	.readable_reg		= tas55x8_accessible_reg,
	.reg_read		= tas55x8_reg_read,
	.reg_write		= tas55x8_reg_write,
};

static int tas55x8_i2c_probe(struct i2c_client *i2c)
{
	struct tas55x8_private *priv;
	struct device *dev = &i2c->dev;
	int i, ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(supply_names); i++)
		priv->supplies[i].supply = supply_names[i];

	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(priv->supplies),
				      priv->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to get regulators: %d\n", ret);
		return ret;
	}

	priv->regmap = devm_regmap_init(dev, NULL, i2c, &tas55x8_regmap);
	if (IS_ERR(priv->regmap)) {
		ret = PTR_ERR(priv->regmap);
		dev_err(&i2c->dev, "Failed to create regmap: %d\n", ret);
		return ret;
	}

	i2c_set_clientdata(i2c, priv);

	/* Request line asserted */
	priv->reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(priv->reset))
		return PTR_ERR(priv->reset);
	gpiod_set_consumer_name(priv->reset, "tas55x8 Reset");

	ret = regulator_bulk_enable(ARRAY_SIZE(priv->supplies), priv->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	tas55x8_reset(priv);

	/* The tas55x8 always returns 0x03 in its tas55x8_DEV_ID register */
	ret = regmap_read(priv->regmap, tas55x8_DEV_ID, &i);
	if (ret == 0 && i != 0x3) {
		dev_err(dev,
			"Failed to identify tas55x8 codec (got %02x)\n", i);
		ret = -ENODEV;
	}

	/*
	 * The chip has been identified, so we can turn off the power
	 * again until the dai link is set up.
	 */
	regulator_bulk_disable(ARRAY_SIZE(priv->supplies), priv->supplies);

	if (ret == 0)
		ret = devm_snd_soc_register_component(&i2c->dev,
					     &soc_component_dev_tas55x8,
					     &tas55x8_dai, 1);

	return ret;
}

static void tas55x8_i2c_remove(struct i2c_client *i2c)
{}

static struct i2c_driver tas55x8_i2c_driver = {
	.driver = {
		.name	= "tas55x8",
		.of_match_table = of_match_ptr(tas55x8_dt_ids),
	},
	.id_table	= tas55x8_i2c_id,
	.probe		= tas55x8_i2c_probe,
	.remove		= tas55x8_i2c_remove,
};

module_i2c_driver(tas55x8_i2c_driver);

MODULE_AUTHOR("Daniel Mack <zonque@gmail.com>");
MODULE_DESCRIPTION("Texas Instruments tas55x8 ALSA SoC Codec Driver");
MODULE_LICENSE("GPL");