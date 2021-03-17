/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "softdm.h"
#include "../utils.h"
#include "../dm.h"
#include "../xrtbuffer.h"
#include "../uio.h"
#include "../debug.h"


int soft_config(void* dmconfig_a, Accel_t *accel){ //, volatile uint8_t* base){
	soft_DMConfig_t* dmconfig = (soft_DMConfig_t*)dmconfig_a;
	//INFO("%p\n", accel->semptr);
	
	dmconfig->data = accel->softBuffer;
	dmconfig->semptr = accel->semptr;
	//INFO("%p\n", dmconfig->semptr);
	return 0;
}

int soft_MM2SStatus(void* dmconfig_a){
	//INFO("\n");
	_unused(dmconfig_a);
	return 0;
}

int soft_S2MMStatus(void* dmconfig_a){
	//INFO("\n");
	_unused(dmconfig_a);
	return 0;
}

int soft_MM2SData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid){
	_unused(tid);
	soft_DMConfig_t* dmconfig = (soft_DMConfig_t*)dmconfig_a;
	memcpy((uint8_t*)dmconfig->data, ((uint8_t*)data->ptr) + offset, size);
	if(firstLast){
        	sem_post(dmconfig->semptr);
		//INFO("######## Soft MM2SData last #############");
	}
	return 0;
}

int soft_S2MMData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast){
	soft_DMConfig_t* dmconfig = (soft_DMConfig_t*)dmconfig_a;
	if(firstLast){
		//int val;
		//sem_getvalue (dmconfig->semptr, &val);
		//INFO("######## Soft S2MMData first #############");
		sem_wait(dmconfig->semptr);
	}
	memcpy((uint8_t*)data->ptr + offset, (uint8_t*)dmconfig->data, size);
	return 0;
}

int soft_S2MMDone(void* dmconfig_a, Buffer_t* data){
	_unused(dmconfig_a);
	_unused(data);
	return 1;
}

int soft_MM2SDone(void* dmconfig_a, Buffer_t* data){
	_unused(dmconfig_a);
	_unused(data);
	return 1;
}

int soft_MM2SAck(void* dmconfig_a){
	//INFO("\n");
	_unused(dmconfig_a);
	return 0;
}

int soft_S2MMAck(void* dmconfig_a){
	//INFO("\n");
	_unused(dmconfig_a);
	return 0;
}

int soft_register(dm_t *datamover){
	//INFO("\n");
	datamover->dmstruct = (void*) malloc(sizeof(soft_DMConfig_t));
        datamover->config     = soft_config;
        datamover->S2MMStatus = soft_S2MMStatus;
        datamover->S2MMData   = soft_S2MMData;
        datamover->S2MMDone   = soft_S2MMDone;
        datamover->MM2SStatus = soft_MM2SStatus;
        datamover->MM2SData   = soft_MM2SData;
        datamover->MM2SDone   = soft_MM2SDone;
	return 0;
}

int soft_unregister(dm_t *datamover){
	//INFO("\n");
	free(datamover->dmstruct);
	return 0;
}
