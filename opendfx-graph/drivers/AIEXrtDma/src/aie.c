#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "device.h"
#include "utils.h"

int AIEXrtDma_open(DeviceConfig_t *config){
	_unused(config);
	printf("AIEXrtDma open \n");
	return 0;
}

int AIEXrtDma_close(DeviceConfig_t *config){
	_unused(config);
	printf("AIEXrtDma close \n");
	return 0;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "AIEXrtDma");
	tDevice->open = AIEXrtDma_open;
	tDevice->close = AIEXrtDma_close;
	*device = tDevice;
	*config = tConfig;
	printf("AIEXrtDma driver registered \n");
	return 0;
}

int unregisterDriver(Device_t** device, DeviceConfig_t** config){
	free(*device);
	free(*config);
	printf("AIEXrtDma driver unregistered \n");
	return 0;
}
