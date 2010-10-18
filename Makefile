obj-m	+= src/irmossup.o
obj-m	+= src/hello-1.o

src/irmossup-objs = src/qres_mod.o src/qres.o src/qsup.o src/qres_gw_ks.o src/qres_proc_fs.o src/qres_timer_thread.o src/qsup_gw_ks.o src/qsup_mod.o src/qos_debug.o  src/qos_memory.o src/qos_kernel_dep.o 

KBUILD_VERBOSE = 1
MODULE_EXT    := ko
KFLAGS   := -DQOS_KS -DQOS_DEBUG_LEVEL=8
#WARN    := -W -Wall -Wstrict-prototypes -Wmissing-prototypes
INCLUDE := -isystem /lib/modules/`uname -r`/build/include
EXTRA_CFLAGS  := -O2 -g ${KFLAGS} ${WARN} ${INCLUDE}

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD		:= $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
