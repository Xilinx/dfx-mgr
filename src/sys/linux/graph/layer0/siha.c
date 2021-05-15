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
#include "zynq_ioctl.h"
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
        //INFO("\n");
	//FILE *fp;
	//size_t len = 0;
	int status;
	int fd[2];
	_unused(accel);
	//ssize_t read;
	//char* line;
	//fds_t fds;
	//INFO("%s\n", accel);
	//@@//slotNum[slot] = loadpdi(accel);
	//listAccelerators();
	//INFO("loading Accel: %s\n", accel);
	//INFO("######################### Before Load Accel ###################\n");
	slotNum[slot] = load_accelerator(accel);
	//INFO("######################### After Load Accel ###################\n");
	//INFO("loadded at slot: %d\n", slotNum[slot]);
	//INFO("%s\n", accel);
	//INFO("############ %d %d ###################\n", slot, slotNum[slot]);
	//if (status < 0) return status;
	//fp = fopen("/home/root/slot.txt", "r");
	//if(fp == NULL){
	//	return -1;
	//}
	//getline(&line, &len, fp);
	//slotNum[slot] = atoi(line);
	if(slotNum[slot] == -1){
		printf("Slot not allocated !!\n");
		return -1;
	}
	sprintf(slotStr[slot], "%d", slotNum[slot]);

	status = dfx_getFDs(slotNum[slot], fd);
	if (status < 0) return status;
	//fds->accelconfig_fd = fd[0];
	//fds->dma_hls_fd = fd[1];
	//INFO("############ getFD ###################\n");
	//@@//status = socketGetFd(slotNum[slot], &fds);
	//@@//if (status < 0) return status;
        //INFO("s2mm_fd        : %d\n", fds.s2mm_fd);
        //INFO("mm2s_fd        : %d\n", fds.mm2s_fd);
        //INFO("config_fd      : %d\n", fds.config_fd);
        //INFO("accelconfig_fd : %d\n", fd[0]); //s.accelconfiig_fd);
        //INFO("dma_hls_fd     : %d\n", fd[1]); ///s.dma_hls_fd);
        //status = getPA(slotStr[slot]);
	//if (status < 0) return status;
	//INFO("############ getPA ###################\n");

        //status = socketGetPA(slotNum[slot], &fds);
	//if (status < 0) return status;
        //INFO("mm2s_pa        : %lx\n", fds.mm2s_pa);
	//INFO("mm2s_size      : %lx\n", fds.mm2s_size);
        //INFO("s2mm_pa        : %lx\n", fds.s2mm_pa);
	//INFO("s2mm_size      : %lx\n", fds.s2mm_size);
	//INFO("config_pa      : %lx\n", fds.config_pa);
	
	if(!pldevices) pldevices = (plDevices_t*) malloc(sizeof(plDevices_t));
	//if(!buffers) buffers   = (Buffers_t*) malloc(sizeof(Buffers_t));

	//@@//buffers->config_size[slot] = fds.config_size;
	//@@//buffers->S2MM_size[slot]   = fds.s2mm_size;
	//@@//buffers->MM2S_size[slot]   = fds.mm2s_size;

	//@@//buffers->config_fd[slot]   = fds.config_fd;
	//@@//buffers->S2MM_fd[slot]     = fds.s2mm_fd;
	//@@//buffers->MM2S_fd[slot]     = fds.mm2s_fd;
	
	//@@//buffers->config_paddr[slot]= fds.config_pa;
	//@@//buffers->S2MM_paddr[slot]  = fds.s2mm_pa;
	//@@//buffers->MM2S_paddr[slot]  = fds.mm2s_pa;
	
	//mapBuffer(buffers->config_fd[slot], buffers->config_size[slot], &buffers->config_ptr[slot]);
	//mapBuffer(buffers->S2MM_fd[slot],   buffers->S2MM_size[slot],   &buffers->S2MM_ptr[slot]);
	//mapBuffer(buffers->MM2S_fd[slot],   buffers->MM2S_size[slot],   &buffers->MM2S_ptr[slot]);
	//INFO("%p %d\n", buffers->config_ptr[slot], slot);
	//printBuffer(buffers, slot);
	pldevices->AccelConfig_fd[slot] = fd[0]; //s.accelconfig_fd;
	pldevices->dma_hls_fd[slot] = fd[1]; //s.dma_hls_fd;
	pldevices->slot[slot] = slotNum[slot];
	//INFO("#######################################\n");
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

	//configSihaBase(pldevices->SIHA_Manager);
	//enableSlot(slot);
	//configDMSlots(0, pldevices->dma_hls[slot]);
	/*switch(slot){
		case 0:	configDMSlots(0, pldevices->dma_hls_0);
			break;
		case 1:	configDMSlots(1, pldevices->dma_hls_1);
			break;
		case 2:	configDMSlots(2, pldevices->dma_hls_2);
			break;
	}*/

	return 0;
}

int SIHAStartAccel(int slot){
       // INFO("\n");
	uint8_t* AccelConfig;
	//printf("%d\n", slot);
	AccelConfig = pldevices->AccelConfig[slot];
	/*switch(slot){
		case 0:
			AccelConfig = pldevices->AccelConfig_0;
			break;
		case 1:
			AccelConfig = pldevices->AccelConfig_1;
			break;
		case 2:
			AccelConfig = pldevices->AccelConfig_2;
			break;
	}*/
	//printf("%hhn\n", AccelConfig);
	*(uint32_t*)(AccelConfig) = 0x81;
	//printf("%x\n", AccelConfig[0]);
	return 0;
}

int SIHAStopAccel(int slot){
        //INFO("\n");
	uint8_t* AccelConfig;
	AccelConfig = pldevices->AccelConfig[slot];
	/*switch(slot){
		case 0:
			AccelConfig = pldevices->AccelConfig_0;
			break;
		case 1:
			AccelConfig = pldevices->AccelConfig_1;
			break;
		case 2:
			AccelConfig = pldevices->AccelConfig_2;
			break;
	}*/
	AccelConfig[0] = 0x00;
	return 0;
}

int SIHAFinaliseAccel(int slot){
        //INFO("\n");
	//int r0, r1;	
	//@@//fds_t fds;
	//@@//fds.config_fd = buffers->config_fd[slot];
	//@@//fds.s2mm_fd   = buffers->S2MM_fd[slot];
	//@@//fds.mm2s_fd   = buffers->MM2S_fd[slot];
	//@@//fds.accelconfig_fd = pldevices->AccelConfig_fd[slot];
	//@@//fds.dma_hls_fd     = pldevices->dma_hls_fd[slot];
	/*switch(slot){
		case 0:
			fds.accelconfig_fd = pldevices->AccelConfig_0_fd;
			fds.dma_hls_fd     = pldevices->dma_hls_0_fd;
			break;
		case 1:
			fds.accelconfig_fd = pldevices->AccelConfig_1_fd;
			fds.dma_hls_fd     = pldevices->dma_hls_1_fd;
			break;
		case 2:
			fds.accelconfig_fd = pldevices->AccelConfig_2_fd;
			fds.dma_hls_fd     = pldevices->dma_hls_2_fd;
			break;
	}*/
	//r0 = unmapBuffer(buffers->config_fd[slot], buffers->config_size[slot], &buffers->config_ptr[slot]);
	//r1 = unmapBuffer(buffers->S2MM_fd[slot], buffers->S2MM_size[slot], &buffers->S2MM_ptr[slot]);
	//r2 = unmapBuffer(buffers->MM2S_fd[slot], buffers->MM2S_size[slot], &buffers->MM2S_ptr[slot]);
	//munmap(buffers->config_ptr[slot], buffers->config_size[slot]);
	//munmap(buffers->S2MM_ptr[slot], buffers->S2MM_size[slot]);
	//munmap(buffers->MM2S_ptr[slot], buffers->MM2S_size[slot]);
	//printf("%d, %d, %d\n", r0, r1, r2);
	//printf("Unmapped Buffers !!\n");
	unmapPlDevicesAccel(pldevices, slot);
	//printf("Unmapped Devices !!\n");
	remove_accelerator(slotNum[slot]);
	//printf("Unmapped Devices !!\n");
	getRMInfo();
	return 0;
}


int SIHAFinalise(int slot){
        //INFO("\n");
	int status;
	finalisePlDevices(pldevices, slot);
	status = finaliseXrtBuffer(slot, buffers);
	if (status < 0) {
		return -1;
	}

	return 0;
}

int SIHAConfig(int slot, uint32_t* config, int size, int tid){
        //INFO("\n");
	_unused(tid);
	memcpy(buffers->config_ptr[slot], (uint8_t*)config, size);
	printf("%lx : %lx\n", buffers->S2MM_paddr[slot], buffers->MM2S_paddr[slot]);
	//MM2SData(slot, buffers->config_paddr[slot], size, tid);
	//while(1){
	//	if(MM2SDone(slot)) {
	//		break;
	//	}
	//}
	return 0;
}

int SIHALoopFile(char* filename){
	_unused(filename);
        //INFO("\n");
	return 0;
}

int SIHAExchangeFile(char* infile, char* outfile){
        //INFO("\n");
	_unused(infile);
	_unused(outfile);
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


