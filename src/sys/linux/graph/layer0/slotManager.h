/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
//#include "dm.h"

#define TASKEND     0x00FF
#define STATUS      0x00F0
#define LOOPFILE    0x00F1
#define FFTCONFIG   0x00F2
#define GETFFT      0x00F3
#define BUFFER      0x00F4
#define BUFFERLOOP  0x00F5
#define DONE        0x00F6
#define CMATRANSACTION 0x00F7
#define TRANSACTIONEND 0x00F8

#define TRANS_S2MM 0
#define TRANS_MM2S 1
typedef struct QueueBuffer{
	int type;
	char filename[1024];
	Buffer_t* buffer;
	Buffer_t* dependentBuffer;
	int direction;
	int transferSize;
	double throughput;
	int status;
}QueueBuffer_t;

extern int TaskInit(dm_t* datamover);
extern int TaskFinalise(dm_t* datamover);
extern int TaskEnd(dm_t* datamover);
extern int transact(dm_t *datamover, Buffer_t* buffer, Buffer_t* dependentBuffer, int insize, int direction);
extern int MM2S_Buffer(dm_t *datamover, Buffer_t* buffer, Buffer_t* dependentBuffer, int insize);
extern int S2MM_Buffer(dm_t *datamover, Buffer_t* buffer, int outsize);
extern int S2MM_Completion(dm_t *datamover);
extern int MM2S_Completion(dm_t *datamover);

