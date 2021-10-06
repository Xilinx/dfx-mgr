/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
typedef struct dm dm_t;
typedef struct DFXBuffer DFXBuffer_t;
typedef struct Accel  Accel_t;
// HLS control
#define APCR       0x00
#define GIER       0x04
#define IIER       0x08
#define IISR       0x0c
#define ADDR_LOW   0x10
#define ADDR_HIGH  0x14
#define SIZE_LOW   0x1c
#define SIZE_HIGH  0x20
#define TID        0x24
//Control signals
#define AP_START       0
#define AP_DONE        1
#define AP_IDLE        2
#define AP_READY       3
#define AP_AUTORESTART 7

//#define S2MM      0x0000
//#define MM2S      0x8000
//#define S2MM      0x8000
//#define MM2S      0xc000
#define S2MM      0x00000
#define MM2S      0x10000

typedef struct sihahls_DMConfig{
	volatile uint8_t* s2mm_baseAddr; 
	volatile uint8_t* s2mm_APCR; 
	volatile uint8_t* s2mm_GIER; 
	volatile uint8_t* s2mm_IIER; 
	volatile uint8_t* s2mm_IISR; 
	volatile uint8_t* s2mm_ADDR_LOW; 
	volatile uint8_t* s2mm_ADDR_HIGH; 
	volatile uint8_t* s2mm_SIZE_LOW; 
	volatile uint8_t* s2mm_SIZE_HIGH; 
	volatile uint8_t* s2mm_TID; 
	volatile uint8_t* mm2s_baseAddr; 
	volatile uint8_t* mm2s_APCR; 
	volatile uint8_t* mm2s_GIER; 
	volatile uint8_t* mm2s_IIER; 
	volatile uint8_t* mm2s_IISR; 
	volatile uint8_t* mm2s_ADDR_LOW; 
	volatile uint8_t* mm2s_ADDR_HIGH; 
	volatile uint8_t* mm2s_SIZE_LOW; 
	volatile uint8_t* mm2s_SIZE_HIGH; 
	volatile uint8_t* mm2s_TID;
	int start;
	Accel_t *accel;
	//int slot; 
}sihahls_DMConfig_t;

extern int sihahls_config(void* dmconfig_a, Accel_t *accel);
extern int sihahls_MM2SStatus(void* dmconfig_a);
extern int sihahls_S2MMStatus(void* dmconfig_a);
extern int sihahls_MM2SData(void* dmconfig_a, uint64_t data, uint64_t size, uint8_t tid);
extern int sihahls_S2MMData(void* dmconfig_a, uint64_t data, uint64_t size);
extern int sihahls_MM2SData_B(void* dmconfig_a, DFXBuffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast, uint8_t tid);
extern int sihahls_S2MMData_B(void* dmconfig_a, DFXBuffer_t* data, uint64_t offset, uint64_t size, uint8_t firstLast);
extern int sihahls_S2MMDone(void* dmconfig_a, DFXBuffer_t* data);
extern int sihahls_MM2SDone(void* dmconfig_a, DFXBuffer_t* data);
extern int sihahls_MM2SAck(void* dmconfig_a);
extern int sihahls_S2MMAck(void* dmconfig_a);

extern int sihahls_register(dm_t *datamover);
extern int sihahls_unregister(dm_t *datamover);

