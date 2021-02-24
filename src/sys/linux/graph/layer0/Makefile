APP = test
OUTPUT_PATH ?= ../../../systest

LIBSOURCES=*.c
OUTS = *.o
NAME := SIHA

CC := aarch64-linux-gnu-gcc
BIGFILES=-D_FILE_OFFSET_BITS=64
#CFLAGS := -g -Wall -I ../include -L ../lib -I /scratch/abajpai/XRT/src/runtime_src/core/include/ -I /scratch/abajpai/XRT/src -I /scratch/abajpai/XRT/src/runtime_src/core/edge/include -I ../../petalinux/build/tmp/sysroots-components/aarch64/libdrm/usr/include -fPIC -lpthread -lwebsockets
CFLAGS := -g -Wall -I ../../include -L ../../lib -fPIC -lpthread -lacapd -lwebsockets -lfpga

DEPS = 
#OBJ = acapdwebsoc.o shellDriver.o dm.o utils.o uio.o xrtbuffer.o siha.o queue.o main.o #client.o
#OBJ = slotManager.o shellDriver.o utils.o uio.o xrtbuffer.o siha.o queue.o #client.o
OBJ =  shellDriver.o utils.o uio.o xrtbuffer.o siha.o queue.o #client.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

siha: $(OBJ)
	$(CC) $^ -shared -o libsiha.so $(CFLAGS)

sihaHLSlib:
	$(MAKE) -C sihaHLS all OUTPUT_PATH=$(OUTPUT_PATH)

softIOlib:
	$(MAKE) -C softIO all OUTPUT_PATH=$(OUTPUT_PATH)

fallbacklib:
	$(MAKE) -C fallback all OUTPUT_PATH=$(OUTPUT_PATH)

.PHONY: clean

clean:
	rm -f *.o *~ main *.so

install:
	#mkdir -p $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	#mkdir -p $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	#/usr/bin/cp -rf libsiha.so $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	#/usr/bin/cp -rf ../lib/* $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	#/usr/bin/cp -rf ../bin/* $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	#/usr/bin/cp -rf main $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	/usr/bin/cp -rf libsiha.so ../../lib
	/usr/bin/cp -rf libsiha.so $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	

all: clean sihaHLSlib softIOlib fallbacklib siha install 

#client install:wq

