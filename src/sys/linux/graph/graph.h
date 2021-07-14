/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include "layer0/uio.h"
#include "layer0/xrtbuffer.h"
#include "layer0/dm.h"
#include <semaphore.h>

#define DDR_BASED 0
#define PL_BASED  1
#define HW_NODE   0
#define SW_NODE   1
#define IO_NODE   2
#define IN_NODE   3
#define OUT_NODE  4

//dmaTypes
#define NONE   0
#define HLSDMA 1
#define HLS_MULTICHANNEL_DMA 2

#define FREE 0
#define BUSY 1

#define BUFF_EMPTY 0
#define BUFF_FULL  1
#define BUFF_BUSY  2

//typedef struct Buffer Buffer_t;
typedef struct BuffNode BuffNode_t;
typedef struct AccelNode AccelNode_t;
typedef struct Link Link_t;
typedef struct AcapGraph AcapGraph_t;
typedef struct DependencyList DependencyList_t;
typedef struct Schedule Schedule_t;
typedef struct Scheduler Scheduler_t;

// Layer 1 Graph Structures
struct AccelNode{
	Accel_t accel; // Accel Node Link List structure contains all the Accel nodes
	int S2MMStatus;
	int MM2SStatus;
	int SchedulerBypassFlag;
	int currentTransactionIndex;
	struct AccelNode *head;
	struct AccelNode *tail;
};

struct BuffNode{
	Buffer_t buffer; // Buffer Node Link List structure contains all the Buffer nodes
	int status;
	int readStatus;
	struct BuffNode *head;
	struct BuffNode *tail;
};

struct Link{// Link(transaction paths) Link List structure contains all the link nodes
	AccelNode_t *accelNode;// Reference to connected accelerator
	BuffNode_t *buffNode;// Reference to connected buffer
	int transactionIndex;
	int transactionSize;
	int offset;
	int totalTransactionDone;
	int totalTransactionScheduled;
	int type;// Direction of transfer path MM2S or S2MM
	int channel;
	int accounted;
	struct Link *head;
	struct Link *tail;
};

/*struct DependencyList{
  Link_t *S2MMlink;
  Link_t *MM2Slink;
  struct DependencyList *head;
  struct DependencyList *tail;
  };*/

struct DependencyList{
	Link_t *link;
	Link_t *dependentLinks[10];
	int linkCount;
	struct DependencyList *head;
	struct DependencyList *tail;
};
struct Schedule{
	DependencyList_t *dependency;
	//Link_t *link;
	int index;
	int size;
	int offset;
	int status;
	int last;
	int first;
	//BuffNode_t *dependentBuffNode[10];// Reference to connected buffer
	//int buffCount;
	struct Schedule *head;
	struct Schedule *tail;
};

struct Scheduler{
	queue_t* CommandQueue;
	queue_t* ResponseQueue;
	pthread_t thread[1];
};

struct AcapGraph{
	int drmfd;
	AccelNode_t *accelNodeHead;
	BuffNode_t *buffNodeHead;
	Link_t *linkHead;
	DependencyList_t *dependencyHead;
	Schedule_t *scheduleHead;
	Scheduler_t *scheduler;
};


extern AccelNode_t* addAccelNode(AccelNode_t **accelNode, AccelNode_t* nextAccel);
extern int forceFallback(AccelNode_t *accelNode);
extern int delAccelNode(AccelNode_t** accelNode);
extern int printAccelInfo(Accel_t accel, char* json);
extern int printAccelNodes(AccelNode_t *accelNode, char* json);
extern int printAccelNodesInfo(AccelNode_t *accelNode, char* json);
extern BuffNode_t* addBuffNode(BuffNode_t** buffNode, BuffNode_t* nextBuff);
extern int delBuffNode(BuffNode_t** buffNode, int drm_fd);
extern int printBuffInfo(Buffer_t buffer, char* json);
extern int printBuffNodes(BuffNode_t *buffNode, char* json);
extern int printBuffNodesInfo(BuffNode_t *buffNode, char* json);
extern Link_t* addLink(Link_t** link, Link_t* nextLink);
extern int delLink(Link_t** link);
extern int printLinkInfo(Link_t *link, char *json);
extern int printLinks(Link_t *link, char *json);
extern int printLinksInfo(Link_t *link, char* json);
extern Link_t* addInputBuffer (AccelNode_t *accelNode, BuffNode_t *buffNode, 
		int offset, int transactionSize, int transactionIndex, int channel);
extern Link_t* addOutputBuffer(AccelNode_t *accelNode, BuffNode_t *buffNode,
		int offset, int transactionSize, int transactionIndex, int channel);

//################################################################################
//         AcapdGraph
//################################################################################
#define DISABLE_SCHEDULER 0
#define ENABLE_SCHEDULER 1
AccelNode_t* acapAddAccelNode(AcapGraph_t *acapGraph, char *name, int dmaType, FALLBACKFUNCTION fallbackfunction, int InterRMCompatible, int SchedulerBypassFlag);
AccelNode_t* acapAddAccelNodeForceFallback(AcapGraph_t *acapGraph, char *name, int dmaType, FALLBACKFUNCTION fallbackfunction, int InterRMCompatible, int SchedulerBypassFlag);
extern AccelNode_t* acapAddInputNode(AcapGraph_t *acapGraph, uint8_t *buff, int size, int SchedulerBypassFlag, sem_t* semptr);
extern AccelNode_t* acapAddOutputNode(AcapGraph_t *acapGraph, uint8_t *buff, int size, int SchedulerBypassFlag, sem_t* semptr);
BuffNode_t* acapAddBuffNode(AcapGraph_t *acapGraph, int size, char *name, int type);
Link_t *acapAddOutputBuffer(AcapGraph_t *acapGraph, AccelNode_t *accelNode, BuffNode_t *buffNode, 
		int offset, int transactionSize, int transactionIndex, int channel);
Link_t *acapAddInputBuffer(AcapGraph_t *acapGraph, AccelNode_t *accelNode, BuffNode_t *buffNode, 
		int offset, int transactionSize, int transactionIndex, int channel);
int acapGraphToJson(AcapGraph_t *acapGraph);
int acapGraphConfig(AcapGraph_t *acapGraph);
int acapGraphSchedule(AcapGraph_t *acapGraph);
AcapGraph_t* acapGraphInit();
int acapGraphFinalise(AcapGraph_t *acapGraph);
extern int delSchedule(Schedule_t** schedule, Schedule_t** scheduleHead);
