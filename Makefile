obj-m	:= src/qres_mod.o

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD		:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
