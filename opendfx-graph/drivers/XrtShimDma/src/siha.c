#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "device.h"
#include "utils.h"

int sihaXrtDma_open(DeviceConfig_t *config){
	_unused(config);
	printf("sihaXrtDma open \n");
	return 0;
}

int sihaXrtDma_close(DeviceConfig_t *config){
	_unused(config);
	printf("sihaXrtDma close \n");
	return 0;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "sihaXrtDma");
	tDevice->open = sihaXrtDma_open;
	tDevice->close = sihaXrtDma_close;
	*device = tDevice;
	*config = tConfig;
	printf("sihaXrtDma driver registered \n");
	return 0;
}

int unregisterDriver(Device_t** device, DeviceConfig_t** config){
	free(*device);
	free(*config);
	printf("sihaXrtDma driver unregistered \n");
	return 0;
}
