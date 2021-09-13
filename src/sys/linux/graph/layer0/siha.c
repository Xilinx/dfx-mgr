/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <zynq_ioctl.h>
#include "dm.h"
#include "shellDriver.h"
#include "siha.h"
#include "utils.h"
#include "uio.h"
#include "xrtbuffer.h"
#include <dfx-mgr/print.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/daemon_helper.h>

static plDevices_t* pldevices=NULL; 
static Buffers_t* buffers=NULL; 
static int slotNum[] = {-1, -1, -1};
char slotStr[3][5];
long inthroughput[3], outthroughput[3]; 

long SIHAGetInThroughput(int slot){
	//INFO("\n");
	return inthroughput[slot];
}

long SIHAGetOutThroughput(int slot){
	//INFO("\n");
	return outthroughput[slot];
}

Buffers_t* SIHAGetBuffers(){
	//INFO("\n");
	return buffers;
}

plDevices_t* SIHAGetPLDevices(){
	//INFO("\n");
	return pldevices;
}

int SIHAInitAccel(int slot, char * accel){
	int status;
	_unused(accel);
	slotNum[slot] = load_accelerator(accel);
	if(slotNum[slot] == -1){
		printf("Slot not allocated !!\n");
		return -1;
	}
	sprintf(slotStr[slot], "%d", slotNum[slot]);

	if(!pldevices) pldevices = (plDevices_t*) malloc(sizeof(plDevices_t));
	pldevices->AccelConfig_fd[slot] = getFD(slotNum[slot], "AccelConfig");
	if (pldevices->AccelConfig_fd[slot] < 0){
		printf("No AccelConfig dev found\n");
		return -1;
	}
	pldevices->dma_hls_fd[slot] = getFD(slotNum[slot], "rm_comm_box");
	if (pldevices->dma_hls_fd[slot] < 0){
		printf("No dma_hls dev found\n");
		return -1;
	}
	pldevices->slot[slot] = slotNum[slot];
	status = mapPlDevicesAccel(pldevices, slot);
	if (status < 0){
		INFO("mapPlDevices Failed !!\n");
		return -1;
	}
	getRMInfo();
	return 0;
}

int SIHAInit(int configSize, int MM2SSize, int S2MMSize, int slot){
	//INFO("\n");
	int status;

	pldevices = (plDevices_t*) malloc(sizeof(plDevices_t));
	buffers   = (Buffers_t*) malloc(sizeof(Buffers_t));

	buffers->config_size[slot] = configSize;
	buffers->S2MM_size[slot] = S2MMSize;
	buffers->MM2S_size[slot] = MM2SSize;

	status = initXrtBuffer(slot, buffers);
	if (status < 0) {
		return -1;
	}
	status = initPlDevices(pldevices, slot);
	if (status < 0){
		printf("initPlDevices Failed !!\n");
		return -1;
	}

	status = mapPlDevices(pldevices, slot);
	if (status < 0){
		printf("mapPlDevices Failed !!\n");
		return -1;
	}

	return 0;
}

int SIHAStartAccel(int slot){
	uint8_t* AccelConfig;
	AccelConfig = pldevices->AccelConfig[slot];
	*(uint32_t*)(AccelConfig) = 0x81;
	return 0;
}

int SIHAStopAccel(int slot){
	uint8_t* AccelConfig;
	AccelConfig = pldevices->AccelConfig[slot];
	AccelConfig[0] = 0x00;
	return 0;
}

int SIHAFinaliseAccel(int slot){
	unmapPlDevicesAccel(pldevices, slot);
	remove_accelerator(slotNum[slot]);
	getRMInfo();
	return 0;
}


int SIHAFinalise(int slot){
	int status;
	finalisePlDevices(pldevices, slot);
	status = finaliseXrtBuffer(slot, buffers);
	if (status < 0) {
		return -1;
	}

	return 0;
}

int SIHAConfig(int slot, uint32_t* config, int size, int tid){
	_unused(tid);
	memcpy(buffers->config_ptr[slot], (uint8_t*)config, size);
	printf("%lx : %lx\n", buffers->S2MM_paddr[slot], buffers->MM2S_paddr[slot]);
	return 0;
}

int SIHAExchangeBuffer(int slot, uint32_t* inbuff, uint32_t* outbuff, int insize, int outsize){
	//INFO("\n");
	struct timespec start, finish;
	double elapsedTime;
	memcpy(buffers->MM2S_ptr[slot], (uint8_t*)inbuff, insize);
	//printhex(inbuff, 0x100);
	//printhex(buffers->MM2S_ptr, 0x100);
	//printf("%d\n", slot);
	printf("%lx : %lx\n", buffers->S2MM_paddr[slot], buffers->MM2S_paddr[slot]);
	clock_gettime(CLOCK_REALTIME, &start);	

	//S2MMData(slot, buffers->S2MM_paddr[slot], outsize);
	//MM2SData(slot, buffers->MM2S_paddr[slot], insize, 0x00);
	//while(1){
	//	if(S2MMDone(slot)) {
	//		printf("S2MM Done !!\n");
	//		break;
	//	}
	//}
	clock_gettime(CLOCK_REALTIME, &finish);
	elapsedTime = (1.0*(finish.tv_nsec - start.tv_nsec))/1000000000 + finish.tv_sec - start.tv_sec;
	inthroughput[slot] = (1.0 * insize)/elapsedTime; 
	outthroughput[slot] = (1.0 * outsize)/elapsedTime; 
	//MM2SStatus(slot);
	//S2MMStatus(slot);
	//printhex(buffers->S2MM_ptr, 0x100);
	memcpy((uint8_t*)outbuff, buffers->S2MM_ptr[slot], outsize);
	return 0;
}


