#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "device.h"
#include "utils.h"
#include "siha.h"

#include <dfx-mgr/daemon_helper.h>

int sihaDma_open(DeviceConfig_t *config){
    sihaDmaConfig_t *sihaCfg = (sihaDmaConfig_t*) malloc(sizeof(sihaDmaConfig_t));
    config->privConfig = sihaCfg;
    sihaCfg->AccelConfig_fd = getFD(config->slot, "AccelConfig");
    if (sihaCfg->AccelConfig_fd < 0){
    	printf("No AccelConfig dev found\n");
        return -1;
    }
    sihaCfg->dma_hls_fd = getFD(config->slot, "");
    if (sihaCfg->dma_hls_fd < 0){
    	printf("No dma_hls dev found\n");
        return -1;
    }
           

	
	printf("sihaDma open \n");
	return 0;
}

int sihaDma_close(DeviceConfig_t *config){
	_unused(config);
	printf("sihaDma close \n");
	return 0;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "sihaDma");
	tDevice->open = sihaDma_open;
	tDevice->close = sihaDma_close;
	*device = tDevice;
	*config = tConfig;
	printf("sihaDma driver registered \n");
	return 0;
}

int unregisterDriver(Device_t** device, DeviceConfig_t** config){
	free(*device);
	free(*config);
	printf("sihaDma driver unregistered \n");
	return 0;
}
