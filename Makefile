# Cross compilation Makefile for ARM
KERN_SRC=/opt/PHYTEC_BSPs/phyCORE-AM335x-PD13.1.0/platform-phyCORE-AM335x/build-target/linux-3.2
obj-m := hwcrypt.o

all:
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-cortexa8-linux-gnueabihf- M=`pwd` modules
clean:
	make -C $(KERN_SRC) ARCH=arm CROSS_COMPILE=arm-cortexa8-linux-gnueabihf- M=`pwd` clean

