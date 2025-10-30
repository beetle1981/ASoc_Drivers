KERNELDIR ?= ~/6.12__rockchip64__arm64
#KERNELDIR ?= ~/armbian/cache/sources/linux-kernel-worktree/6.12__rockchip64__arm64
PWD := $(shell pwd)

ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-
export ARCH CROSS_COMPILE


obj-m := ak7755.o

	
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

#	export ARCH=arm64
#	export CROSS_COMPILE=aarch64-linux-gnu-
