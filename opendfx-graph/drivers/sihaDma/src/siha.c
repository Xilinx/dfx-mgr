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
    sihaCfg->AccelConfig = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
			sihaCfg->AccelConfig_fd, 0);
	if(sihaCfg->AccelConfig == MAP_FAILED){
		printf("Mmap AccelConfig_%d failed !!\n", config->slot);
		return -1;
	}
    sihaCfg->dma_hls_fd = getFD(config->slot, "rm_comm_box");
    if (sihaCfg->dma_hls_fd < 0){
    	printf("No dma_hls dev found\n");
        return -1;
    }
	sihaCfg->dma_hls = (uint8_t*) mmap(0, 0x20000, PROT_READ | PROT_WRITE, MAP_SHARED, 
			sihaCfg->dma_hls_fd, 0);
	if(sihaCfg->dma_hls == MAP_FAILED){
		printf("Mmap dma_hls_%d failed !!\n", config->slot);
		return -1;
	}
	
	sihaCfg->s2mm_baseAddr  = sihaCfg->dma_hls + S2MM;
	sihaCfg->s2mm_APCR      = sihaCfg->s2mm_baseAddr + APCR;
	sihaCfg->s2mm_GIER      = sihaCfg->s2mm_baseAddr + GIER; 
	sihaCfg->s2mm_IIER      = sihaCfg->s2mm_baseAddr + IIER;
	sihaCfg->s2mm_IISR      = sihaCfg->s2mm_baseAddr + IISR;
	sihaCfg->s2mm_ADDR_LOW  = sihaCfg->s2mm_baseAddr + ADDR_LOW;
	sihaCfg->s2mm_ADDR_HIGH = sihaCfg->s2mm_baseAddr + ADDR_HIGH;
	sihaCfg->s2mm_SIZE_LOW  = sihaCfg->s2mm_baseAddr + SIZE_LOW;
	sihaCfg->s2mm_SIZE_HIGH = sihaCfg->s2mm_baseAddr + SIZE_HIGH;
	sihaCfg->s2mm_TID       = sihaCfg->s2mm_baseAddr + TID;
	sihaCfg->mm2s_baseAddr  = sihaCfg->dma_hls + MM2S;
	sihaCfg->mm2s_APCR      = sihaCfg->mm2s_baseAddr + APCR;
	sihaCfg->mm2s_GIER      = sihaCfg->mm2s_baseAddr + GIER; 
	sihaCfg->mm2s_IIER      = sihaCfg->mm2s_baseAddr + IIER;
	sihaCfg->mm2s_IISR      = sihaCfg->mm2s_baseAddr + IISR;
	sihaCfg->mm2s_ADDR_LOW  = sihaCfg->mm2s_baseAddr + ADDR_LOW;
	sihaCfg->mm2s_ADDR_HIGH = sihaCfg->mm2s_baseAddr + ADDR_HIGH;
	sihaCfg->mm2s_SIZE_LOW  = sihaCfg->mm2s_baseAddr + SIZE_LOW;
	sihaCfg->mm2s_SIZE_HIGH = sihaCfg->mm2s_baseAddr + SIZE_HIGH;
	sihaCfg->mm2s_TID       = sihaCfg->mm2s_baseAddr + TID;
	sihaCfg->start          = 0;

    printf("base Address %p\n",sihaCfg->dma_hls);
    printf("S2MM %p\n",sihaCfg->s2mm_baseAddr);
    printf("MM2S %p\n",sihaCfg->mm2s_baseAddr);
    printf("S2MM %p\n",sihaCfg->s2mm_baseAddr + APCR);
    printf("MM2S %p\n",sihaCfg->mm2s_baseAddr + APCR);

	*(uint32_t*)(sihaCfg->AccelConfig) = 0x81;
	sihaCfg->start = 1;
	printf("sihaDma open \n");
	return 0;
}

int sihaDma_close(DeviceConfig_t *config){
	_unused(config);
	printf("sihaDma close \n");
	return 0;
}


int sihaDma_MM2SStatus(DeviceConfig_t *config){
    sihaDmaConfig_t *sihaCfg = (sihaDmaConfig_t*) config->privConfig;
	uint32_t status;
	//printf("%llx\n", (long long int)ACCELCONFIG[slot]);
	INFO("\n");
	asm volatile("dmb ishld" ::: "memory");
	status = *(uint32_t*)(sihaCfg->mm2s_APCR);
	INFOP("################# MM2S %p ls ##############\n", (uint32_t*)(sihaCfg->mm2s_baseAddr));
	INFOP("ap_start     = %x \n", (status >> AP_START) & 0x1);
	INFOP("ap_done      = %x \n", (status >> AP_DONE) & 0x1);
	INFOP("ap_idle      = %x \n", (status >> AP_IDLE) & 0x1);
	INFOP("ap_ready     = %x \n", (status >> AP_READY) & 0x1);
	INFOP("auto_restart = %x \n", (status >> AP_AUTORESTART) & 0x1);
	status = *(uint32_t*)(sihaCfg->mm2s_GIER);
	INFOP("GIER = %x \n", status);
	status = *(uint32_t*)(sihaCfg->mm2s_IIER);
	INFOP("IIER = %x \n", status);
	status = *(uint32_t*)(sihaCfg->mm2s_IISR);
	INFOP("IISR = %x \n", status);
	INFOP("@ %x ADDR_LOW  = %x \n", MM2S + ADDR_LOW,  *(uint32_t*)(sihaCfg->mm2s_ADDR_LOW));
	INFOP("@ %x ADDR_HIGH = %x \n", MM2S + ADDR_HIGH, *(uint32_t*)(sihaCfg->mm2s_ADDR_HIGH));
	INFOP("@ %x SIZE_LOW  = %x \n", MM2S + SIZE_LOW,  *(uint32_t*)(sihaCfg->mm2s_SIZE_LOW));
	INFOP("@ %x SIZE_HIGH = %x \n", MM2S + SIZE_HIGH, *(uint32_t*)(sihaCfg->mm2s_SIZE_HIGH));
	INFOP("@ %x TID       = %x \n", MM2S + TID,       *(uint32_t*)(sihaCfg->mm2s_TID));
	INFOP("########################################\n");
	asm volatile("dmb ishld" ::: "memory");
	return 0;
}

int sihaDma_S2MMStatus(DeviceConfig_t *config){
    sihaDmaConfig_t *sihaCfg = (sihaDmaConfig_t*) config->privConfig;
	uint32_t status;
	INFO("\n");
	asm volatile("dmb ishld" ::: "memory"); 
	status = *(uint32_t*)(sihaCfg->s2mm_APCR);
	INFOP("################# S2MM %p ls ###############\n", (uint32_t*)(sihaCfg->s2mm_baseAddr));
	INFOP("ap_start     = %x \n", (status >> AP_START) & 0x1);
	INFOP("ap_done      = %x \n", (status >> AP_DONE) & 0x1);
	INFOP("ap_idle      = %x \n", (status >> AP_IDLE) & 0x1);
	INFOP("ap_ready     = %x \n", (status >> AP_READY) & 0x1);
	INFOP("auto_restart = %x \n", (status >> AP_AUTORESTART) & 0x1);
	status = *(uint32_t*)(sihaCfg->s2mm_GIER);
	INFOP("GIER = %x \n", status);
	status = *(uint32_t*)(sihaCfg->s2mm_IIER);
	INFOP("IIER = %x \n", status);
	status = *(uint32_t*)(sihaCfg->s2mm_IISR);
	INFOP("IISR = %x \n", status);
	INFOP("@ %x ADDR_LOW  = %x \n", S2MM + ADDR_LOW,  *(uint32_t*)(sihaCfg->s2mm_ADDR_LOW));
	INFOP("@ %x ADDR_HIGH = %x \n", S2MM + ADDR_HIGH, *(uint32_t*)(sihaCfg->s2mm_ADDR_HIGH));
	INFOP("@ %x SIZE_LOW  = %x \n", S2MM + SIZE_LOW,  *(uint32_t*)(sihaCfg->s2mm_SIZE_LOW));
	INFOP("@ %x SIZE_HIGH = %x \n", S2MM + SIZE_HIGH, *(uint32_t*)(sihaCfg->s2mm_SIZE_HIGH));
	INFOP("@ %x TID       = %x \n", S2MM + TID,       *(uint32_t*)(sihaCfg->s2mm_TID));
	INFOP("########################################\n");
	asm volatile("dmb ishld" ::: "memory");
	return 0;
}

int sihaDma_MM2SData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid){
    sihaDmaConfig_t *sihaCfg = (sihaDmaConfig_t*) config->privConfig;
	acapd_debug("MM2S %x\n", MM2S);
	_unused(firstLast);
	uint64_t address = buffer->phyAddr + offset;
	*(volatile uint32_t*)(sihaCfg->mm2s_ADDR_LOW)  = address & 0xFFFFFFFF;
	*(volatile uint32_t*)(sihaCfg->mm2s_ADDR_HIGH) = (address >> 32) & 0xFFFFFFFF;
	acapd_debug("ADDR_LOW : %x\n", address & 0xffffffff);
	acapd_debug("ADDR_HIGH: %x\n", (address >> 32) & 0xffffffff);
	acapd_debug("SIZE_LOW : %x\n", (size >> 4) & 0xffffffff);
	acapd_debug("SIZE_HIGH: %x\n", (size >> 36) & 0xffffffff);
	acapd_debug("TID      : %x\n", tid);
	acapd_debug("GIER     : %x\n", 0x01);
	acapd_debug("IIER     : %x\n", 0x03);
	acapd_debug("APCR     : %x\n", 0x01 << AP_START);
	*(volatile uint32_t*)(sihaCfg->mm2s_SIZE_LOW)  = (size >> 4) & 0xFFFFFFFF;
	*(volatile uint32_t*)(sihaCfg->mm2s_SIZE_HIGH) = (size >> 36) & 0xFFFFFFFF;
	*(volatile uint32_t*)(sihaCfg->mm2s_TID)       = tid;
	*(volatile uint32_t*)(sihaCfg->mm2s_GIER)      = 0x1;
	*(volatile uint32_t*)(sihaCfg->mm2s_IIER)      = 0x3;
	*(volatile uint32_t*)(sihaCfg->mm2s_APCR)      = 0x1 << AP_START;
	return 0;
}

int sihaDma_S2MMData(DeviceConfig_t *config, BuffConfig_t *buffer, uint64_t offset, uint64_t size, uint8_t firstLast){
    sihaDmaConfig_t *sihaCfg = (sihaDmaConfig_t*) config->privConfig;
	acapd_debug("S2MM %x\n", S2MM);
	_unused(firstLast);
	uint64_t address = buffer->phyAddr + offset;
	*(uint32_t*)(sihaCfg->s2mm_ADDR_LOW)  = address & 0xFFFFFFFF;
	*(uint32_t*)(sihaCfg->s2mm_ADDR_HIGH) = (address >> 32) & 0xFFFFFFFF;
	acapd_debug("ADDR_LOW : %x\n", address & 0xffffffff);
	acapd_debug("ADDR_HIGH: %x\n", (address >> 32) & 0xffffffff);
	acapd_debug("SIZE_LOW : %x\n", (size >> 4) & 0xffffffff);
	acapd_debug("SIZE_HIGH: %x\n", (size >> 36) & 0xffffffff);
	acapd_debug("GIER     : %x\n", 0x01);
	acapd_debug("IIER     : %x\n", 0x03);
	acapd_debug("APCR     : %x\n", 0x01 << AP_START);
	*(uint32_t*)(sihaCfg->s2mm_SIZE_LOW)  = (size >> 4) & 0xFFFFFFFF;
	*(uint32_t*)(sihaCfg->s2mm_SIZE_HIGH) = (size >> 36) & 0xFFFFFFFF;
	*(uint32_t*)(sihaCfg->s2mm_GIER)      = 0x1;
	*(uint32_t*)(sihaCfg->s2mm_IIER)      = 0x3;
	*(uint32_t*)(sihaCfg->s2mm_APCR)      = 0x1 << AP_START;
	return 0;
}

int sihaDma_S2MMDone(DeviceConfig_t *config){
    sihaDmaConfig_t *sihaCfg = (sihaDmaConfig_t*) config->privConfig;
	int status, res;
	status = *(uint32_t*)(uint32_t*)(sihaCfg->s2mm_APCR);
	res = status ? ((status >> AP_DONE) | (status >> AP_IDLE)) & 0x1 : 1;
	acapd_debug("S2MM Status : %x %x\n", status, res);
	//if(res){
	//	data->writeStatus += 1;
	//}
	//getch();
	return res;
}

int sihaDma_MM2SDone(DeviceConfig_t *config){
    sihaDmaConfig_t *sihaCfg = (sihaDmaConfig_t*) config->privConfig;
	int status, res;
	status = *(volatile uint32_t*)(uint32_t*)(sihaCfg->mm2s_APCR);
	res = status ? ((status >> AP_DONE) | (status >> AP_IDLE)) & 0x1 : 1;
	acapd_debug("MM2S Status : %x %x\n", status, res);
	//if(res){
	//	data->readStatus += 1;
	//	if(data->readStatus == 2*data->readerCount){
	//		data->readStatus = 0;
	//		data->writeStatus = 0;
	//	}
	//}
	//getch();
	return res;
}

int registerDriver(Device_t** device, DeviceConfig_t** config){
	Device_t *tDevice = (Device_t*)malloc(sizeof(Device_t)); 
	DeviceConfig_t *tConfig = (DeviceConfig_t*)malloc(sizeof(DeviceConfig_t)); 
	strcpy(tConfig->name, "sihaDma");
	tDevice->open       = sihaDma_open;
	tDevice->close      = sihaDma_close;
	tDevice->MM2SStatus = sihaDma_MM2SStatus;
	tDevice->S2MMStatus = sihaDma_S2MMStatus;
	tDevice->MM2SData   = sihaDma_MM2SData;
	tDevice->S2MMData   = sihaDma_S2MMData;
	tDevice->MM2SDone   = sihaDma_MM2SDone;
	tDevice->S2MMDone   = sihaDma_S2MMDone;
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
