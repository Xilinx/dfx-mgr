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
#include "sihaXrt.h"

#include <dfx-mgr/daemon_helper.h>

int sihaXrtDma_open(DeviceConfig_t *config){
    sihaXrtDmaConfig_t *sihaCfg = (sihaXrtDmaConfig_t*) malloc(sizeof(sihaXrtDmaConfig_t));
    config->privConfig = sihaCfg;
    
	//strcpy(s2mm_kname, "dms2mm0:dms2mm0_1");//, strlen("dms2mm0:dms2mm0_1");
	//strcpy(mm2s_kname, "dmmm2s0:dmmm2s0_1");//, strlen("dmmm2s0:dmmm2s0_1");
    sihaCfg->s2mm_kernel = xrtPLKernelOpen(sihaCfg->device, sihaCfg->uuid, "dms2mm0:dms2mm0_1"); //sihaCfg->s2mm_kname);
  	sihaCfg->mm2s_kernel = xrtPLKernelOpen(sihaCfg->device, sihaCfg->uuid, "dmmm2s0:dmmm2s0_1"); //sihaCfg->mm2s_kname);
	printf("sihaXrtDma open \n");
	return 0;
}

int sihaXrtDma_close(DeviceConfig_t *config){
    sihaXrtDmaConfig_t *sihaCfg = (sihaXrtDmaConfig_t*) config->privConfig;
    xrtKernelClose(sihaCfg->s2mm_kernel);
    xrtKernelClose(sihaCfg->mm2s_kernel);
	printf("sihaXrtDma close \n");
	return 0;
}


int sihaXrtDma_MM2SStatus(DeviceConfig_t *config){
    sihaXrtDmaConfig_t *sihaCfg = (sihaXrtDmaConfig_t*) config->privConfig;
	//uint32_t status;
	_unused(sihaCfg);
	return 0;
}

int sihaXrtDma_S2MMStatus(DeviceConfig_t *config){
    sihaXrtDmaConfig_t *sihaCfg = (sihaXrtDmaConfig_t*) config->privConfig;
	//uint32_t status;
	_unused(sihaCfg);
	return 0;
}

int sihaXrtDma_MM2SData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid){
    sihaXrtDmaConfig_t *sihaCfg = (sihaXrtDmaConfig_t*) config->privConfig;
	acapd_debug("sihaXrtDma MM2S\n");
	sihaCfg->mm2s_Run = xrtKernelRun(sihaCfg->s2mm_kernel, buffer->xrtBuffer, size, tid);
	_unused(offset);
	_unused(firstLast);
	return 0;
}

int sihaXrtDma_S2MMData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast){
    sihaXrtDmaConfig_t *sihaCfg = (sihaXrtDmaConfig_t*) config->privConfig;
	acapd_debug("sihaXrtDma S2MM\n");
	sihaCfg->s2mm_Run = xrtKernelRun(sihaCfg->s2mm_kernel, buffer->xrtBuffer, size);
	_unused(offset);
	_unused(firstLast);
	return 0;
}

int sihaXrtDma_S2MMDone(DeviceConfig_t *config){
    sihaXrtDmaConfig_t *sihaCfg = (sihaXrtDmaConfig_t*) config->privConfig;
	enum ert_cmd_state status;
	int res = 0;
	status = xrtRunWaitFor(sihaCfg->s2mm_Run, 0);
	//status = *(uint32_t*)(uint32_t*)(sihaCfg->s2mm_APCR);
	//res = status ? ((status >> AP_DONE) | (status >> AP_IDLE)) & 0x1 : 1;
	_unused(status);
	_unused(res);
	return res;
}

int sihaXrtDma_MM2SDone(DeviceConfig_t *config){
    sihaXrtDmaConfig_t *sihaCfg = (sihaXrtDmaConfig_t*) config->privConfig;
	enum ert_cmd_state status;
	int res = 0;
	status = xrtRunWaitFor(sihaCfg->mm2s_Run, 0);
	//status = *(volatile uint32_t*)(uint32_t*)(sihaCfg->mm2s_APCR);
	//res = status ? ((status >> AP_DONE) | (status >> AP_IDLE)) & 0x1 : 1;
	_unused(status);
	_unused(res);
	return res;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "sihaXrtDma");
	tDevice->open       = sihaXrtDma_open;
	tDevice->close      = sihaXrtDma_close;
	tDevice->MM2SStatus = sihaXrtDma_MM2SStatus;
	tDevice->S2MMStatus = sihaXrtDma_S2MMStatus;
	tDevice->MM2SData   = sihaXrtDma_MM2SData;
	tDevice->S2MMData   = sihaXrtDma_S2MMData;
	tDevice->MM2SDone   = sihaXrtDma_MM2SDone;
	tDevice->S2MMDone   = sihaXrtDma_S2MMDone;
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
