#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "softdm.h"
#include "utils.h"
#include "dm.h"
#include "xrtbuffer.h"
#include "uio.h"
#include "debug.h"


int soft_config(void* dmconfig_a, Accel_t *accel){ //, volatile uint8_t* base){
	soft_DMConfig_t* dmconfig = (soft_DMConfig_t*)dmconfig_a;
	//INFO("\n");
	dmconfig->data = accel->softBuffer;
	return 0;
}

int soft_MM2SStatus(void* dmconfig_a){
	//INFO("\n");
	return 0;
}

int soft_S2MMStatus(void* dmconfig_a){
	//INFO("\n");
	return 0;
}

int soft_MM2SData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t tid){
	//INFO("\n");
	//INFO("%p\n", data->ptr);
	//INFO("%p\n", (uint32_t*)((uint8_t*)data->ptr + offset));
	soft_DMConfig_t* dmconfig = (soft_DMConfig_t*)dmconfig_a;
	memcpy((uint8_t*)dmconfig->data, ((uint8_t*)data->ptr) + offset, size);
        //printhex((uint32_t*)dmconfig->data, size);
        //printhex((uint32_t*)((uint8_t*)data->ptr + offset), size);
	return 0;
}

int soft_S2MMData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size){
	//INFO("\n");
	//INFO("%p\n", data->ptr);
	//INFO("%p\n", (uint32_t*)((uint8_t*)data->ptr + offset));
	soft_DMConfig_t* dmconfig = (soft_DMConfig_t*)dmconfig_a;
	memcpy((uint8_t*)data->ptr + offset, (uint8_t*)dmconfig->data, size);
        //printhex((uint32_t*)dmconfig->data, size);
        //printhex((uint32_t*)((uint8_t*)data->ptr + offset), size);
	return 0;
}

int soft_S2MMDone(void* dmconfig_a, Buffer_t* data){
	//INFO("\n");
	//INFO("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	return 1;
}

int soft_MM2SDone(void* dmconfig_a, Buffer_t* data){
	//INFO("\n");
	//INFO("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	return 1;
}

int soft_MM2SAck(void* dmconfig_a){
	//INFO("\n");
	return 0;
}

int soft_S2MMAck(void* dmconfig_a){
	//INFO("\n");
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
