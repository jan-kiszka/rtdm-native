RTDMDIR := /home/mah/rtdm-native
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += rtdmtest.o

obj-m += rtdm.o
rtdm-objs += ../../skins/rtdm/native/module.o \
	../../skins/rtdm/native/device.o \
	../../skins/rtdm/native/drvlib.o \
	../../skins/rtdm/native/core.o

ccflags-y := -DCONFIG_XENO_OPT_DEBUG_RTDM=1
ccflags-y += -I$(RTDMDIR)/include/


default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

test:
	echo $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.mod.c *.ko *.o *.symvers *.order ../../skins/rtdm/native/*.o


install: rtdm.ko rtdmtest.ko
	mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/rtdm
	cp rtdm.ko rtdmtest.ko /lib/modules/$(shell uname -r)/kernel/drivers/rtdm
	depmod -a
