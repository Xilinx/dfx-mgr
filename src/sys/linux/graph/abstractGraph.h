/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <semaphore.h>
typedef struct AbstractBuffNode AbstractBuffNode_t;
typedef struct AbstractAccelNode AbstractAccelNode_t;
typedef struct AbstractLink AbstractLink_t;
typedef struct AbstractGraph AbstractGraph_t;
typedef struct Element Element_t;
typedef struct graphSocket graphSocket_t;
typedef struct AccelNode AccelNode_t;
typedef struct BuffNode BuffNode_t;
typedef struct Link Link_t;
typedef struct JobScheduler JobScheduler_t; 

#define AGRAPH_INIT		0XC0
#define AGRAPH_SCHEDULED	0XC1
#define AGRAPH_EXECUTING	0XC2
#define AGRAPH_COMPLETED	0XC3

struct Element{
	void* node;
        struct Element *head;
        struct Element *tail;
};

struct AbstractAccelNode{
        uint8_t type;
        char name[256];
        uint32_t size;
        uint32_t id;
        int fd;    	// File descriptor from ACAPD
        int handle;	// Buffer XRT Handeler
        uint8_t* ptr;	// Buffer Ptr
        unsigned long phyAddr; // Buffer Physical Address
	uint32_t semaphore; 
	sem_t* semptr;
	AccelNode_t *node;
        //int SchedulerBypassFlag;
};

struct AbstractBuffNode{
        uint8_t type;
        char name[256];
        uint32_t size;
        uint32_t id;
	BuffNode_t *node;
};

struct AbstractLink{
        AbstractAccelNode_t *accelNode;// Reference to connected accelerator
        AbstractBuffNode_t *buffNode;// Reference to connected buffer
        uint8_t type;
        uint8_t transactionIndex;
        uint32_t transactionSize;
        uint32_t offset;
        uint8_t channel;
        uint32_t id;
	Link_t *node;
};

struct AbstractGraph{
        uint32_t id;
        uint8_t type;
        uint8_t state;
        uint32_t accelCount;
	graphSocket_t *gs;
	int xrt_fd;
        Element_t *accelNodeHead;
        Element_t *buffNodeHead;
        Element_t *linkHead;
        uint32_t accelNodeID;
        uint32_t buffNodeID;
        uint32_t linkID;
};



//extern AcapGraph_t* graphInit();
//extern int graphFinalise(AcapGraph_t *acapGraph);
extern AbstractGraph_t* graphInit(); //uint8_t schedulerBypassFlag);
extern int graphFinalise(AbstractGraph_t *graph);

extern AbstractAccelNode_t* addInputNode(AbstractGraph_t *graph, int size);
extern AbstractAccelNode_t* addOutputNode(AbstractGraph_t *graph, int size);
extern AbstractAccelNode_t* addAcceleratorNode(AbstractGraph_t *graph, char *name);
extern AbstractBuffNode_t* addBuffer(AbstractGraph_t *graph, int size, int type);
extern AbstractLink_t *addOutBuffer(AbstractGraph_t *graph, AbstractAccelNode_t *accelNode, AbstractBuffNode_t *buffNode,
			    uint32_t offset, uint32_t transactionSize, uint8_t transactionIndex, uint8_t channel);

extern AbstractLink_t *addInBuffer(AbstractGraph_t *graph, AbstractAccelNode_t *accelNode, AbstractBuffNode_t *buffNode,
			    uint32_t offset, uint32_t transactionSize, uint8_t transactionIndex, uint8_t channel);

extern int abstractGraph2Json(AbstractGraph_t *graph, char* json);
extern int abstractGraphConfig(AbstractGraph_t *graph);
extern int abstractGraphFinalise(AbstractGraph_t *graph);
extern Element_t* addElement(Element_t** headElement, Element_t* nextElement);
extern Element_t *searchGraphById(Element_t** headElement, uint32_t id);
extern int abstractGraphServerConfig(JobScheduler_t *scheduler, char* json, int len, int* fd);
extern int abstractGraphServerFinalise(JobScheduler_t *scheduler, char* json);
extern int reallocateIOBuffers(AbstractGraph_t *graph, int* fd, int fdcount);
extern int appFinaliseIPBuffers(AbstractGraph_t *graph);
