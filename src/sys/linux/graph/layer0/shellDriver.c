#include <stdio.h>
#include <stdint.h>

static volatile uint8_t* SHELLCONFIG;

//#define SHELLCONFIG                XPAR_STATIC_SHELL_SIHA_MANAGER_0_BASEADDR
int configSihaBase(volatile uint8_t* base){
	SHELLCONFIG = base;
	return 0;
}

int enableSlot(int slot){
	switch(slot){
		case 0:
			printf("enabling slot 0 ...\n");
			*(uint32_t*)(SHELLCONFIG + 0x4000)  = 0x01;
			*(uint32_t*)(SHELLCONFIG + 0x4004)  = 0x01;
			*(uint32_t*)(SHELLCONFIG + 0x4008)  = 0x0b;
			*(uint32_t*)(SHELLCONFIG + 0x400c)  = 0x02;
			break;
		case 1:
			printf("enabling slot 1 ...\n");
			*(uint32_t*)(SHELLCONFIG + 0x5000)  = 0x01;
			*(uint32_t*)(SHELLCONFIG + 0x5004)  = 0x01;
			*(uint32_t*)(SHELLCONFIG + 0x5008)  = 0x0b;
			*(uint32_t*)(SHELLCONFIG + 0x500c)  = 0x02;
			break;
		case 2:
			printf("enabling slot 2 ...\n");
			*(uint32_t*)(SHELLCONFIG + 0x6000)  = 0x01;
			*(uint32_t*)(SHELLCONFIG + 0x6004)  = 0x01;
			*(uint32_t*)(SHELLCONFIG + 0x6008)  = 0x0b;
			*(uint32_t*)(SHELLCONFIG + 0x600c)  = 0x02;
			break;
		default: break;
	}
	return 0;
}

int disableSlot(int slot){
	switch(slot){
		case 0:
			*(uint32_t*)(SHELLCONFIG + 0x4000)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x4004)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x4008)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x400c)  = 0x00;
			printf("disabling slot 0 ...\n");
			break;
		case 1:
			*(uint32_t*)(SHELLCONFIG + 0x5000)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x5004)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x5008)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x500c)  = 0x00;
			printf("disabling slot 1 ...\n");
			break;
		case 2:
			*(uint32_t*)(SHELLCONFIG + 0x6000)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x6004)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x6008)  = 0x00;
			*(uint32_t*)(SHELLCONFIG + 0x600c)  = 0x00;
			printf("disabling slot 2 ...\n");
			break;
		default: break;
	}
	return 0;
}
