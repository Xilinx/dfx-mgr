/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
//#include "queue.h"
#include <pthread.h>

typedef struct Buffer Buffer_t;
typedef struct Accel  Accel_t;
typedef struct queue queue_t; 

typedef int (*CONFIG)(void*, Accel_t *accel); //volatile uint8_t*);
typedef int (*MM2SSTATUS)(void*);
typedef int (*S2MMSTATUS)(void*);
typedef int (*MM2SDATA)(void*, Buffer_t*, uint64_t, uint64_t, uint8_t, uint8_t);
typedef int (*S2MMDATA)(void*, Buffer_t*, uint64_t, uint64_t, uint8_t);
typedef int (*S2MMDONE)(void*, Buffer_t*);
typedef int (*MM2SDONE)(void*, Buffer_t*);

typedef struct dm{
	void* dmstruct;
	int S2MMChannelStatus;
	int MM2SChannelStatus;
        CONFIG     config;
        S2MMSTATUS S2MMStatus;
        S2MMDATA   S2MMData;
        S2MMDONE   S2MMDone;
        MM2SSTATUS MM2SStatus;
        MM2SDATA   MM2SData;
        MM2SDONE   MM2SDone;

        queue_t* MM2SCommandQueue;
        queue_t* MM2SResponseQueue;
        queue_t* S2MMCommandQueue;
        queue_t* S2MMResponseQueue;
        queue_t* CommandQueue;
        queue_t* ResponseQueue;
        queue_t* CacheQueue;
        pthread_t thread[2];

}dm_t;
