obj-m	:= irmossup.o

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD		:= $(shell pwd)

default:
	 gcc -I /usr/src/linux-headers-2.6.32-24/include/ -c qres.c -std=c89 2> /dev/stdout
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
