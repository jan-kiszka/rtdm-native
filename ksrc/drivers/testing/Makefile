#obj-m := rtdmtest.o
obj-m += hello.o

obj-m += startstop.o
startstop-objs:= rtdmtest.o \
	../../skins/rtdm/native/device.o \
	../../skins/rtdm/native/drvlib.o \
	../../skins/rtdm/native/core.o

#	../../skins/rtdm/native/module.o

ccflags-y := -DCONFIG_XENO_OPT_DEBUG_RTDM=1 -DCONFIG_MODVERSIONS=1 -DMODVERSIONS=1
ccflags-y += -DMODULE -D__KERNEL__
ccflags-y += -isystem /lib/modules/`uname -r`/build/include
#ccflags-y += -W -Wall -Wstrict-prototypes -Wmissing-prototypes
ccflags-y += -I/home/mah/rtdm-native/include

KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

ifndef KVER
KVER := $(shell uname -r)
endif

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
#	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) ARCH=arm  modules

test:
	echo $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.mod.c *.ko *.o *.symvers *.order ../../skins/rtdm/native/*.o


install: rtcan_c_can.ko
	cp rtcan_c_can.ko /lib/modules/$(shell uname -r)/kernel/drivers/xenomai/can
	depmod -a
