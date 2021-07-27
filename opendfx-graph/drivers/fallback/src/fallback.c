#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "device.h"

//int fallback_open(DeviceConfig_t *config){
int fallback_open(void){
	printf("Fallback open \n");
	return 0;
}

int fallback_close(DeviceConfig_t *config){
	printf("Fallback close \n");
	return 0;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "fallback");
	tDevice->open = fallback_open;
	tDevice->close = fallback_close;
	tDevice->open();
	*device = tDevice;
	*config = tConfig;
	printf("Fallback driver registered \n");
	return 0;
}

