/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/assert.h>
#include "jobScheduler.h"
#include "abstractGraph.h"
#include "graph.h"
#include "metadata.h"

int softgFFT(void** inData, int* inDataSize, void** outData, int* outDataSize){
    INFO("FALLBACK CALLED !!\n");
	_unused(outDataSize);
    memcpy(outData[0], inData[0], inDataSize[0]);
    return 0;
}

int softgFIR(void** inData, int* inDataSize, void** outData, int* outDataSize){
    INFO("FALLBACK CALLED !!\n");
	_unused(outDataSize);
    memcpy(outData[0], inData[0], inDataSize[0]);
    return 0;
}

Element_t* addElement(Element_t** headElement, Element_t* nextElement){
	Element_t *head = *headElement;
	if(head != NULL){
		while(head->tail != NULL){
			head = head->tail;
		}
		head->tail = nextElement;
		nextElement->head = head;
	}
	else{
		*headElement = nextElement;
	}
	return nextElement;
}

int delElement(Element_t** elementList, Element_t* element){
	Element_t *headElement = element->head;
	Element_t *tailElement = element->tail;
	
	if(headElement != NULL){
		headElement->tail = element;
	}else{
		*elementList = tailElement;
	}
	if(tailElement != NULL){
		tailElement->head = headElement;
	}
	free(element);
	element = NULL;
	return 0;
}

AbstractGraph_t* graphInit(){//uint8_t schedulerBypassFlag){
	srand(time(NULL));
	AbstractGraph_t * graph = malloc(sizeof(AbstractGraph_t));
	graph->id = rand(); 
	graph->type = ENABLE_SCHEDULER;
    graph->accelNodeHead = NULL;
    graph->buffNodeHead = NULL;
    graph->linkHead = NULL;
    graph->accelNodeID = 0;
    graph->buffNodeID = 0;
    graph->linkID = 0;
	graph->accelCount = 0;
	return graph;
}

//int graphFinalise(AcapGraph_t *acapGraph){
//	acapGraphFinalise(acapGraph);
//	return 0;
//}

int graphFinalise(AbstractGraph_t *graph){
	//acapGraphFinalise(acapGraph);
	free(graph);
	return 0;
}

/*AccelNode_t* addInputNode(AcapGraph_t *acapGraph, uint8_t *buff, int size, int SchedulerBypassFlag){
	return acapAddInputNode(acapGraph, buff, size, SchedulerBypassFlag);
}

AccelNode_t* addOutputNode(AcapGraph_t *acapGraph, uint8_t *buff, int size, int SchedulerBypassFlag){
	return acapAddOutputNode(acapGraph, buff, size, SchedulerBypassFlag);
}

AccelNode_t* addAcceleratorNode(AcapGraph_t *acapGraph, char *name, int SchedulerBypassFlag){
	Json_t* json = malloc(sizeof(Json_t));
	Metadata_t *metadata = malloc(sizeof(Metadata_t));
	char jsonfile[100];
	#if 1
	if(strcmp(name, "FFT4")){
		strcpy(jsonfile, "/media/test/home/root/accel.json");
	}
	else if(strcmp(name, "aes128encdec")){
		strcpy(jsonfile, "/media/test/home/root/accel.json");
	}
	else if(strcmp(name, "fir_compiler")){
		strcpy(jsonfile, "/media/test/home/root/accel.json");
	}
	#else
	if(strcmp(name, "FFT4")){
		strcpy(jsonfile, "/media/test/home/root/accelInterrm.json");
	}
	else if(strcmp(name, "aes128encdec")){
		strcpy(jsonfile, "/media/test/home/root/accelInterrm.json");
	}
	else if(strcmp(name, "fir_compiler")){
		strcpy(jsonfile, "/media/test/home/root/accelInterrm.json");
	}
	#endif

	file2json(jsonfile, json);
	json2meta(json, metadata);
	printMeta(metadata);
        return acapAddAccelNode(acapGraph, name, metadata->DMA_type, NULL, //metadata->fallback.lib,
							metadata->interRM.compatible, SchedulerBypassFlag);
}

BuffNode_t* addBuffer(AcapGraph_t *acapGraph, int size, int type){
	return acapAddBuffNode(acapGraph, size, "buffer", type);
}

Link_t *addOutBuffer(AcapGraph_t *acapGraph, AccelNode_t *accelNode, BuffNode_t *buffNode,
			    int offset, int transactionSize, int transactionIndex, int channel){
	return acapAddOutputBuffer(acapGraph, accelNode, buffNode, offset, transactionSize, transactionIndex, channel);
}

Link_t *addInBuffer(AcapGraph_t *acapGraph, AccelNode_t *accelNode, BuffNode_t *buffNode,
			    int offset, int transactionSize, int transactionIndex, int channel){
	return acapAddInputBuffer(acapGraph, accelNode, buffNode, offset, transactionSize, transactionIndex, channel);
}
*/
//Link_t *addInputBuffer(AcapGraph_t *acapGraph, AccelNode_t *accelNode, BuffNode_t *buffNode,
//			   int transactionSize, int transactionIndex, int channel){
//	return acapAddInputBuffer(AcapGraph_t *acapGraph, AccelNode_t *accelNode, BuffNode_t *buffNode,
//			   int transactionSize, int transactionIndex, int channel);
//}

AbstractAccelNode_t* createNode(uint32_t id, uint32_t type, uint32_t size, char* name){
	AbstractAccelNode_t *node = (AbstractAccelNode_t *) malloc(sizeof(AbstractAccelNode_t));
    node->id = id;
    node->type = type;
    strcpy(node->name, name);
    node->size = size;
	node->semaphore = rand(); 
	return node;
}

int printAccelNode(AbstractAccelNode_t *node, char * json){
	//printf("%p\n", node);
	//printf("type: %d\n", node->type);
	//printf("name: %s\n", node->name);
	//printf("size: %d\n", node->size);
	return sprintf(json, "{\"id\": %d, \"type\": %d, \"name\": \"%s\", \"size\": %d, \"semaphore\": %d}", node->id, node->type, node->name, node->size, node->semaphore);
}

AbstractBuffNode_t* createBNode(uint32_t id, uint32_t type, uint32_t size, char* name){
	AbstractBuffNode_t *node = (AbstractBuffNode_t *) malloc(sizeof(AbstractBuffNode_t));
    node->id = id;
    node->type = type;
    strcpy(node->name, name);
    node->size = size;
	return node;
}

int printBuffNode(AbstractBuffNode_t *node, char * json){
	//printf("type: %d, name: %s, size: %d\n", node->type, node->name, node->size);
	return sprintf(json, "{\"id\": %d, \"type\": %d, \"name\": \"%s\", \"size\": %d}", node->id, node->type, node->name, node->size);
}

AbstractLink_t* createLink(AbstractAccelNode_t *accelNode, AbstractBuffNode_t *buffNode,
				uint32_t id, uint8_t type, uint32_t offset, uint32_t transactionSize, 
				uint8_t transactionIndex, uint8_t channel){
	AbstractLink_t *node = (AbstractLink_t *) malloc(sizeof(AbstractLink_t));
    node->id = id;
	node->accelNode = accelNode;
    node->buffNode = buffNode;
    node->type = type;
    node->transactionIndex = transactionIndex;
    node->transactionSize = transactionSize;
    node->offset = offset;
    node->channel = channel;
	return node;
}

int printLink(AbstractLink_t *node, char * json){
	return sprintf(json, "{\"id\": %d, \"accelNode\": %d, \"buffNode\": %d, \"type\": %d, \"transactionIndex\": %d, \"transactionSize\": %d, \"offset\": %d, \"channel\": %d}", 
			node->id, node->accelNode->id, node->buffNode->id, node->type, node->transactionIndex, node->transactionSize, node->offset, node->channel);
}

AbstractAccelNode_t* addInputNode(AbstractGraph_t *graph, int size){
	Element_t* element = (Element_t *) malloc(sizeof(Element_t));
	element->node =  createNode(graph->accelNodeID, IN_NODE, size, "Input");
	graph->accelNodeID ++;
    element->head = NULL;
    element->tail = NULL;
	addElement(&(graph->accelNodeHead), element);
	return element->node; 
}

AbstractAccelNode_t* addOutputNode(AbstractGraph_t *graph, int size){
	Element_t* element = (Element_t *) malloc(sizeof(Element_t));
	element->node =  createNode(graph->accelNodeID, OUT_NODE, size, "Output");
	graph->accelNodeID ++;
    element->head = NULL;
    element->tail = NULL;
	addElement(&(graph->accelNodeHead), element);
	return element->node; 
}

AbstractAccelNode_t* addAcceleratorNode(AbstractGraph_t *graph, char *name){
	Element_t* element = (Element_t *) malloc(sizeof(Element_t));
	element->node =  createNode(graph->accelNodeID, HW_NODE, 0, name);
	graph->accelNodeID ++;
	graph->accelCount ++;
    element->head = NULL;
    element->tail = NULL;
	addElement(&(graph->accelNodeHead), element);
	return element->node; 
}

AbstractBuffNode_t* addBuffer(AbstractGraph_t *graph, int size, int type){
	Element_t* element = (Element_t *) malloc(sizeof(Element_t));
	element->node =  createBNode(graph->buffNodeID, type, size, "Buffer");
	graph->buffNodeID ++; 
    element->head = NULL;
    element->tail = NULL;
	addElement(&(graph->buffNodeHead), element);
	return element->node; 
}

AbstractLink_t *addOutBuffer(AbstractGraph_t *graph, AbstractAccelNode_t *accelNode, AbstractBuffNode_t *buffNode,
			    uint32_t offset, uint32_t transactionSize, uint8_t transactionIndex, uint8_t channel){
	Element_t* element = (Element_t *) malloc(sizeof(Element_t));
	element->node =  createLink(accelNode, buffNode, graph->linkID, 1, offset, transactionSize, transactionIndex, channel);
	graph->linkID ++; 
    element->head = NULL;
    element->tail = NULL;
	addElement(&(graph->linkHead), element);
	return element->node; 
}

AbstractLink_t *addInBuffer(AbstractGraph_t *graph, AbstractAccelNode_t *accelNode, AbstractBuffNode_t *buffNode,
			    uint32_t offset, uint32_t transactionSize, uint8_t transactionIndex, uint8_t channel){
	Element_t* element = (Element_t *) malloc(sizeof(Element_t));
	element->node =  createLink(accelNode, buffNode, graph->linkID, 0, offset, transactionSize, transactionIndex, channel);
	graph->linkID ++;
        element->head = NULL;
        element->tail = NULL;
	addElement(&(graph->linkHead), element);
	return element->node; 
}


int abstractGraph2Json(AbstractGraph_t *graph, char* json){
	int len = 0;
    uint32_t id = graph->id;
    uint8_t type = graph->type;
        
	Element_t *accelNodeHead = graph->accelNodeHead, *accelNode;
    Element_t *buffNodeHead = graph->buffNodeHead, *buffNode;
    Element_t *linkHead = graph->linkHead, *link;
	len += sprintf(json + len, "{\"id\": %d, \"type\": %x,\n", id, type);
	accelNode = accelNodeHead;
	len += sprintf(json + len, "\"accelNode\": [\n");
	while(accelNode != NULL){
		len += printAccelNode(accelNode->node, json + len);
		if(accelNode->tail){
			len += sprintf(json + len, ",\n");
		}
		accelNode = accelNode->tail;
	}
	len += sprintf(json + len, "],\n");

	buffNode = buffNodeHead;
	len += sprintf(json + len, "\"buffNode\": [\n");
	while(buffNode != NULL){
		len += printBuffNode(buffNode->node, json + len);
		if(buffNode->tail){
			len += sprintf(json + len, ",\n");
		}
		buffNode = buffNode->tail;
	}
	len += sprintf(json + len, "],\n");
	link = linkHead;
	len += sprintf(json + len, "\"links\": [\n");
	while(link != NULL){
		len += printLink(link->node, json + len);
		if(link->tail){
			len += sprintf(json + len, ",\n");
		}
		link = link->tail;
	}
	len += sprintf(json + len, "]\n");
	
	len += sprintf(json + len, "}\n");
	printf("%s\n", json);
	return len;
}

int abstractGraphConfig(AbstractGraph_t *graph){
	char json[1024*32];
	int len;
	int fd[25];
	int fdcount = 0;
	int status;
	graph->gs = malloc(sizeof(socket_t));	
	INFO("\n");
	len = abstractGraph2Json(graph, json);
	INFO("\n");
        status = initSocket(graph->gs);
	INFO("\n");
	if(status < 0){
		return -1;
	}
	//INFO("\n");
//	INFO("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
        status = graphClientSubmit(graph->gs, json, len, fd, &fdcount);
	if (status < 0){
		return -1;
	}
	//INFO("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	//for(int i = 0; i < fdcount; i++){
	//	INFO("%d\n",fd[i]);
	//}
	reallocateIOBuffers(graph, fd, fdcount);
	return 0;
}


int abstractGraphFinalise(AbstractGraph_t *graph){
	char json[1024*4];
	int len;
	//len = abstractGraph2Json(graph, json);
	memset(json, '\0', 1024*4);
	len = sprintf(json, "{\"id\": %d}", graph->id);
	//INFO("%d %s\n", len, json);
    graphClientFinalise(graph->gs, json, len);
	appFinaliseIPBuffers(graph);
	//printf("abstractGraphFinalise\n");
	
    while(graph->linkHead != NULL){
        free((AbstractLink_t *)graph->linkHead->node);
        graph->linkHead->node = NULL;
        delElement(&(graph->linkHead), graph->linkHead);
    }
    while(graph->buffNodeHead != NULL){
        free((AbstractBuffNode_t *)graph->buffNodeHead->node);
        graph->buffNodeHead->node = NULL;
        delElement(&(graph->buffNodeHead), graph->buffNodeHead);
    }
    while(graph->accelNodeHead != NULL){
        free((AbstractAccelNode_t *)graph->accelNodeHead->node);
        graph->accelNodeHead->node = NULL;
        delElement(&(graph->accelNodeHead), graph->accelNodeHead);
    }
    free(graph);
	return 0;
}

int appFinaliseIPBuffers(AbstractGraph_t *graph){
	Element_t *element	= graph->accelNodeHead;
	while (element != NULL){
		AbstractAccelNode_t *node = element->node;
		if(node->type == IN_NODE || node->type == OUT_NODE){
			char SemaphoreName[100];
			memset(SemaphoreName, '\0', 100);
			sprintf(SemaphoreName, "%d", node->semaphore);
			sem_close(node->semptr);
			sem_unlink(SemaphoreName);
			node->semptr = NULL;	
			//INFO("unmapBuffer fd : %d size: %d ptr: %p\n", node->fd, node->size, &node->ptr);
			unmapBuffer(node->fd, node->size, &node->ptr);
		}
		element = element->tail;
	}
	return 0;
}

int reallocateIOBuffers(AbstractGraph_t *graph, int* fd, int fdcount){
	int i = 0;
	_unused(fdcount);
	Element_t *element	= graph->accelNodeHead;
	while (element != NULL){
		AbstractAccelNode_t *node = element->node;
		if(node->type == IN_NODE || node->type == OUT_NODE){
			node->fd = fd[i];
			mapBuffer(node->fd, node->size, &node->ptr);
			i ++;
			//INFO("%p\n", node);
			//INFO("node->fd = %d\n", node->fd);
			//INFO("node->size = %d\n", node->size);
			//INFO("node->ptr = %p\n", node->ptr);
			//INFO("node->semaphore = %d\n", node->semaphore);
			char SemaphoreName[100];
			memset(SemaphoreName, '\0', 100);
			sprintf(SemaphoreName, "%d", node->semaphore);
			//INFO("%s\n", SemaphoreName);
			node->semptr = sem_open(SemaphoreName, /* name */
               				O_CREAT,       /* create the semaphore */
                			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,   /* protection perms */
                			0);            /* initial value */
        		if (node->semptr == ((void*) -1)){ 
				INFO("sem_open\n");
			}
		}
		element = element->tail;
	}
	return 0;
}

int allocateIOBuffers(AbstractGraph_t *graph, int* fd){
	int status;
	//INFO("graph ID: %d\n", graph->id);
	int fdcnt = 0;

	if(!graph->xrt_fd){
		graph->xrt_fd = open("/dev/dri/renderD128",  O_RDWR);
		if (graph->xrt_fd < 0) {
			return -1;
		}
	}
	Element_t *element	= graph->accelNodeHead;
	while (element != NULL){
		AbstractAccelNode_t *node = element->node;
		if(node->type == IN_NODE || node->type == OUT_NODE){
			status = xrt_allocateBuffer(graph->xrt_fd, node->size, &node->handle,
		       		&node->ptr, &node->phyAddr, &node->fd);
      			if(status < 0){
				printf( "error @ config allocation\n");
				return status;
			}
			char SemaphoreName[100];
			memset(SemaphoreName, '\0', 100);
			sprintf(SemaphoreName, "%d", node->semaphore);
			//INFO("%s\n", SemaphoreName);
			node->semptr = sem_open(SemaphoreName, /* name */
               				O_CREAT,       /* create the semaphore */
                			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,   /* protection perms */
                			0);            /* initial value */
        		if (node->semptr == ((void*) -1)){ 
				INFO("sem_open\n");
			}
			fd[fdcnt] = node->fd;
			fdcnt ++;
			//INFO("io allocated : size = %d handle = %d ptr = %p paddr = %lx\n", 
			//	node->size, node->handle, node->ptr, node->phyAddr);
		}
		element = element->tail;
	}
	return fdcnt;
}

int deallocateIOBuffers(AbstractGraph_t *graph){
	int status;
	//INFO("graph ID: %d\n", graph->id);
	Element_t *element	= graph->accelNodeHead;
	while (element != NULL){
		AbstractAccelNode_t *node = element->node;
		if(node->type == IN_NODE || node->type == OUT_NODE){
			char SemaphoreName[100];
			memset(SemaphoreName, '\0', 100);
			sprintf(SemaphoreName, "%d", node->semaphore);
			sem_close(node->semptr);
			sem_unlink(SemaphoreName);
			node->semptr = NULL;	
		//	INFO("unmapBuffer fd : %d size: %d ptr: %p\n", node->fd, node->size, &node->ptr);
                        unmapBuffer(node->fd, node->size, &node->ptr);
			status = xrt_deallocateBuffer(graph->xrt_fd, node->size, &node->handle, &node->ptr, &node->phyAddr);
      			if(status < 0){
				printf( "error @ config deallocation\n");
				return status;
			}
			//INFO("io deallocated");
		}
		element = element->tail;
	}
	return 0;
}

int abstractGraphServerConfig(JobScheduler_t *scheduler, char* json, int len, int* fd){
	char json2[1024*32];
	_unused(len);
	int status;
	int fdcnt;
	//INFO("%s\n", json);
	//INFO("%p\n", scheduler->graphList);
	AbstractGraph_t *graph = malloc(sizeof(AbstractGraph_t));
        Element_t* element = (Element_t *) malloc(sizeof(Element_t));
	status = graphParser(json, &graph);
	if(status == 1){
		INFO("Error Occured !!");
		return 1;
	}
	//INFO("\n");
	graph->state = AGRAPH_PREINIT;	
	abstractGraph2Json(graph, json2);
	fdcnt = allocateIOBuffers(graph, fd);
	
	//INFO("\n");
        element->node =  graph;
        element->head = NULL;
        element->tail = NULL;
	//INFO("%p\n", scheduler->graphList);
	if(scheduler->graphList == NULL){
		scheduler->graphList = element;
	}
	else{
        	addElement(&(scheduler->graphList), element);
	}
	//INFO("\n");
	jobSchedulerSubmit(scheduler, element);
	graph->state = AGRAPH_INIT;	
	return fdcnt;
}

Element_t *searchGraphById(Element_t **GraphList, uint32_t id){
	Element_t *graphElement = *GraphList;
	Element_t * graph = NULL;
	while(1){
		if(id == ((AbstractGraph_t *)graphElement->node)->id){
			graph = graphElement;
			break;
		}
		if(graphElement->tail){
			graphElement = graphElement->tail;
		}
		else break;
	}
	return graph;
}

int abstractGraphServerFinalise(JobScheduler_t *scheduler, char* json){
	int id = graphIDParser(json);
	Element_t *graphNode = searchGraphById(&(scheduler->graphList), id);
	jobSchedulerRemove(scheduler, graphNode);
	AbstractGraph_t *graph = (AbstractGraph_t *)graphNode->node;
	printf("abstractGraphFinalise\n");
	deallocateIOBuffers(graph);
	printf("abstractGraphFinalise\n");
        while(graph->linkHead != NULL){
		free((AbstractLink_t *)graph->linkHead->node);
		graph->linkHead->node = NULL;
		delElement(&(graph->linkHead), graph->linkHead);
        }
        while(graph->buffNodeHead != NULL){
		free((AbstractBuffNode_t *)graph->buffNodeHead->node);
		graph->buffNodeHead->node = NULL;
		delElement(&(graph->buffNodeHead), graph->buffNodeHead);
        }
        while(graph->accelNodeHead != NULL){
		free((AbstractAccelNode_t *)graph->accelNodeHead->node);
		graph->accelNodeHead->node = NULL;
		delElement(&(graph->accelNodeHead), graph->accelNodeHead);
        }
	free(graph);
	graph = NULL;
	//INFO("%p %p\n", graphNode->head, graphNode->tail);
	delElement(&(scheduler->graphList), graphNode);
	return 0;
}

int Data2IO(AbstractAccelNode_t *node, uint8_t *ptr, int size){
	memcpy(node->ptr, ptr, size);
        sem_post(node->semptr);
	return 0;
}

int IO2Data(AbstractAccelNode_t *node, uint8_t *ptr, int size){
        sem_wait(node->semptr);
	memcpy(ptr, node->ptr, size);
	return 0;
}
