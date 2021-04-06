/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fallback.h"
#include "../utils.h"
#include "../dm.h"
#include "../xrtbuffer.h"
#include "../uio.h"
#include "../debug.h"

int fallback_config(void* dmconfig_a, Accel_t *accel){
	fallback_DMConfig_t* dmconfig = (fallback_DMConfig_t*)dmconfig_a;
	_unused(dmconfig);
	_unused(accel);
	//INFO("\n");
	//dmconfig->data = base;
	return 0;
}
	
	
/*int fallback_config(void* dmconfig_a, uint8_t InputChannelCount, uint8_t OutputChannelCount){
	fallback_DMConfig_t* dmconfig = (fallback_DMConfig_t*)dmconfig_a;
	INFO("\n");
	dmconfig->InputChannelReq[0]  = 0;
	dmconfig->InputChannelCount   = InputChannelCount;
	dmconfig->OutputChannelReq[0] = 0;
	dmconfig->OutputChannelCount  = OutputChannelCount;
	return 0;
}*/

int fallback_MM2SStatus(void* dmconfig_a){
	_unused(dmconfig_a);
	//INFO("\n");
	return 0;
}

int fallback_S2MMStatus(void* dmconfig_a){
	_unused(dmconfig_a);
	//INFO("\n");
	return 0;
}

int fallback_MM2SData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid){
	//INFO("\n");
	_unused(firstLast);
	_unused(offset);
	fallback_DMConfig_t* dmconfig = (fallback_DMConfig_t*)dmconfig_a;
	dmconfig->InputChannelReq[tid] = data->ptr;
	dmconfig->InputChannelSize[tid] = size;
	//printf("MM2SData %d : %p %lx\n", tid, data->ptr, data->phyAddr);
	//printf("%p : %p \n", dmconfig->InputChannelReq[0], dmconfig->OutputChannelReq[0]);
	if(dmconfig->InputChannelReq[0] != NULL && dmconfig->OutputChannelReq[0] != NULL){
		if(dmconfig->fallbackfunction != NULL){
			dmconfig->fallbackfunction((void**)dmconfig->InputChannelReq, dmconfig->InputChannelSize,
							(void**)dmconfig->OutputChannelReq, dmconfig->OutputChannelSize);
		}
		dmconfig->MM2Sstatus = 1;
		dmconfig->S2MMstatus = 1;
	}
	if(tid != 0){
		dmconfig->MM2Sstatus = 1;
	}
	return 0;
}

int fallback_S2MMData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast){
	//INFO("\n");
	_unused(offset);
	_unused(firstLast);
	fallback_DMConfig_t* dmconfig = (fallback_DMConfig_t*)dmconfig_a;
	dmconfig->OutputChannelReq[0] = data->ptr;
	dmconfig->OutputChannelSize[0] = size;
	//printf("S2MMData %p %lx\n", data->ptr, data->phyAddr);
	//printf("%p : %p \n", dmconfig->InputChannelReq[0], dmconfig->OutputChannelReq[0]);
	if(dmconfig->InputChannelReq[0] != NULL && dmconfig->OutputChannelReq[0] != NULL){
		if(dmconfig->fallbackfunction != NULL){
			dmconfig->fallbackfunction((void**)dmconfig->InputChannelReq, dmconfig->InputChannelSize,
							(void**)dmconfig->OutputChannelReq, dmconfig->OutputChannelSize);
		}
		dmconfig->S2MMstatus = 1;
		dmconfig->MM2Sstatus = 1;
	}
	return 0;
}

int fallback_S2MMDone(void* dmconfig_a, Buffer_t* data){
	//INFO("\n");
	fallback_DMConfig_t* dmconfig = (fallback_DMConfig_t*)dmconfig_a;
	_unused(data);
	int status = dmconfig->S2MMstatus;
	dmconfig->S2MMstatus = 0;
	return status;
}

int fallback_MM2SDone(void* dmconfig_a, Buffer_t* data){
	//INFO("\n");
	fallback_DMConfig_t* dmconfig = (fallback_DMConfig_t*)dmconfig_a;
	_unused(data);
	int status = dmconfig->MM2Sstatus;
	dmconfig->MM2Sstatus = 0;
	return status;
}

int fallback_MM2SAck(void* dmconfig_a){
	_unused(dmconfig_a);
	//INFO("\n");
	return 0;
}

int fallback_S2MMAck(void* dmconfig_a){
	//INFO("\n");
	_unused(dmconfig_a);
	return 0;
}

int fallback_register(dm_t *datamover, uint8_t InputChannelCount, uint8_t OutputChannelCount, FALLBACKFUNCTION fallbackfunction){
	//INFO("\n");
	datamover->dmstruct = (void*) malloc(sizeof(fallback_DMConfig_t));
        datamover->config     = fallback_config;
        datamover->S2MMStatus = fallback_S2MMStatus;
        datamover->S2MMData   = fallback_S2MMData;
        datamover->S2MMDone   = fallback_S2MMDone;
        datamover->MM2SStatus = fallback_MM2SStatus;
        datamover->MM2SData   = fallback_MM2SData;
        datamover->MM2SDone   = fallback_MM2SDone;
	fallback_DMConfig_t* dmconfig = (fallback_DMConfig_t*)datamover->dmstruct;
	dmconfig->InputChannelCount = InputChannelCount;
	dmconfig->OutputChannelCount = OutputChannelCount;
	dmconfig->InputChannelReq[0] = NULL;
	dmconfig->InputChannelReq[1] = NULL;
	dmconfig->InputChannelReq[2] = NULL;
	dmconfig->InputChannelReq[3] = NULL;
	dmconfig->InputChannelReq[4] = NULL;
	dmconfig->InputChannelReq[5] = NULL;
	dmconfig->InputChannelReq[6] = NULL;
	dmconfig->InputChannelReq[7] = NULL;
	dmconfig->InputChannelReq[8] = NULL;
	dmconfig->InputChannelReq[9] = NULL;
	dmconfig->OutputChannelReq[0] = NULL;
	dmconfig->fallbackfunction = fallbackfunction;
	dmconfig->S2MMstatus = 0;
	dmconfig->MM2Sstatus = 0;
	return 0;
}

int fallback_unregister(dm_t *datamover){
	INFO("\n");
	free(datamover->dmstruct);
	return 0;
}
