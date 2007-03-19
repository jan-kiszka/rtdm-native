ifeq ($(PATCHLEVEL),6)

# Makefile frag for Linux v2.6

obj-$(CONFIG_XENO_SKIN_RTDM) += xeno_rtdm.o

xeno_rtdm-y := core.o device.o module.o drvlib.o

EXTRA_CFLAGS += -Iinclude/xenomai

else

# Makefile frag for Linux v2.4

O_TARGET := built-in.o

obj-$(CONFIG_XENO_SKIN_RTDM) := xeno_rtdm.o

list-multi := xeno_rtdm.o

xeno_rtdm-objs := core.o device.o module.o drvlib.o

opt_objs-y :=

xeno_rtdm-objs += $(opt_objs-y)

export-objs := $(xeno_rtdm-objs)

#EXTRA_CFLAGS += -D__IN_XENOMAI__ -I$(TOPDIR)/include/xenomai -I$(TOPDIR)/include/xenomai/compat -I..

include $(TOPDIR)/Rules.make

xeno_rtdm.o: $(xeno_rtdm-objs)
	$(LD) -r -o $@ $(xeno_rtdm-objs)

endif
