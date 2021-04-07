/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "sihaHLSdm.h"
#include "../utils.h"
#include "../dm.h"
#include "../xrtbuffer.h"
#include "../uio.h"
#include <dfx-mgr/print.h>
#include <dfx-mgr/assert.h>

int sihahls_config(void* dmconfig_a, Accel_t *accel){ //volatile uint8_t* base){
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	//INFO("\n");
	dmconfig->s2mm_baseAddr  = accel->dma_hls + S2MM;
        dmconfig->s2mm_APCR      = dmconfig->s2mm_baseAddr + APCR;
        dmconfig->s2mm_GIER      = dmconfig->s2mm_baseAddr + GIER; 
        dmconfig->s2mm_IIER      = dmconfig->s2mm_baseAddr + IIER;
        dmconfig->s2mm_IISR      = dmconfig->s2mm_baseAddr + IISR;
        dmconfig->s2mm_ADDR_LOW  = dmconfig->s2mm_baseAddr + ADDR_LOW;
        dmconfig->s2mm_ADDR_HIGH = dmconfig->s2mm_baseAddr + ADDR_HIGH;
        dmconfig->s2mm_SIZE_LOW  = dmconfig->s2mm_baseAddr + SIZE_LOW;
        dmconfig->s2mm_SIZE_HIGH = dmconfig->s2mm_baseAddr + SIZE_HIGH;
        dmconfig->s2mm_TID       = dmconfig->s2mm_baseAddr + TID;
	dmconfig->mm2s_baseAddr  = accel->dma_hls + MM2S;
        dmconfig->mm2s_APCR      = dmconfig->mm2s_baseAddr + APCR;
        dmconfig->mm2s_GIER      = dmconfig->mm2s_baseAddr + GIER; 
        dmconfig->mm2s_IIER      = dmconfig->mm2s_baseAddr + IIER;
        dmconfig->mm2s_IISR      = dmconfig->mm2s_baseAddr + IISR;
        dmconfig->mm2s_ADDR_LOW  = dmconfig->mm2s_baseAddr + ADDR_LOW;
        dmconfig->mm2s_ADDR_HIGH = dmconfig->mm2s_baseAddr + ADDR_HIGH;
        dmconfig->mm2s_SIZE_LOW  = dmconfig->mm2s_baseAddr + SIZE_LOW;
        dmconfig->mm2s_SIZE_HIGH = dmconfig->mm2s_baseAddr + SIZE_HIGH;
        dmconfig->mm2s_TID       = dmconfig->mm2s_baseAddr + TID;
	dmconfig->accel		 = accel;
	dmconfig->start          = 0;
	return 0;
}

int sihahls_start(void* dmconfig_a){
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	//INFO("\n");
	//return 0;
	*(uint32_t*)(dmconfig->accel->AccelConfig) = 0x81;
        switch (dmconfig->accel->slot){
		case 0:
 	               //*(uint32_t*)(dmconfig->accel->dma_hls + 0x0) = 0x00000000;
                       //*(uint32_t*)(dmconfig->accel->dma_hls + 0x4) = 0x00000002;
                       //*(uint32_t*)(dmconfig->accel->dma_hls + 0x8) = 0x3fffffff;
                      // *(uint32_t*)(dmconfig->accel->dma_hls + 0xC) = 0x00000002;
                       break;
                case 1:
                       //*(uint32_t*)(dmconfig->accel->dma_hls + 0x0) = 0x80000000;
                      // *(uint32_t*)(dmconfig->accel->dma_hls + 0x4) = 0x00000002;
                      // *(uint32_t*)(dmconfig->accel->dma_hls + 0x8) = 0xbfffffff;
                       //*(uint32_t*)(dmconfig->accel->dma_hls + 0xC) = 0x00000002;
                       break;
                case 2:
                       //*(uint32_t*)(dmconfig->accel->dma_hls + 0x0) = 0x00000000;
                       //*(uint32_t*)(dmconfig->accel->dma_hls + 0x4) = 0x00000003;
                       //*(uint32_t*)(dmconfig->accel->dma_hls + 0x8) = 0x3fffffff;
                       //*(uint32_t*)(dmconfig->accel->dma_hls + 0xC) = 0x00000003;
                       break;
	}
	dmconfig->start = 1;
	return 0;
}

int sihahls_MM2SStatus(void* dmconfig_a){
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	uint32_t status;
	//printf("%llx\n", (long long int)ACCELCONFIG[slot]);
	INFO("\n");
	asm volatile("dmb ishld" ::: "memory");
	status = *(uint32_t*)(dmconfig->mm2s_APCR);
	INFOP("################# MM2S %p ls ##############\n", (uint32_t*)(dmconfig->mm2s_baseAddr));
	INFOP("ap_start     = %x \n", (status >> AP_START) & 0x1);
	INFOP("ap_done      = %x \n", (status >> AP_DONE) & 0x1);
	INFOP("ap_idle      = %x \n", (status >> AP_IDLE) & 0x1);
	INFOP("ap_ready     = %x \n", (status >> AP_READY) & 0x1);
	INFOP("auto_restart = %x \n", (status >> AP_AUTORESTART) & 0x1);
	status = *(uint32_t*)(dmconfig->mm2s_GIER);
	INFOP("GIER = %x \n", status);
	status = *(uint32_t*)(dmconfig->mm2s_IIER);
	INFOP("IIER = %x \n", status);
	status = *(uint32_t*)(dmconfig->mm2s_IISR);
	INFOP("IISR = %x \n", status);
	INFOP("@ %x ADDR_LOW  = %x \n", MM2S + ADDR_LOW,  *(uint32_t*)(dmconfig->mm2s_ADDR_LOW));
	INFOP("@ %x ADDR_HIGH = %x \n", MM2S + ADDR_HIGH, *(uint32_t*)(dmconfig->mm2s_ADDR_HIGH));
	INFOP("@ %x SIZE_LOW  = %x \n", MM2S + SIZE_LOW,  *(uint32_t*)(dmconfig->mm2s_SIZE_LOW));
	INFOP("@ %x SIZE_HIGH = %x \n", MM2S + SIZE_HIGH, *(uint32_t*)(dmconfig->mm2s_SIZE_HIGH));
	INFOP("@ %x TID       = %x \n", MM2S + TID,       *(uint32_t*)(dmconfig->mm2s_TID));
	INFOP("########################################\n");
	asm volatile("dmb ishld" ::: "memory");
	return 0;
}

int sihahls_S2MMStatus(void* dmconfig_a){
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	uint32_t status;
	INFO("\n");
	asm volatile("dmb ishld" ::: "memory"); 
	status = *(uint32_t*)(dmconfig->s2mm_APCR);
	INFOP("################# S2MM %p ls ###############\n", (uint32_t*)(dmconfig->s2mm_baseAddr));
	INFOP("ap_start     = %x \n", (status >> AP_START) & 0x1);
	INFOP("ap_done      = %x \n", (status >> AP_DONE) & 0x1);
	INFOP("ap_idle      = %x \n", (status >> AP_IDLE) & 0x1);
	INFOP("ap_ready     = %x \n", (status >> AP_READY) & 0x1);
	INFOP("auto_restart = %x \n", (status >> AP_AUTORESTART) & 0x1);
	status = *(uint32_t*)(dmconfig->s2mm_GIER);
	INFOP("GIER = %x \n", status);
	status = *(uint32_t*)(dmconfig->s2mm_IIER);
	INFOP("IIER = %x \n", status);
	status = *(uint32_t*)(dmconfig->s2mm_IISR);
	INFOP("IISR = %x \n", status);
	INFOP("@ %x ADDR_LOW  = %x \n", S2MM + ADDR_LOW,  *(uint32_t*)(dmconfig->s2mm_ADDR_LOW));
	INFOP("@ %x ADDR_HIGH = %x \n", S2MM + ADDR_HIGH, *(uint32_t*)(dmconfig->s2mm_ADDR_HIGH));
	INFOP("@ %x SIZE_LOW  = %x \n", S2MM + SIZE_LOW,  *(uint32_t*)(dmconfig->s2mm_SIZE_LOW));
	INFOP("@ %x SIZE_HIGH = %x \n", S2MM + SIZE_HIGH, *(uint32_t*)(dmconfig->s2mm_SIZE_HIGH));
	INFOP("@ %x TID       = %x \n", S2MM + TID,       *(uint32_t*)(dmconfig->s2mm_TID));
	INFOP("########################################\n");
	asm volatile("dmb ishld" ::: "memory");
	return 0;
}

int sihahls_MM2SData(void* dmconfig_a, uint64_t data, uint64_t size, uint8_t tid){
	INFO("\n");
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	if (dmconfig->start == 0){
		sihahls_start(dmconfig_a);
	}
	*(uint32_t*)(dmconfig->mm2s_ADDR_LOW)  = data & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->mm2s_ADDR_HIGH) = (data >> 32) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->mm2s_SIZE_LOW)  = (size >> 4) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->mm2s_SIZE_HIGH) = (size >> 36) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->mm2s_TID)       = tid;
	*(uint32_t*)(dmconfig->mm2s_GIER)      = 0x1;
	*(uint32_t*)(dmconfig->mm2s_IIER)      = 0x3;
	*(uint32_t*)(dmconfig->mm2s_APCR)      = 0x1 << AP_START;
	return 0;
}

int sihahls_S2MMData(void* dmconfig_a, uint64_t data, uint64_t size){
	INFO("\n");
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	if (dmconfig->start == 0){
		sihahls_start(dmconfig_a);
	}
	*(uint32_t*)(dmconfig->s2mm_ADDR_LOW)  = data & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->s2mm_ADDR_HIGH) = (data >> 32) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->s2mm_SIZE_LOW)  = (size >> 4) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->s2mm_SIZE_HIGH) = (size >> 36) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->s2mm_GIER)      = 0x1;
	*(uint32_t*)(dmconfig->s2mm_IIER)      = 0x3;
	*(uint32_t*)(dmconfig->s2mm_APCR)      = 0x1 << AP_START;
	return 0;
}

int sihahls_MM2SData_B(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid){
	_unused(firstLast);
	//INFO("%x %d\n", size, tid);
	//INFO("%p\n", data->ptr);
        //printhex((uint32_t*)((uint8_t*)data->ptr + offset), size);
	//INFO("%x, %x\n", offset, size);
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	//INFO("%d \n", dmconfig->start);
	if (dmconfig->start == 0){
		sihahls_start(dmconfig_a);
	}
	uint64_t address = data->phyAddr + offset ;
	if(data->InterRMCompatible == 2){
		printf("InterRMCompatible");
		*(uint32_t*)(dmconfig->mm2s_ADDR_LOW)  = data->otherAccel_phyAddr[dmconfig->accel->slot] & 0xFFFFFFFF;
		*(uint32_t*)(dmconfig->mm2s_ADDR_HIGH) = (data->otherAccel_phyAddr[dmconfig->accel->slot] >> 32) & 0xFFFFFFFF;
	}
	else{
		*(uint32_t*)(dmconfig->mm2s_ADDR_LOW)  = address & 0xFFFFFFFF;
		*(uint32_t*)(dmconfig->mm2s_ADDR_HIGH) = (address >> 32) & 0xFFFFFFFF;
	}
	*(uint32_t*)(dmconfig->mm2s_SIZE_LOW)  = (size >> 4) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->mm2s_SIZE_HIGH) = (size >> 36) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->mm2s_TID)       = tid;
	*(uint32_t*)(dmconfig->mm2s_GIER)      = 0x1;
	*(uint32_t*)(dmconfig->mm2s_IIER)      = 0x3;
	*(uint32_t*)(dmconfig->mm2s_APCR)      = 0x1 << AP_START;
	return 0;
}

int sihahls_S2MMData_B(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast){
	_unused(firstLast);
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	if (dmconfig->start == 0){
		sihahls_start(dmconfig_a);
	}
	uint64_t address = data->phyAddr + offset ;
	if(data->InterRMCompatible == 2){
		printf("InterRMCompatible");
		*(uint32_t*)(dmconfig->s2mm_ADDR_LOW)  = data->otherAccel_phyAddr[data->sincSlot] & 0xFFFFFFFF;
		*(uint32_t*)(dmconfig->s2mm_ADDR_HIGH) = (data->otherAccel_phyAddr[data->sincSlot] >> 32) & 0xFFFFFFFF;
	}
	else{
		*(uint32_t*)(dmconfig->s2mm_ADDR_LOW)  = address & 0xFFFFFFFF;
		*(uint32_t*)(dmconfig->s2mm_ADDR_HIGH) = (address >> 32) & 0xFFFFFFFF;
	}
	*(uint32_t*)(dmconfig->s2mm_SIZE_LOW)  = (size >> 4) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->s2mm_SIZE_HIGH) = (size >> 36) & 0xFFFFFFFF;
	*(uint32_t*)(dmconfig->s2mm_GIER)      = 0x1;
	*(uint32_t*)(dmconfig->s2mm_IIER)      = 0x3;
	*(uint32_t*)(dmconfig->s2mm_APCR)      = 0x1 << AP_START;
	return 0;
}

int sihahls_S2MMDone(void* dmconfig_a, Buffer_t* data){
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	int status, res;
	status = *(uint32_t*)(uint32_t*)(dmconfig->s2mm_APCR);
	res = ((status >> AP_DONE) | (status >> AP_IDLE)) & 0x1;
	//INFO("%x, %x\n", res, status);
	//INFO("\n");
	if(res){
		data->writeStatus += 1;
	}
	return res;
}

int sihahls_MM2SDone(void* dmconfig_a, Buffer_t* data){
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	int status, res;
	status = *(uint32_t*)(uint32_t*)(dmconfig->mm2s_APCR);
	res = ((status >> AP_DONE) | (status >> AP_IDLE)) & 0x1;
	//INFO("%x, %x\n", res, status);
	if(res){
		data->readStatus += 1;
	        if(data->readStatus == 2*data->readerCount){
        	        data->readStatus = 0;
                	data->writeStatus = 0;
        	}
	}
	return res;
}

int sihahls_MM2SAck(void* dmconfig_a){
	//INFO("\n");
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	int status;
	status = *(uint32_t*)(dmconfig->mm2s_IISR);
	*(uint32_t*)(dmconfig->mm2s_IISR) = status;
	return 0;
}

int sihahls_S2MMAck(void* dmconfig_a){
	//INFO("\n");
	sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)dmconfig_a;
	int status;
	status = *(uint32_t*)(dmconfig->s2mm_IISR);
	*(uint32_t*)(dmconfig->s2mm_IISR) = status;
	return 0;
}

int sihahls_register(dm_t *datamover){
	//INFO("\n");
	datamover->dmstruct = (void*) malloc(sizeof(sihahls_DMConfig_t));
        datamover->config     = sihahls_config;
        datamover->S2MMStatus = sihahls_S2MMStatus;
        datamover->S2MMData   = sihahls_S2MMData_B;
        datamover->S2MMDone   = sihahls_S2MMDone;
        datamover->MM2SStatus = sihahls_MM2SStatus;
        datamover->MM2SData   = sihahls_MM2SData_B;
        datamover->MM2SDone   = sihahls_MM2SDone;
	return 0;
}

int sihahls_unregister(dm_t *datamover){
	//INFO("\n");
	free(datamover->dmstruct);
	return 0;
}
