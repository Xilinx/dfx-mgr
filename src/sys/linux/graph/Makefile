APP = test
OUTPUT_PATH ?= `pwd`/../../systest

LIBSOURCES=*.c
OUTS = *.o
NAME := SIHA

CC := aarch64-linux-gnu-gcc
BIGFILES=-D_FILE_OFFSET_BITS=64
CFLAGS := -g -Wall -I ./layer0 -I ../jsmn -I ../include -L ../lib -fPIC -lpthread -lacapd -lwebsockets -lfpga -lsihaHLSdm -lsoftdm -lfallback -lsiha

DEPS = 
OBJ = graphClient.o abstractGraph.o metadata.o graph.o scheduler.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

siha:
	$(MAKE) -C layer0 all OUTPUT_PATH=$(OUTPUT_PATH)

main: main.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

graphServer: graphServer.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

#graphClient: graphClient.o $(OBJ)
#	$(CC) -o $@ $^ $(CFLAGS)

graph: $(OBJ)
	$(CC) $^ -shared -o libgraph.so $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ main *.so

install:
	mkdir -p $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	mkdir -p $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	/usr/bin/cp -rf libgraph.so ../lib
	/usr/bin/cp -rf libgraph.so $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	/usr/bin/cp -rf ../lib/* $(OUTPUT_PATH)/linux/rootfs/home/root/lib
	/usr/bin/cp -rf ../bin/* $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	/usr/bin/cp -rf main $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	/usr/bin/cp -rf graphServer $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	/usr/bin/cp -rf graphClient $(OUTPUT_PATH)/linux/rootfs/home/root/bin
	

all: clean siha graph main graphServer   

#client install
