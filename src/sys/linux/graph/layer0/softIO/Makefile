APP = test
OUTPUT_PATH ?= ../../../../systest

LIBSOURCES=*.c
OUTS = *.o
NAME := SIHA

CC := aarch64-linux-gnu-gcc
BIGFILES=-D_FILE_OFFSET_BITS=64
CFLAGS := -g -Wall -I ../ -I ../../../include -L ../../../lib -fPIC -lpthread -lacapd -lwebsockets -lfpga

DEPS = 
OBJ = softdm.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

softdm: $(OBJ)
	$(CC) $^ -shared -o libsoftdm.so $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ main *.so

install:
	/usr/bin/cp -rf libsoftdm.so ../../../lib
	/usr/bin/cp -rf libsoftdm.so $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	#mkdir -p $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	#mkdir -p $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	#/usr/bin/cp -rf libsiha.so $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	#/usr/bin/cp -rf ../lib/* $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	#/usr/bin/cp -rf ../bin/* $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	#/usr/bin/cp -rf main $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	

all: clean softdm install
#client install
