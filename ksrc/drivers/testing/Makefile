obj-m := rtdmtest.o
rtdmtest-y := ../../skins/rtdm/native/device.o
rtdmtest-y := ../../skins/rtdm/native/core.o

ccflags-y := -DCONFIG_XENO_OPT_DEBUG_RTDM=1
ccflags-y += -I/home/mah/rtdm-native/include

#EXTRA_CFLAGS := -I/usr/src/linux-headers-$(shell uname -r)/include/xenomai/ #-I/home/mah/rtcan-bb/can

KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
#	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) ARCH=arm  modules

test:
	echo $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.mod.c *.ko *.o *.symvers *.order


install: rtcan_c_can.ko
	cp rtcan_c_can.ko /lib/modules/$(shell uname -r)/kernel/drivers/xenomai/can
	depmod -a
