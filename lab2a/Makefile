# Short instruction for building kernel module

CFLAGS = -Wall -g

PRVI_C = prvi.c
PRVI_O = ${PRVI_C:.c=.o}

DRUGI_C = drugi.c
DRUGI_O = ${DRUGI_C:.c=.o}

ifneq ($(KERNELRELEASE),)
# call from kernel build system

# Add your debugging flag (or not) to CFLAGS
# debug by default, comment next line if required
DEBUG = y
ifeq ($(DEBUG),y)
  ccflags-y += -DSHOFER_DEBUG
endif

obj-m	:= shofer.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

labos: prvi drugi

prvi: $(PRVI_O)
	$(CC) $(PRVI_O) -o ${PRVI_C:.c=}

drugi: $(DRUGI_O)
	$(CC) $(DRUGI_O) -o ${DRUGI_C:.c=}


clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f prvi drugi prvi.o drugi.o
