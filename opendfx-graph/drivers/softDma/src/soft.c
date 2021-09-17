#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "device.h"
#include "utils.h"
#include "soft.h"

#include <dfx-mgr/daemon_helper.h>

int softDma_open(DeviceConfig_t *config){
    softDmaConfig_t *softCfg = (softDmaConfig_t*) malloc(sizeof(softDmaConfig_t));
    config->privConfig = softCfg;
	
	printf("softDma open \n");
	return 0;
}

int softDma_close(DeviceConfig_t *config){
	_unused(config);
	printf("softDma close \n");
	return 0;
}


int softDma_MM2SStatus(DeviceConfig_t *config){
    softDmaConfig_t *softCfg = (softDmaConfig_t*) config->privConfig;
    _unused(softCfg);
	return 0;
}

int softDma_S2MMStatus(DeviceConfig_t *config){
    softDmaConfig_t *softCfg = (softDmaConfig_t*) config->privConfig;
    _unused(softCfg);
	return 0;
}

int softDma_MM2SData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid){
    softDmaConfig_t *softCfg = (softDmaConfig_t*) config->privConfig;
	_unused(softCfg);
    _unused(firstLast);
    _unused(tid);
	memcpy((uint8_t*)config->ptr, ((uint8_t*)buffer->ptr) + offset, size);
	acapd_debug("Soft MM2S\n");
	if(firstLast){
		sem_post(config->semptr);
	    INFO("######## Soft MM2SData last #############");
	}
	
	return 0;
}

int softDma_S2MMData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast){
    softDmaConfig_t *softCfg = (softDmaConfig_t*) config->privConfig;
	_unused(softCfg);
    _unused(firstLast);
	int status;
	if(firstLast){
		status = sem_trywait(config->semptr);
		//printf("softDma_S2MMData : %d\n", status);
		if(status < 0){
			return -1;
		}
	}
	memcpy(((uint8_t*)buffer->ptr) + offset, (uint8_t*)config->ptr, size);
	acapd_debug("Soft S2MM\n");
	return 0;
}

int softDma_S2MMDone(DeviceConfig_t *config){
    softDmaConfig_t *softCfg = (softDmaConfig_t*) config->privConfig;
	_unused(softCfg);
	return 1;
}

int softDma_MM2SDone(DeviceConfig_t *config){
    softDmaConfig_t *softCfg = (softDmaConfig_t*) config->privConfig;
	_unused(softCfg);
	return 1;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "softDma");
	tDevice->open       = softDma_open;
	tDevice->close      = softDma_close;
	tDevice->MM2SStatus = softDma_MM2SStatus;
	tDevice->S2MMStatus = softDma_S2MMStatus;
	tDevice->MM2SData   = softDma_MM2SData;
	tDevice->S2MMData   = softDma_S2MMData;
	tDevice->MM2SDone   = softDma_MM2SDone;
	tDevice->S2MMDone   = softDma_S2MMDone;
	*device = tDevice;
	*config = tConfig;
	printf("softDma driver registered \n");
	return 0;
}

int unregisterDriver(Device_t** device, DeviceConfig_t** config){
	free(*device);
	free(*config);
	printf("softDma driver unregistered \n");
	return 0;
}
