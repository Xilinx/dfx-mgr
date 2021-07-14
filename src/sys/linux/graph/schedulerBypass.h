/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
//#include "dm.h"

#define SCHEDULER_TASKEND     0x00FF
#define SCHEDULER_STATUS      0x00F0
#define SCHEDULER_TRIGGER     0x00F1
#define SCHEDULER_COMPLETION  0x00F2

typedef struct ScQueueBuffer{
	int type;
	char filename[1024];
	Buffer_t* buffer;
	Buffer_t* dependentBuffer;
	int transferSize;
	double throughput;
}ScQueueBuffer_t;

extern int SchedulerInit(AcapGraph_t *acapGraph);
extern int SchedulerFinalise(AcapGraph_t *acapGraph);
extern int SchedulerCompletion(AcapGraph_t *acapGraph);
extern int SchedulerTrigger(AcapGraph_t *acapGraph);
