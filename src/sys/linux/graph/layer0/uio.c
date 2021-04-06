/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "uio.h"
#include "uiomap.h"

//static plDevices pldevices;

int initPlDevices(plDevices_t* pldevices, int slot){
	//static plDevices pldevices;
	//pldevices->clk_wiz_fd = open(DEV_clk_wiz, O_RDWR);
	//if(pldevices->clk_wiz_fd < 0) return -1;

	//pldevices->SIHA_Manager_fd = open(DEV_SIHA_Manager, O_RDWR);
	//if(pldevices->SIHA_Manager_fd < 0) return -1;

	pldevices->AccelConfig_fd[slot] = open(DEV_AccelConfig[slot], O_RDWR);
	if(pldevices->AccelConfig_fd[slot] < 0) return -1;
		
	pldevices->dma_hls_fd[slot] = open(DEV_dma_hls[slot], O_RDWR);
	if(pldevices->dma_hls_fd[slot] < 0) return -1;
	
	/*switch(slot){
		case 0:
			pldevices->AccelConfig_0_fd = open(DEV_AccelConfig_0, O_RDWR);
			if(pldevices->AccelConfig_0_fd < 0) return -1;
		
			pldevices->dma_hls_0_fd = open(DEV_dma_hls_0, O_RDWR);
			if(pldevices->dma_hls_0_fd < 0) return -1;
			//printf("PlDevices openned !!\n", DEV_AccelConfig_0);
		
			break;
		case 1:
			pldevices->AccelConfig_1_fd = open(DEV_AccelConfig_1, O_RDWR);
			if(pldevices->AccelConfig_1_fd < 0) return -1;
		
			pldevices->dma_hls_1_fd = open(DEV_dma_hls_1, O_RDWR);
			if(pldevices->dma_hls_1_fd < 0) return -1;
		
			break;
		case 2:
			pldevices->AccelConfig_2_fd = open(DEV_AccelConfig_2, O_RDWR);
			if(pldevices->AccelConfig_2_fd < 0) return -1;
			
			pldevices->dma_hls_2_fd = open(DEV_dma_hls_2, O_RDWR);
			if(pldevices->dma_hls_2_fd < 0) return -1;

			break;
	}*/

	return 0;
}

int finalisePlDevices(plDevices_t* pldevices, int slot){
	//static plDevices pldevices;
	//close(pldevices->clk_wiz_fd);
	//close(pldevices->SIHA_Manager_fd);

	close(pldevices->AccelConfig_fd[slot]);
	close(pldevices->dma_hls_fd[slot]);
	/*switch(slot){
		case 0:
			close(pldevices->AccelConfig_0_fd);
			close(pldevices->dma_hls_0_fd);
			break;
		case 1:
			close(pldevices->AccelConfig_1_fd);
 			close(pldevices->dma_hls_1_fd);
			break;
		case 2:
			close(pldevices->AccelConfig_2_fd);
			close(pldevices->dma_hls_2_fd);
			break;
	}*/

	return 0;
}

int mapPlDevices(plDevices_t* pldevices, int slot){
	//pldevices->SIHA_Manager = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
	//		pldevices->SIHA_Manager_fd, 0);
	//if(pldevices->SIHA_Manager == MAP_FAILED){
	//	printf("Mmap SIHA Manager failed !!\n");
	//	return -1;
	//}
	pldevices->AccelConfig[slot] = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
							pldevices->AccelConfig_fd[slot], 0);
	if(pldevices->AccelConfig[slot] == MAP_FAILED){
		printf("Mmap AccelConfig_0 failed !!\n");
		return -1;
	}
	
	pldevices->dma_hls[slot] = (uint8_t*) mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, 
						pldevices->dma_hls_fd[slot], 0);
	if(pldevices->dma_hls[slot] == MAP_FAILED){
		printf("Mmap dma_hls_0 failed !!\n");
		return -1;
	}

	/*switch(slot){
		case 0:	pldevices->AccelConfig_0 = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->AccelConfig_0_fd, 0);
			if(pldevices->AccelConfig_0 == MAP_FAILED){
				printf("Mmap AccelConfig_0 failed !!\n");
				return -1;
			}
	
			pldevices->dma_hls_0 = (uint8_t*) mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->dma_hls_0_fd, 0);
			if(pldevices->dma_hls_0 == MAP_FAILED){
				printf("Mmap dma_hls_0 failed !!\n");
				return -1;
			}
			break;
		case 1:	pldevices->AccelConfig_1 = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->AccelConfig_1_fd, 0);
			if(pldevices->AccelConfig_1 == MAP_FAILED){
				printf("Mmap AccelConfig_1 failed !!\n");
				return -1;
			}
	
			pldevices->dma_hls_1 = (uint8_t*) mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->dma_hls_1_fd, 0);
			if(pldevices->dma_hls_1 == MAP_FAILED){
				printf("Mmap dma_hls_1 failed !!\n");
				return -1;
			}
			break;
		case 2:	pldevices->AccelConfig_2 = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->AccelConfig_2_fd, 0);
			if(pldevices->AccelConfig_2 == MAP_FAILED){
				printf("Mmap AccelConfig_2 failed !!\n");
				return -1;
			}
	
			pldevices->dma_hls_2 = (uint8_t*) mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->dma_hls_2_fd, 0);
			if(pldevices->dma_hls_2 == MAP_FAILED){
				printf("Mmap dma_hls_2 failed !!\n");
				return -1;
			}
			break;
	}*/
	return 0;
}

int mapPlDevicesAccel(plDevices_t* pldevices, int slot){
	pldevices->AccelConfig[slot] = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->AccelConfig_fd[slot], 0);
	if(pldevices->AccelConfig[slot] == MAP_FAILED){
		printf("Mmap AccelConfig_%d failed !!\n", slot);
		return -1;
	}
	
	pldevices->dma_hls[slot] = (uint8_t*) mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->dma_hls_fd[slot], 0);
	if(pldevices->dma_hls[slot] == MAP_FAILED){
		printf("Mmap dma_hls_%d failed !!\n", slot);
		return -1;
	}
	/*switch(slot){
		case 0:	pldevices->AccelConfig_0 = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->AccelConfig_0_fd, 0);
			if(pldevices->AccelConfig_0 == MAP_FAILED){
				printf("Mmap AccelConfig_0 failed !!\n");
				return -1;
			}
	
			pldevices->dma_hls_0 = (uint8_t*) mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->dma_hls_0_fd, 0);
			if(pldevices->dma_hls_0 == MAP_FAILED){
				printf("Mmap dma_hls_0 failed !!\n");
				return -1;
			}
			break;
		case 1:	pldevices->AccelConfig_1 = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->AccelConfig_1_fd, 0);
			if(pldevices->AccelConfig_1 == MAP_FAILED){
				printf("Mmap AccelConfig_1 failed !!\n");
				return -1;
			}
	
			pldevices->dma_hls_1 = (uint8_t*) mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->dma_hls_1_fd, 0);
			if(pldevices->dma_hls_1 == MAP_FAILED){
				printf("Mmap dma_hls_1 failed !!\n");
				return -1;
			}
			break;
		case 2:	pldevices->AccelConfig_2 = (uint8_t*) mmap(0, 0x9000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->AccelConfig_2_fd, 0);
			if(pldevices->AccelConfig_2 == MAP_FAILED){
				printf("Mmap AccelConfig_2 failed !!\n");
				return -1;
			}
	
			pldevices->dma_hls_2 = (uint8_t*) mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, 
					pldevices->dma_hls_2_fd, 0);
			if(pldevices->dma_hls_2 == MAP_FAILED){
				printf("Mmap dma_hls_2 failed !!\n");
				return -1;
			}
			break;
	}*/
	return 0;
}

int unmapPlDevicesAccel(plDevices_t* pldevices, int slot){

	munmap(pldevices->AccelConfig[slot], 0x9000);
	munmap(pldevices->dma_hls[slot], 0x10000);
	/*switch(slot){
		case 0:	munmap(pldevices->AccelConfig_0, 0x9000);
			munmap(pldevices->dma_hls_0, 0x10000);
			break;
		case 1:	munmap(pldevices->AccelConfig_1, 0x9000);
			munmap(pldevices->dma_hls_1, 0x10000);
			break;
		case 2:	munmap(pldevices->AccelConfig_2, 0x9000);
			munmap(pldevices->dma_hls_2, 0x10000);
			break;
	}*/
	return 0;
}

int printPlDevices(plDevices_t* pldevices){
	for(int i = 0; i < 10; i ++){
        	printf("Accel Config %d : %d : %p \n", i, pldevices->AccelConfig_fd[i], pldevices->AccelConfig[i]);
	        printf("DMA HLS %d      : %d : %p \n", i, pldevices->dma_hls_fd[i], pldevices->dma_hls[i]);
	}
        /*printf("Accel Config 0 : %d\n", pldevices->AccelConfig_0_fd);
        printf("DMA HLS 0      : %d\n", pldevices->dma_hls_0_fd);
        printf("Accel Config 1 : %d\n", pldevices->AccelConfig_1_fd);
        printf("DMA HLS 1      : %d\n", pldevices->dma_hls_1_fd);
        printf("Accel Config 2 : %d\n", pldevices->AccelConfig_2_fd);
        printf("DMA HLS 2      : %d\n", pldevices->dma_hls_2_fd);*/
        return 0;
}

