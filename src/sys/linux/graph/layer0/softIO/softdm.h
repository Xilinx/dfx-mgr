/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <semaphore.h>
typedef struct dm dm_t;
typedef struct Buffer Buffer_t;
typedef struct Accel  Accel_t;

typedef struct soft_DMConfig{
        uint8_t* data; 
        uint64_t size; 
	sem_t* semptr;
}soft_DMConfig_t;

extern int soft_config(void* dmconfig_a, Accel_t *accel); //, volatile uint8_t* base);
extern int soft_MM2SStatus(void* dmconfig_a);
extern int soft_S2MMStatus(void* dmconfig_a);
extern int soft_MM2SData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid);
extern int soft_S2MMData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast);
extern int soft_S2MMDone(void* dmconfig_a, Buffer_t* data);
extern int soft_MM2SDone(void* dmconfig_a, Buffer_t* data);
extern int soft_MM2SAck(void* dmconfig_a);
extern int soft_S2MMAck(void* dmconfig_a);

extern int soft_register(dm_t *datamover);
extern int soft_unregister(dm_t *datamover);

