/*
 * tas55x8.h -- audio driver for tas55x8
 */

#ifndef _TAS55x8_H
#define _TAS55x8_H

//#define TAS55x8_I2C_IF
#define TAS55x8_IO_CONTROL



#define I2C_ADDR       0x36; /*write address 0x36 and read is 0x37*/
#define TAS5558_FS   96000;

//Regiser Map 8 bits (1 bytes)
#define CLOCK_CONTROL_REG          0x00; /**/
#define GENERAL_STATUS_REG         0x01; /*ID code for TAS5558*/
#define ERROR_STATUS_REG           0x02; /*CLIP and frame slip errors*/
#define SYSTEM_CONTROL1_REG        0x03; /*PWM high pass, clock set, umute select, PSVC select*/
#define SYSTEM_CONTROL2_REG        0x04; /*Automute, shutdown, Line out, SDOUT*/

#define CH1_CONFIG_CONTROL_REG     0x05;
#define CH2_CONFIG_CONTROL_REG     0x06;
#define CH3_CONFIG_CONTROL_REG     0x07;
#define CH4_CONFIG_CONTROL_REG     0x08;
#define CH5_CONFIG_CONTROL_REG     0x09;
#define CH6_CONFIG_CONTROL_REG     0x0A;
#define CH7_CONFIG_CONTROL_REG     0x0B;
#define CH8_CONFIG_CONTROL_REG     0x0C;

#define HEADPHONE_CONFIG_CONTROL_REG           0x0D; /*configure headphone output*/
#define SERIAL_DATA_INTERFACE_CONTROL_REG      0x0E; /*Set serial data interface to right-justified, I2S, or left-justified.*/
#define SOFT_MUTE_REG                          0x0F; /*Soft mute for chanels*/
#define ENERGY_MANAGERS_REG                    0x10; 
#define RESERVED                               0x11; /**/
#define OSCILLATOR_TRIM                        0x12; /**/
#define RESERVED0                              0x13; /**/
#define AUTOMUTE_CONTROL1_REG                  0x14; /*Set automute delay and threshold*/
#define AUTOMUTE_CONTROL2_REG                  0x15; /*Set PWM automute threshold; set back-end reset period*/

#define MODULATION12_LIMIT_REG     0x16;
#define MODULATION34_LIMIT_REG     0x17;
#define MODULATION56_LIMIT_REG     0x18;
#define MODULATION78_LIMIT_REG     0x19;
#define RESERVED1                  0x1A;

#define DELAY_CH1_REG     0x1B;
#define DELAY_CH2_REG     0x1C;
#define DELAY_CH3_REG     0x1D;
#define DELAY_CH4_REG     0x1E;
#define DELAY_CH5_REG     0x1F;
#define DELAY_CH6_REG     0x20;
#define DELAY_CH7_REG     0x21;
#define DELAY_CH8_REG     0x22;

#define OFFSET_DELAY_REG               0x23;
#define PWM_SEQUENCE_TIMING_REG        0x24;
#define PWM_ENERGY_MANAGER_REG         0x25;
#define RESERVED2                      0x26;
#define INDIVIDUAL_CH_SHUTDOWN_REG     0x27;
#define RESERVED3                      0x28; //0x28-0x2F
#define RESERVED10                     0X2F;

#define INPUT_MUX_CH12_REG     0x30;
#define INPUT_MUX_CH34_REG     0x31;
#define INPUT_MUX_CH56_REG     0x32;
#define INPUT_MUX_CH78_REG     0x33;

#define PWM_MUX_CH12_REG     0x34;
#define PWM_MUX_CH34_REG     0x35;
#define PWM_MUX_CH56_REG     0x36;
#define PWM_MUX_CH78_REG     0x37;

#define DELAY_CH1_BD_MODE_REG     0x38;
#define DELAY_CH2_BD_MODE_REG     0x39;
#define DELAY_CH3_BD_MODE_REG     0x3A;
#define DELAY_CH4_BD_MODE_REG     0x3B;
#define DELAY_CH5_BD_MODE_REG     0x3C;
#define DELAY_CH6_BD_MODE_REG     0x3D;
#define DELAY_CH7_BD_MODE_REG     0x3E;
#define DELAY_CH8_BD_MODE_REG     0x3F;


//register address N bits (N/8 bytes)
 
#define BANK_SWITCHING_CMD_REG     0x40;
#define INPUT_MIXER_CH1_REG        0x41; //0x41-0x48
#define INPUT_MIXER_CH2_REG        0x42;
#define INPUT_MIXER_CH3_REG        0x43;
#define INPUT_MIXER_CH4_REG        0x44;
#define INPUT_MIXER_CH5_REG        0x45;
#define INPUT_MIXER_CH6_REG        0x46;
#define INPUT_MIXER_CH7_REG        0x47;
#define INPUT_MIXER_CH8_REG        0x48;

//bass mixer
#define IPMIX1_TO_CH8_REG          0x49; //Input mixer 1 to Ch8 mixer coefficient (default = 0)
#define IPMIX2_TO_CH8_REG          0x4A; //Input mixer 2 to Ch8 mixer coefficient (default = 0) 0000 0000u[31:28]
#define IPMIX7_TO_CH12_REG         0x4B; //Ch7 biquad-2 output to Ch1 mixer and Ch2 mixer coefficient 0000 0000(default = 0)
#define CH7_BQ_BQ2_REG             0x4C; //Ch7 biquad-2 bypass coefficient (default = 0)
#define CH7_BQ2_REG                0x4D; //Ch7 biquad-2 inline coefficient (default = 1)
#define IPMIX8_TO_CH12_REG         0x4E; //Ch8 biquad-2 output to Ch1 mixer and Ch2 mixer coefficient
#define CH8_BQ_BQ2_REG             0x4F; //Ch8 biquad-2 bypass coefficient (default = 0)
#define CH8_BQ2_REG                0x50; //Ch8 biquad-2 inline coefficient (default = 1)

//Biquad Filter, 0x51-0x88, 7biquads * 8channels, 20 Bytes per BQ
#define BQFILTER_CH1_BQ1_REG          0x51; //BQ1 FILTER
#define BQFILTER_CH1_BQ2_REG          0x52; //BQ2 FILTER
#define BQFILTER_CH1_BQ3_REG          0x53; //BQ3 FILTER
#define BQFILTER_CH1_BQ4_REG          0x54; //BQ4 FILTER
#define BQFILTER_CH1_BQ5_REG          0x55; //BQ5 FILTER
#define BQFILTER_CH1_BQ6_REG          0x56; //BQ6 FILTER
#define BQFILTER_CH1_BQ7_REG          0x57; //BQ7 FILTER

#define BQFILTER_CH2_BQ1_REG          0x58; //BQ1 FILTER
#define BQFILTER_CH2_BQ2_REG          0x59; //BQ2 FILTER
#define BQFILTER_CH2_BQ3_REG          0x5A; //BQ3 FILTER
#define BQFILTER_CH2_BQ4_REG          0x5B; //BQ4 FILTER
#define BQFILTER_CH2_BQ5_REG          0x5C; //BQ5 FILTER
#define BQFILTER_CH2_BQ6_REG          0x5D; //BQ6 FILTER
#define BQFILTER_CH2_BQ7_REG          0x5E; //BQ7 FILTER

#define BQFILTER_CH3_BQ1_REG          0x5F; //BQ1 FILTER
#define BQFILTER_CH3_BQ2_REG          0x60; //BQ2 FILTER
#define BQFILTER_CH3_BQ3_REG          0x61; //BQ3 FILTER
#define BQFILTER_CH3_BQ4_REG          0x62; //BQ4 FILTER
#define BQFILTER_CH3_BQ5_REG          0x63; //BQ5 FILTER
#define BQFILTER_CH3_BQ6_REG          0x64; //BQ6 FILTER
#define BQFILTER_CH3_BQ7_REG          0x65; //BQ7 FILTER

#define BQFILTER_CH4_BQ1_REG          0x66; //BQ1 FILTER
#define BQFILTER_CH4_BQ2_REG          0x67; //BQ2 FILTER
#define BQFILTER_CH4_BQ3_REG          0x68; //BQ3 FILTER
#define BQFILTER_CH4_BQ4_REG          0x69; //BQ4 FILTER
#define BQFILTER_CH4_BQ5_REG          0x6A; //BQ5 FILTER
#define BQFILTER_CH4_BQ6_REG          0x6B; //BQ6 FILTER
#define BQFILTER_CH4_BQ7_REG          0x6C; //BQ7 FILTER

#define BQFILTER_CH5_BQ1_REG          0x6D; //BQ1 FILTER
#define BQFILTER_CH5_BQ2_REG          0x6E; //BQ2 FILTER
#define BQFILTER_CH5_BQ3_REG          0x6F; //BQ3 FILTER
#define BQFILTER_CH5_BQ4_REG          0x70; //BQ4 FILTER
#define BQFILTER_CH5_BQ5_REG          0x71; //BQ5 FILTER
#define BQFILTER_CH5_BQ6_REG          0x72; //BQ6 FILTER
#define BQFILTER_CH5_BQ7_REG          0x73; //BQ7 FILTER

#define BQFILTER_CH6_BQ1_REG          0x74; //BQ1 FILTER
#define BQFILTER_CH6_BQ2_REG          0x75; //BQ2 FILTER
#define BQFILTER_CH6_BQ3_REG          0x76; //BQ3 FILTER
#define BQFILTER_CH6_BQ4_REG          0x77; //BQ4 FILTER
#define BQFILTER_CH6_BQ5_REG          0x78; //BQ5 FILTER
#define BQFILTER_CH6_BQ6_REG          0x79; //BQ6 FILTER
#define BQFILTER_CH6_BQ7_REG          0x7A; //BQ7 FILTER

#define BQFILTER_CH7_BQ1_REG          0x7B; //BQ1 FILTER
#define BQFILTER_CH7_BQ2_REG          0x7C; //BQ2 FILTER
#define BQFILTER_CH7_BQ3_REG          0x7D; //BQ3 FILTER
#define BQFILTER_CH7_BQ4_REG          0x7E; //BQ4 FILTER
#define BQFILTER_CH7_BQ5_REG          0x7F; //BQ5 FILTER
#define BQFILTER_CH7_BQ6_REG          0x80; //BQ6 FILTER
#define BQFILTER_CH7_BQ7_REG          0x81; //BQ7 FILTER

#define BQFILTER_CH8_BQ1_REG          0x82; //BQ1 FILTER
#define BQFILTER_CH8_BQ2_REG          0x83; //BQ2 FILTER
#define BQFILTER_CH8_BQ3_REG          0x84; //BQ3 FILTER
#define BQFILTER_CH8_BQ4_REG          0x85; //BQ4 FILTER
#define BQFILTER_CH8_BQ5_REG          0x86; //BQ5 FILTER
#define BQFILTER_CH8_BQ6_REG          0x87; //BQ6 FILTER
#define BQFILTER_CH8_BQ7_REG          0x88; //BQ7 FILTER

// bass and treble bypass
#define BASS_TREBLE_CH1_REG            0x89; //0x89-0x90
#define BASS_TREBLE_CH2_REG            0x8A;
#define BASS_TREBLE_CH3_REG            0x8B;
#define BASS_TREBLE_CH4_REG            0x8C;
#define BASS_TREBLE_CH5_REG            0x8D;
#define BASS_TREBLE_CH6_REG            0x8E;
#define BASS_TREBLE_CH7_REG            0x8F;
#define BASS_TREBLE_CH8_REG            0x90;

#define LOUDNESS_LOG2_GAIN_REG         0x91;
#define LOUDNESS_LOG2_OFFSET_REG       0x92;
#define LOUDNESS_GAIN_REG              0x93;
#define LOUDNESS_OFFSET_REG            0x94;
#define LOUDNESS_BIQUAD_REG            0x95;

#define DRC1_CONTROL_REG               0x96;
#define DRC2_CONTROL_REG               0x97;

#define DRC1_ENERGY_REG                0x98;
#define DRC1_THRESHOLD_REG             0x99;
#define DRC1_SLOPE_REG                 0x9A;
#define DRC1_OFFSET_REG                0x9B;
#define DRC1_ATTACK_DECAY_REG          0x9C;

#define DRC2_ENERGY_REG                0x9D;
#define DRC2_THRESHOLD_REG             0x9E;
#define DRC2_SLOPE_REG                 0x9F;
#define DRC2_OFFSET_REG                0xA0;
#define DRC2_ATTACK_DECAY_REG          0xA1;

#define DRC_BYPASS1_REG        0xA2;
#define DRC_BYPASS2_REG        0xA3;
#define DRC_BYPASS3_REG        0xA4;
#define DRC_BYPASS4_REG        0xA5;
#define DRC_BYPASS5_REG        0xA6;
#define DRC_BYPASS6_REG        0xA7;
#define DRC_BYPASS7_REG        0xA8;
#define DRC_BYPASS8_REG        0xA9;

#define OUTPUT_TO_PWM1_REG     0xAA;
#define OUTPUT_TO_PWM2_REG     0xAB;
#define OUTPUT_TO_PWM3_REG     0xAC;
#define OUTPUT_TO_PWM4_REG     0xAD;
#define OUTPUT_TO_PWM5_REG     0xAE;
#define OUTPUT_TO_PWM6_REG     0xAF;
#define OUTPUT_TO_PWM7_REG     0xB0;
#define OUTPUT_TO_PWM8_REG     0xB1;

#define ENERGY_MANAGER_AVERAGING_REG           0xB2;
#define ENERGY_MANAGER_WEIGHTING_CH1_REG       0xB3;
#define ENERGY_MANAGER_WEIGHTING_CH2_REG       0xB4;
#define ENERGY_MANAGER_WEIGHTING_CH3_REG       0xB5;
#define ENERGY_MANAGER_WEIGHTING_CH4_REG       0xB6;
#define ENERGY_MANAGER_WEIGHTING_CH5_REG       0xB7;
#define ENERGY_MANAGER_WEIGHTING_CH6_REG       0xB8;
#define ENERGY_MANAGER_WEIGHTING_CH7_REG       0xB9;
#define ENERGY_MANAGER_WEIGHTING_CH8_REG       0xBA;

#define ENERGY_MANAGER_HIGH_THRESHOLD_SATELLITE_REG        0xBB;
#define ENERGY_MANAGER_LOW_THRESHOLD_SATELLITE_REG         0xBC;
#define ENERGY_MANAGER_HIGH_THRESHOLD_SUBWOOFER_REG        0xBD;
#define ENERGY_MANAGER_LOW_THRESHOLD_SUBWOOFER_REG         0xBE;
#define RESERVED11                                         0xBF; //0xBF-0xC2
#define RESERVED14                                         0xC2; 

#define ASRC_STATUS_REG            0xC3;
#define ASRC_CONTROL_REG           0xC4;
#define ASRC_MODE_CONTROL_REG      0xC5;
#define RESERVED15                 0xC6; //0xC6-0xCB
#define RESERVED20                 0xCB;

#define AUTO_MUTE_BEHAVIOUR                    0xCC;
#define RESERVED21                             0xCD; //0xCD-0xCE
#define RESERVED100                            0xCE;
#define PSVC_VOLUME_BIQUAD                     0xCF;
#define VOLUME_TREBLE_BASS_SLEW_RATES_REG      0xD0;

#define CH1_VOLUME_REG         0xD1;
#define CH2_VOLUME_REG         0xD2;
#define CH3_VOLUME_REG         0xD3;
#define CH4_VOLUME_REG         0xD4;
#define CH5_VOLUME_REG         0xD5;
#define CH6_VOLUME_REG         0xD6;
#define CH7_VOLUME_REG         0xD7;
#define CH8_VOLUME_REG         0xD8;
#define MASTER_VOLUME_REG      0xD9;
#define BASS_FILTER_SET_REG            0xDA;
#define BASS_FILTER_INDEX_REG          0xDB;
#define TREBLE_FILTER_SET_REG          0xDC;
#define TREBLE_FILTER_INDEX_REG        0xDD;
#define AM_MODE_REG                    0xDE;
#define PSVC_RANGE_REG                 0xDF;
#define GENERAL_CONTROL_REG            0xE0;
#define RESERVED22                     0xE1; // 0xE1-0xE2
#define RESERVED23                     0xE2;

#define R_DOLBY_COEFLR_REG         0xE3;
#define R_DOLBY_COEFC_REG          0xE4;
#define R_DOLBY_COEFLSP_REG        0xE5;
#define R_DOLBY_COEFRSP_REG        0xE6;
#define R_DOLBY_COEFLSM_REG        0xE7;
#define R_DOLBY_COEFRSM_REG        0xE8;

#define THD_MANAGER_PRE_REG        0xE9;
#define THD_MANAGER_POST_REG       0xEA;
#define RESERVED24                 0xEB;

#define SDIN5_INPUT1_MIX_REG       0xEC;
#define SDIN5_INPUT2_MIX_REG       0xED;
#define SDIN5_INPUT3_MIX_REG       0xEE;
#define SDIN5_INPUT4_MIX_REG       0xEF;
#define SDIN5_INPUT5_MIX_REG       0xF0;
#define SDIN5_INPUT6_MIX_REG       0xF1;
#define SDIN5_INPUT7_MIX_REG       0xF2;
#define SDIN5_INPUT8_MIX_REG       0xF3;

#define KHZ192_PROCESS_FLOW_OUTPUT_MIX1_REG        0xF4;
#define KHZ192_PROCESS_FLOW_OUTPUT_MIX2_REG        0xF5;
#define KHZ192_PROCESS_FLOW_OUTPUT_MIX3_REG        0xF6;
#define KHZ192_PROCESS_FLOW_OUTPUT_MIX4_REG        0xF7;
#define RESERVED25                                 0xF8; //0xF8-0xF9
#define RESERVED26                                 0xF9;

#define KHZ192_IMAGE_SELECT_REG            0xFA;
#define KHZ192_DOLBY_DOWNMIX_COEF1_REG     0xFB;
#define KHZ192_DOLBY_DOWNMIX_COEF2_REG     0xFC; 
#define RESERVED27                         0xFD;

#define SPECIAL_REG                        0xFE;
#define RESERVED28                         0xFF;

    //Clock Control Register Masks
#define DATA_RATE_MASK         0xE0;
#define DATA_RATE_32KHZ        0x00;
#define DATA_RATE_44_1KHZ      0x40;
#define DATA_RATE_48KHZ        0x60;
#define DATA_RATE_88_2KHZ      0x80;
#define DATA_RATE_96KHZ        0xA0;
#define DATA_RATE_176_4KHZ     0xC0;
#define DATA_RATE_192KHZ       0xE0;

#define MCLK_FREQ_MASK         0x1C;
#define MCLK_FREQ_64           0x00;
#define MCLK_FREQ_128          0x04;
#define MCLK_FREQ_192          0x08;
#define MCLK_FREQ_256          0x0C;
#define MCLK_FREQ_384          0x10;
#define MCLK_FREQ_512          0x14;
#define MCLK_FREQ_768          0x18;

#define CLK_REG_VALID_MASK     0x03;
#define CLK_REG_VALID          0x01;
#define CLK_REG_NOT_VALID      0x00;

    //Error Status Register Masks
#define FRAME_SLIP_MASK            0x08;
#define CLIP_INDICATOR_MASK        0x04;
#define FAULTZ_MASK                0x02;


#define TAS55x8_PCM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE  |		\
			     SNDRV_PCM_FMTBIT_S20_3LE |		\
			     SNDRV_PCM_FMTBIT_S24_3LE)

#define TAS55x8_PCM_RATES   (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100  | \
			     SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200  | \
			     SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 | \
			     SNDRV_PCM_RATE_192000)

#endif /* __TAS5558_H__ */