# Short instruction for building kernel module

ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-m	:= pipenos.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

test:
	$(CC) pisaci.c -o pisaci -Wall
	$(CC) citaci.c -o citaci -Wall

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f pisaci citaci
