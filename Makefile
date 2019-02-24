# Makefile supports different kernel path
# Jul 1, 2014
# root@davejingtian.org
# http://davejingtian.org

ifneq ($(KERNELPATH),)
	KDIR	:= $(KERNELPATH)
else
	# Default kernel path
	KDIR	:= /lib/modules/$(shell uname -r)/build
endif

ARCH	:= $(shell uname -p)

ifeq ($(ARCH),x86_64)
	KDEF	:= X86_64
else
	KDEF	:= AARCH64
endif


PWD	:= $(shell pwd)
MOD	:= sch

obj-m += $(MOD).o

EXTRA_CFLAGS	:= -D $(KDEF)

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
