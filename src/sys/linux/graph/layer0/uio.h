/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
typedef struct dm dm_t;

struct plDevices{
	//int clk_wiz_fd;
	//int SIHA_Manager_fd;
	int slot[20];
	int AccelConfig_fd[20];
	//int AccelConfig_0_fd;
	//int AccelConfig_1_fd;
	//int AccelConfig_2_fd;
	int dma_hls_fd[20];
	//int dma_hls_0_fd;
	//int dma_hls_1_fd;
	//int dma_hls_2_fd;
	//uint8_t* clk_wiz;
	//uint8_t* SIHA_Manager;
	uint8_t* AccelConfig[20];
	//uint8_t* AccelConfig_0;
	//uint8_t* AccelConfig_1;
	//uint8_t* AccelConfig_2;
	uint8_t* dma_hls[20];
	//uint8_t* dma_hls_0;
	//uint8_t* dma_hls_1;
	//uint8_t* dma_hls_2;
};
typedef struct plDevices plDevices_t; 

typedef int (*FALLBACKFUNCTION)(void*, int, void*, int, void*, int);

#define ACCELDONE 1
#define INTER_RM_COMPATIBLE 1
#define INTER_RM_NOT_COMPATIBLE 0
struct Accel{
        char name[1024];// Accelerator name
        int index;      // Serial No of Accelerator
        int inHardware; // This reg captures the Accel implemented in Hardware
        int type;       //
        int inDmaType;  // Inward  Data Mover Type
        int outDmaType; // Outward Data Mover Type
        FALLBACKFUNCTION fallbackfunction;// Pointer to a software fallback function
        int AccelConfig_fd;
        int dma_hls_fd;
        int accels_fd[10];
        uint8_t* AccelConfig;
        uint8_t* dma_hls;
        uint8_t* accels[10];
        int slot;
        int InterRMCompatible;
        int SchedulerBypassFlag;
        uint8_t* confBuffer;
        uint8_t* softBuffer;
        uint64_t softBufferSize;
        uint64_t status;
        dm_t* datamover;
};

typedef struct Accel Accel_t;


extern int initPlDevices(plDevices_t* pldevices, int slot);
extern int finalisePlDevices(plDevices_t* pldevices, int slot);
extern int mapPlDevices(plDevices_t* pldevices, int slot);
extern int mapPlDevicesAccel(plDevices_t* pldevices, int slot);
extern int unmapPlDevicesAccel(plDevices_t* pldevices, int slot);
extern int printPlDevices(plDevices_t* pldevices);
