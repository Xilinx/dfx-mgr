/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef XRTBUFFER_H
#include <stdint.h>

struct Buffers{
	int fd;
	int config_fd[20];
	int config_size[20];
	int config_handle[20];
	int config_used[20];
	uint8_t* config_ptr[20];
	unsigned long config_paddr[20];
	
	int S2MM_fd[20];
	int S2MM_size[20];
	int S2MM_handle[20];
	int S2MM_used[20];
	uint8_t* S2MM_ptr[20];
	unsigned long S2MM_paddr[20];
	
	int MM2S_fd[20];
	int MM2S_size[20];
	int MM2S_handle[20];
	int MM2S_used[20];
	uint8_t* MM2S_ptr[20];
	unsigned long MM2S_paddr[20];
};
typedef struct Buffers Buffers_t;

#define EMPTY 0
#define INQUEUE 1
#define FULL  2
struct Buffer{
        int index; // Serial No of Buffer
        int type;  //
        int fd;    // File descriptor from ACAPD
        int size;  // Buffer Size
        int handle;// Buffer XRT Handeler
        uint8_t* ptr; // Buffer Ptr
        unsigned long phyAddr; // Buffer Physical Address
        int srcSlot; // Buffer Physical Address
        int sincSlot; // Buffer Physical Address
        unsigned long otherAccel_phyAddr[3]; // Buffer Physical Address
        int InterRMCompatible; // Buffer Physical Address
        unsigned int readctr[2]; 
        unsigned int writectr[2];
        unsigned int readerCount;
        unsigned int readStatus;
        unsigned int writeStatus;
        unsigned int status;
        char name[1024]; // Buffer Name Identifier
};

typedef struct Buffer Buffer_t;

extern int printBuffer(Buffers_t* buffers, int slot);
extern int initXrtBuffer(int slot, Buffers_t* buffers);
extern int initXrtBuffer(int slot, Buffers_t* buffers);
extern int finaliseXrtBuffer(int slot, Buffers_t* buffers);
extern int mapBuffer(int fd, int size, uint8_t** ptr);
extern int unmapBuffer(int fd, int size, uint8_t** ptr);
extern int xrt_allocateBuffer(int fd, int size, int* handle, uint8_t** ptr, unsigned long* paddr);
#endif

#define XRTBUFFER_H

