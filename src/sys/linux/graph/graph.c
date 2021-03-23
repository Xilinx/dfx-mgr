/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include "layer0/sihaHLS/sihaHLSdm.h"
#include "layer0/softIO/softdm.h"
#include "layer0/fallback/fallback.h"
//#include "dm.h"
#include "graph.h"
#include "layer0/utils.h"
#include "layer0/debug.h"
#include "layer0/siha.h"
#include "scheduler.h"


#include <fcntl.h>

AccelNode_t* createAccelNode(char *name, int inDmaType, 
				int outDmaType, FALLBACKFUNCTION fallbackfunction,
				int InterRMCompatible, int SchedulerBypassFlag){
	//INFO("\n"); 
	AccelNode_t *nextAccel = (AccelNode_t *) malloc(sizeof(AccelNode_t));
	strcpy(nextAccel->accel.name, name);
	nextAccel->accel.inDmaType = inDmaType;
	nextAccel->accel.outDmaType = outDmaType;
	nextAccel->accel.fallbackfunction = fallbackfunction;
	nextAccel->accel.InterRMCompatible = InterRMCompatible;
	nextAccel->accel.SchedulerBypassFlag = SchedulerBypassFlag;
	nextAccel->S2MMStatus = 0;
	nextAccel->MM2SStatus = 0;
	nextAccel->currentTransactionIndex = 0;
	nextAccel->head = NULL;
	nextAccel->tail = NULL;
	//INFO("\n"); 
	return nextAccel;
}

AccelNode_t* addAccelNode(AccelNode_t **accelNode, AccelNode_t *nextAccel){
	//INFO("\n"); 
	int index = -1;
	AccelNode_t *accel = *accelNode;
	if(accel != NULL){
		if(index < accel->accel.index){
			index = accel->accel.index;
		}
		while((accel)->tail != NULL){
			accel = accel->tail;
			if(index < accel->accel.index){
				index = accel->accel.index;
			}
				
		} 
		accel->tail = nextAccel;
		nextAccel->head = accel;
		nextAccel->accel.index = index;
	}
	else{
		*accelNode = nextAccel;
	}
	nextAccel->accel.index = index + 1;
	if(nextAccel->accel.type == IO_NODE || nextAccel->accel.type == IN_NODE || nextAccel->accel.type == OUT_NODE){
		nextAccel->accel.inHardware = 0;
		nextAccel->accel.datamover = malloc(sizeof(dm_t));
		nextAccel->accel.status = 0;
		soft_register(nextAccel->accel.datamover);
		//INFO("%p\n", nextAccel->accel.semptr);
                nextAccel->accel.datamover->config(nextAccel->accel.datamover->dmstruct, &(nextAccel->accel)); 
                ///TaskInit(nextAccel->accel.datamover);

	}

	else if(nextAccel->accel.type == SW_NODE){
		//INFO("SW NODE SELECTED\n");
		nextAccel->accel.inHardware = 0;
                nextAccel->accel.datamover = malloc(sizeof(dm_t));
                nextAccel->accel.status = 0;
                fallback_register(nextAccel->accel.datamover, 1, 1, nextAccel->accel.fallbackfunction);
                //nextAccel->accel.datamover->config(nextAccel->accel.datamover->dmstruct, nextAccel->accel.dma_hls);
                nextAccel->accel.datamover->config(nextAccel->accel.datamover->dmstruct, &(nextAccel->accel)); //.dma_hls);
                //TaskInit(nextAccel->accel.datamover);
	}

	else if(nextAccel->accel.type == HW_NODE){
		INFO("%d, %s\n", nextAccel->accel.index, nextAccel->accel.name);
		int status = SIHAInitAccel(nextAccel->accel.index, nextAccel->accel.name);
		if(status < 0){
			nextAccel->accel.inHardware = 0;
			
                        nextAccel->accel.datamover = malloc(sizeof(dm_t));
                        nextAccel->accel.status = 0;
                	fallback_register(nextAccel->accel.datamover, 1, 1, nextAccel->accel.fallbackfunction);
                        //nextAccel->accel.datamover->config(nextAccel->accel.datamover->dmstruct, nextAccel->accel.dma_hls);
                        nextAccel->accel.datamover->config(nextAccel->accel.datamover->dmstruct, &(nextAccel->accel)); //.dma_hls);
                        //TaskInit(nextAccel->accel.datamover);
		}
		else{
			nextAccel->accel.inHardware = 1;
			//INFO("$$$\n");
			plDevices_t* pldevices = SIHAGetPLDevices();
			nextAccel->accel.AccelConfig_fd = pldevices->AccelConfig_fd[nextAccel->accel.index];
			nextAccel->accel.dma_hls_fd = pldevices->dma_hls_fd[nextAccel->accel.index];
			nextAccel->accel.AccelConfig = pldevices->AccelConfig[nextAccel->accel.index];
			nextAccel->accel.dma_hls = pldevices->dma_hls[nextAccel->accel.index];
			nextAccel->accel.slot = pldevices->slot[nextAccel->accel.index];
			nextAccel->accel.datamover = malloc(sizeof(dm_t));
			nextAccel->accel.status = 0;
			sihahls_register(nextAccel->accel.datamover);
			sihahls_DMConfig_t* dmconfig = (sihahls_DMConfig_t*)nextAccel->accel.datamover->dmstruct;
			_unused(dmconfig);
			nextAccel->accel.datamover->config(nextAccel->accel.datamover->dmstruct, &(nextAccel->accel));
			//TaskInit(nextAccel->accel.datamover);
		}

	}

	return nextAccel;
}

int delAccelNode(AccelNode_t** accelNode){
	//INFO("\n"); 
	AccelNode_t *tAccelNode = *accelNode;
	AccelNode_t *headAccel = tAccelNode->head;
	AccelNode_t *tailAccel = tAccelNode->tail;
	
	if(headAccel != NULL){
		headAccel->tail = tailAccel;
	}
	else{
		*accelNode = tailAccel;
	}
	if(tailAccel != NULL){
		tailAccel->head = headAccel;
	}
	if(tAccelNode->accel.type == IO_NODE || tAccelNode->accel.type == IN_NODE || tAccelNode->accel.type == OUT_NODE){
		//TaskFinalise(tAccelNode->accel.datamover);
		soft_unregister(tAccelNode->accel.datamover);
		free(tAccelNode->accel.datamover);
	}
	else if(tAccelNode->accel.type == SW_NODE){
		//TaskFinalise(tAccelNode->accel.datamover);
		fallback_unregister(tAccelNode->accel.datamover);
		free(tAccelNode->accel.datamover);
	}
	else if(tAccelNode->accel.type == HW_NODE){
		if(tAccelNode->accel.inHardware){
			//TaskFinalise(tAccelNode->accel.datamover);
			sihahls_unregister(tAccelNode->accel.datamover);
			free(tAccelNode->accel.datamover);
			SIHAFinaliseAccel(tAccelNode->accel.index);
			//INFO("############\n");
		}
		else {
			//TaskFinalise(tAccelNode->accel.datamover);
			fallback_unregister(tAccelNode->accel.datamover);
			free(tAccelNode->accel.datamover);
		}
	}
	free(tAccelNode);
	return 0;
}

int printAccelInfo(Accel_t accel, char* json){
	//INFO("\n"); 
	int len = sprintf(json, "{\"node\": \"%s_%x\", \"type\": %d, \"inDmaType\": %d, \"outDmaType\": %d", 
			accel.name, accel.index, accel.type, accel.inDmaType, accel.outDmaType);
	if(accel.AccelConfig){
		len += sprintf(json + len, ", \"AccelConfig_fd\": %d, \"dma_hls_fd\": %d, \"AccelConfig\": \"%p\", \"dma_hls\": \"%p\", \"slot\": %d, \"InterRMCompatible\": %d}", accel.AccelConfig_fd, accel.dma_hls_fd, accel.AccelConfig, accel.dma_hls, accel.slot, accel.InterRMCompatible);
	}
	else{
		len += sprintf(json + len, "}");
	}
	
	return len;
}

int printAccelNodes(AccelNode_t *accelNode, char* json){
	//INFO("\n"); 
	int len = 0;
	if(accelNode != NULL){
		len += printAccelInfo(accelNode->accel, json);
		if(accelNode->tail != NULL){
			len += sprintf(json + len, ",\n");
		}
		len += printAccelNodes(accelNode->tail, json + len);
	}
	return len;
}

int printAccelNodesInfo(AccelNode_t *accelNode, char* json){
	//INFO("\n"); 
	int len = 0;
	len += sprintf(json, "{\"nodes\": [");
	len += printAccelNodes(accelNode, json + len);
	len += sprintf(json + len, "],\n");
	return len;
}

BuffNode_t* createBuffNode(int size, char *name, int type){
	//INFO("\n"); 
	BuffNode_t *nextBuff = (BuffNode_t *) malloc(sizeof(BuffNode_t));
	nextBuff->head = NULL;
	nextBuff->tail = NULL;
	nextBuff->status = 0;
	nextBuff->buffer.size = size;
	nextBuff->buffer.type = type;
	nextBuff->buffer.fd   = 0;
	nextBuff->buffer.handle  = 0;
	nextBuff->buffer.ptr     = NULL;
	nextBuff->buffer.phyAddr = 0;
	nextBuff->buffer.srcSlot = 0;
	nextBuff->buffer.sincSlot = 0;
	//nextBuff->buffer.otherAccel_phyAddr[0] = 0x080000000;
	//nextBuff->buffer.otherAccel_phyAddr[1] = 0x100000000;
	//nextBuff->buffer.otherAccel_phyAddr[2] = 0x140000000;
	nextBuff->buffer.otherAccel_phyAddr[0] = 0x200000000;
	nextBuff->buffer.otherAccel_phyAddr[1] = 0x280000000;
	nextBuff->buffer.otherAccel_phyAddr[2] = 0x300000000;
	nextBuff->buffer.readerCount = 0;
        nextBuff->buffer.readStatus = 0;
        nextBuff->buffer.writeStatus = 0;
	nextBuff->buffer.InterRMCompatible = 0;
	strcpy(nextBuff->buffer.name, name);
	return nextBuff;
}

BuffNode_t* addBuffNode(BuffNode_t** buffNode, BuffNode_t* nextBuff){
	//INFO("\n"); 
	int index = -1;
	BuffNode_t *buff = *buffNode;
	if(buff != NULL){
		if(index < buff->buffer.index){
			index = buff->buffer.index;
		}
		while(buff->tail != NULL){
			buff = buff->tail;
			if(index < buff->buffer.index){
				index = buff->buffer.index;
			}
		} 
		buff->tail = nextBuff;
		nextBuff->head = buff;
	}
	else{
		*buffNode = nextBuff;
	}
	nextBuff->buffer.index = index + 1;
	//printf("%d\n", nextBuff->buffer.index);
	return nextBuff;
}

int delBuffNode(BuffNode_t** buffNode){
	//INFO("\n"); 
	BuffNode_t *tBuffNode = *buffNode;
	BuffNode_t *headBuff = tBuffNode->head;
	BuffNode_t *tailBuff = tBuffNode->tail;
	
	if(headBuff != NULL){
		headBuff->tail = tailBuff;
	}else{
		*buffNode = tailBuff;
	}
	if(tailBuff != NULL){
		tailBuff->head = headBuff;
	}
	free(tBuffNode);
	return 0;
}

int printBuffInfo(Buffer_t buffer, char* json){
	//INFO("\n"); 
	int len = sprintf(json, "{\"node\": \"%s_%x\", \"type\": %d, ", buffer.name, buffer.index, buffer.type); 
	len += sprintf(json + len, "\"fd\": %d, \"size\": %d, \"handle\": %d, ", buffer.fd, buffer.size, buffer.handle);
	len += sprintf(json + len, "\"ptr\": \"%p\", \"phyAddr\": \"%lx\", \"InterRMCompatible\": %d}", buffer.ptr, buffer.phyAddr, buffer.InterRMCompatible);
	return len;
}

int printBuffNodes(BuffNode_t *buffNode, char* json){
	//INFO("\n"); 
	int len = 0;
	if(buffNode != NULL){
		len += printBuffInfo(buffNode->buffer, json);
		if(buffNode->tail != NULL){
			len += sprintf(json + len, ",\n");
		}
		len += printBuffNodes(buffNode->tail, json + len);
	}
	return len;
}

int printBuffNodesInfo(BuffNode_t *buffNode, char* json){
	//INFO("\n"); 
	int len = 0;
	len += sprintf(json, "\"buffnodes\": [");
	len += printBuffNodes(buffNode, json + len);
	len += sprintf(json + len, "],\n");
	return len;
}

Link_t* addLink(Link_t** link, Link_t* nextLink){
	//INFO("\n"); 
	Link_t *tlink = *link;
	if(tlink != NULL){
		while(tlink->tail != NULL){
			tlink = tlink->tail;
		} 
		tlink->tail = nextLink;
		nextLink->head = tlink;
	}
	else{
		*link = nextLink;
	}
	return nextLink;
}

int delLink(Link_t** link){
	//INFO("\n"); 
	Link_t *tLink = *link;
	Link_t *headLink = tLink->head;
	Link_t *tailLink = tLink->tail;
	if(headLink != NULL){	
		headLink->tail = tailLink;
	}else{
		*link = tailLink;
	}
	if(tailLink != NULL){
		tailLink->head = headLink;
	}
	free(tLink);
	
	return 0;
}

int printLinkInfo(Link_t *link, char *json){
	//INFO("\n"); 
	int len = 0;
	len += sprintf(json + len, "{");
	if(link != NULL){
		if(link->type == 1){
			len += sprintf(json + len, "\"out\": \"%s_%x\", ", link->accelNode->accel.name,
				link->accelNode->accel.index);
			len += sprintf(json + len, "\"in\": \"%s_%x\", ", link->buffNode->buffer.name,
				link->buffNode->buffer.index);
		}
		else{
			len += sprintf(json + len, "\"out\": \"%s_%x\", ", link->buffNode->buffer.name,
				link->buffNode->buffer.index);
			len += sprintf(json + len, "\"in\": \"%s_%x\", ", link->accelNode->accel.name,
				link->accelNode->accel.index);
		}
		len += sprintf(json + len, "\"channel\": \"0x%x\", ", link->channel);
		len += sprintf(json + len, "\"transactionSize\": \"0x%x\", ", link->transactionSize);
		len += sprintf(json + len, "\"transactionIndex\": \"0x%x\" ", link->transactionIndex);
		len += sprintf(json + len, "}");
	}
	return len;
}

int printLinks(Link_t *link, char *json){
	//INFO("\n"); 
	int len = 0;
	if(link != NULL){
		len += printLinkInfo(link, json + len);
		if(link->tail != NULL){
			len += sprintf(json + len, ",\n");
		}
		len += printLinks(link->tail, json + len);
	}
	return len;
}

int printLinksInfo(Link_t *link, char* json){
	//INFO("\n"); 
	int len = 0;
	len += sprintf(json + len, "\"links\":[");
	len += printLinks(link, json + len);
	len += sprintf(json + len, "]}\n");
	return len;
}

Schedule_t* createSchedule(DependencyList_t *dependency, int offset, int size, int first, int last){ //, BuffNode_t *dependentBuffNode){
	//INFO("\n"); 
	Schedule_t* schedule = (Schedule_t *) malloc(sizeof(Schedule_t));
	schedule->dependency = dependency;
	schedule->size = size;
	schedule->offset = offset;
	schedule->status = 0;
	schedule->first = first;
	schedule->last = last;
	//schedule->dependentBuffNode = dependentBuffNode;
	schedule->head = NULL;
	schedule->tail = NULL;
	return schedule;
}

Schedule_t* addSchedule(Schedule_t** schedule, Schedule_t* nextSchedule){
	//INFO("\n"); 
	Schedule_t *tschedule = *schedule;
	if(tschedule != NULL){
		while(tschedule->tail != NULL){
			tschedule = tschedule->tail;
		} 
		tschedule->tail = nextSchedule;
		nextSchedule->head = tschedule;
	}
	else{
		*schedule = nextSchedule;
	}
	return nextSchedule;
}

int delSchedule(Schedule_t** schedule, Schedule_t** scheduleHead){
	Schedule_t *tSchedule = *schedule;
	Schedule_t *headSchedule = tSchedule->head;
	Schedule_t *tailSchedule = tSchedule->tail;
	if(headSchedule != NULL){	
		*schedule = tailSchedule;
		headSchedule->tail = tailSchedule;
	}else{
		*schedule = tailSchedule;
		*scheduleHead = tailSchedule;
		//INFO("%p\n", *schedule);
	}
	if(tailSchedule != NULL){
		
		tailSchedule->head = headSchedule;
	}
	free(tSchedule);
	return 0;
}

int printSchedule(Schedule_t* schedule){
	//INFO("\n"); 
	//char json[1000];
	if(schedule != NULL){
		while(1){
			//printDepend(schedule->dependency);
			INFOP("%s%d ", 
				schedule->dependency->link->accelNode->accel.name, 
				schedule->dependency->link->accelNode->accel.index);
			if(schedule->dependency->link->type){
				INFOP("====> ");
			}else{
				INFOP("<==== ");
			}
			INFOP("%s%d ", 
				schedule->dependency->link->buffNode->buffer.name, 
				schedule->dependency->link->buffNode->buffer.index);
			//if(schedule->dependentBuffNode){
			//	INFOP("depends on %s%d\n", 
			//		schedule->dependentBuffNode->buffer.name, 
			//		schedule->dependentBuffNode->buffer.index);
			//}
			//else{
				INFOP("\tfirst: %d last: %d\n", schedule->first, schedule->last); 
			//}
			if(schedule->tail != NULL){
				schedule = schedule->tail;
			}
			else break;
		}
	}
	return 0;
}

Link_t* addInputBuffer(AccelNode_t *accelNode, BuffNode_t *buffNode,
                       int offset, int transactionSize, int transactionIndex, int channel){
	//INFO("\n"); 
	Link_t* link = (Link_t *) malloc(sizeof(Link_t));
	link->accelNode = accelNode;
	link->buffNode  = buffNode;
	link->transactionSize = transactionSize;
	link->offset = offset;
	link->totalTransactionDone = 0;
	link->totalTransactionScheduled = 0;
	link->transactionIndex = transactionIndex;
	link->channel = channel; 
	link->accounted = 0; 
	link->type      = 0;
	link->head = NULL;
	link->tail = NULL;
	buffNode->buffer.readerCount += 1;
	if(accelNode->accel.InterRMCompatible == INTER_RM_COMPATIBLE){
		buffNode->buffer.InterRMCompatible += 1;
		//buffNode->buffer.srcSlot = accelNode->accel.slot;
		buffNode->buffer.sincSlot = accelNode->accel.slot;
	}
	return link;
}
	
Link_t* addOutputBuffer(AccelNode_t *accelNode, BuffNode_t *buffNode,
                        int offset, int transactionSize, int transactionIndex, int channel){
	//INFO("\n"); 
	Link_t* link = (Link_t *) malloc(sizeof(Link_t));
	link->accelNode = accelNode;
	link->buffNode  = buffNode;
	link->transactionSize = transactionSize;
	link->offset = offset;
	link->totalTransactionDone = 0;
	link->totalTransactionScheduled = 0;
	link->transactionIndex = transactionIndex;
	link->channel = channel; 
	link->accounted = 0; 
	link->type      = 1;
	link->head = NULL;
	link->tail = NULL;
	//INFO("\n");
//	INFO("%p\n", accelNode); //->accel.InterRMCompatible); 
	if(accelNode->accel.InterRMCompatible == INTER_RM_COMPATIBLE){
		buffNode->buffer.InterRMCompatible += 1;
		buffNode->buffer.srcSlot = accelNode->accel.slot;
		//buffNode->buffer.sincSlot = accelNode->accel.slot;
	}
	//INFO("\n"); 
	return link;
}

int updateBuffers(AcapGraph_t* graph, Link_t* linkHead){
	Link_t* link = linkHead;
	Buffers_t* buffers = SIHAGetBuffers();
	//INFO("%p\n", buffers);
	if (link != NULL){
		if(link->buffNode->buffer.type == PL_BASED && link->buffNode->buffer.InterRMCompatible == 2){
		}
		else if(buffers != NULL && link->type == 0 && link->buffNode->buffer.fd == 0 && link->accelNode->accel.inHardware){
			//INFO("%s %d %p\n", link->accelNode->accel.name, link->accelNode->accel.index, link->tail);
			//INFO("%x : %x\n", link->buffNode->buffer.size, buffers->MM2S_size[link->accelNode->accel.index]);
			if(link->buffNode->buffer.size == buffers->MM2S_size[link->accelNode->accel.index]){
				link->buffNode->buffer.fd      = buffers->MM2S_fd    [link->accelNode->accel.index];
				link->buffNode->buffer.size    = buffers->MM2S_size  [link->accelNode->accel.index];
				link->buffNode->buffer.handle  = buffers->MM2S_handle[link->accelNode->accel.index];
				link->buffNode->buffer.ptr     = buffers->MM2S_ptr   [link->accelNode->accel.index];
				link->buffNode->buffer.phyAddr = buffers->MM2S_paddr [link->accelNode->accel.index];
			}
			else if(link->buffNode->buffer.size == buffers->config_size[link->accelNode->accel.index]){
				link->buffNode->buffer.fd      = buffers->config_fd    [link->accelNode->accel.index];
				link->buffNode->buffer.size    = buffers->config_size  [link->accelNode->accel.index];
				link->buffNode->buffer.handle  = buffers->config_handle[link->accelNode->accel.index];
				link->buffNode->buffer.ptr     = buffers->config_ptr   [link->accelNode->accel.index];
				link->buffNode->buffer.phyAddr = buffers->config_paddr [link->accelNode->accel.index];
			}
			link->buffNode->buffer.readctr[0] = 0;
			link->buffNode->buffer.readctr[1] = 0;
			link->buffNode->buffer.writectr[0] = 0;
			link->buffNode->buffer.writectr[1] = 0;
			link->buffNode->buffer.status = EMPTY;
                        link->buffNode->readStatus = 0;
			//INFO("%s%d %d %d %p %lx\n", link->buffNode->buffer.name, link->buffNode->buffer.index, link->buffNode->buffer.fd, link->buffNode->buffer.size, link->buffNode->buffer.ptr, link->buffNode->buffer.phyAddr);
		}
		else if(buffers == NULL){
			//INFO("%p\n", buffers);
			int tmpfd;
			xrt_allocateBuffer(graph->drmfd, link->buffNode->buffer.size, 
				&link->buffNode->buffer.handle, &link->buffNode->buffer.ptr, 
				&link->buffNode->buffer.phyAddr, &tmpfd);
		}
		if(link->tail != NULL){
			updateBuffers(graph, link->tail);
		}
	}
	return 0;
}

int updateBuffersPass2(Link_t* linkHead){
	Link_t* link = linkHead;
	Buffers_t* buffers = SIHAGetBuffers();
	//INFO("\n"); 
	if (link != NULL){
		//INFO("##%d\n", link->buffNode->buffer.fd);
		if(link->buffNode->buffer.type == PL_BASED && link->buffNode->buffer.InterRMCompatible == 2){
		}
		else if(buffers != NULL && link->buffNode->buffer.fd == 0){
			//INFO("%s %d %p\n", link->accelNode->accel.name, link->accelNode->accel.index, link->tail);

			link->buffNode->buffer.fd      = buffers->S2MM_fd    [link->accelNode->accel.index];
			link->buffNode->buffer.size    = buffers->S2MM_size  [link->accelNode->accel.index];
			link->buffNode->buffer.handle  = buffers->S2MM_handle[link->accelNode->accel.index];
			link->buffNode->buffer.ptr     = buffers->S2MM_ptr   [link->accelNode->accel.index];
			link->buffNode->buffer.phyAddr = buffers->S2MM_paddr [link->accelNode->accel.index];
                        link->buffNode->buffer.readctr[0] = 0;
                        link->buffNode->buffer.readctr[1] = 0;
                        link->buffNode->buffer.writectr[0] = 0;
                        link->buffNode->buffer.writectr[1] = 0;
                        link->buffNode->buffer.status = EMPTY;
                        link->buffNode->readStatus = 0;

			//INFO("%s%d %d %d %p %lx\n", link->buffNode->buffer.name, link->buffNode->buffer.index, link->buffNode->buffer.fd, link->buffNode->buffer.size, link->buffNode->buffer.ptr, link->buffNode->buffer.phyAddr);
		}
		if(link->tail != NULL){
			updateBuffersPass2(link->tail);
		}
	}
	return 0;
}

DependencyList_t* createDependency(Link_t* link){
	//INFO("\n"); 
	DependencyList_t* dependency = (DependencyList_t *) malloc(sizeof(DependencyList_t));
	dependency->link = link;
	dependency->linkCount = 0;
	dependency->head = NULL;
	dependency->tail = NULL;
	return dependency;
}

DependencyList_t* addDependency(DependencyList_t** dependency, DependencyList_t* nextDependency){
	//INFO("\n"); 
	DependencyList_t* tdependency = *dependency;
	if(tdependency != NULL){
		while(tdependency->tail != NULL){
			tdependency = tdependency->tail;
		} 
		tdependency->tail = nextDependency;
		nextDependency->head = tdependency;
	}
	else{
		*dependency = nextDependency;
	}
	return nextDependency;
}

int delDependency(DependencyList_t** dependency){
	//INFO("\n"); 
	DependencyList_t *tDependency = *dependency;
	DependencyList_t *headDependency = tDependency->head;
	DependencyList_t *tailDependency = tDependency->tail;
	if(headDependency != NULL){	
		headDependency->tail = tailDependency;
	}else{
		*dependency = tailDependency;
	}
	if(tailDependency != NULL){
		tailDependency->head = headDependency;
	}
	free(tDependency);
	return 0;
}

int printDepend(DependencyList_t* dependency){
	//INFO("%p\n", dependency); 
	char json[1000];
	if(dependency != NULL){
		printLinkInfo(dependency->link, json);
		//INFO("link : %s\n",json);
		for(int i=0; i < dependency->linkCount; i++){
			printLinkInfo(dependency->dependentLinks[i], json);
			//INFO("depends on : %s\n",json);
		}
	}
	return 0;
}

int printDependency(DependencyList_t* dependency){
	//INFO("\n"); 
	//char json[1000];
	if(dependency != NULL){
		while(1){
			//printDepend(dependency);
			if(dependency->tail != NULL){
				dependency = dependency->tail;
			}
			else break;
		}
	}
	return 0;
}

int updateDepend(DependencyList_t** dependencyHead, Link_t* linkHead){
        //char json[1000];
	Link_t* link = linkHead;
	DependencyList_t* dependency;
	DependencyList_t* cDependency = *dependencyHead;
	while(cDependency != NULL){
		link = linkHead;
		while(link != NULL){
			if(link->type == 0 && 
			cDependency->link->type == 1 &&
			cDependency->link->transactionIndex == link->transactionIndex){
				cDependency->dependentLinks[cDependency->linkCount] = link;
				cDependency->linkCount = cDependency->linkCount + 1;
			}
			if(link->type == 0 && 
			cDependency->link->type == 1 &&
			cDependency->link->buffNode->buffer.index == link->buffNode->buffer.index){
				if(link->accounted == 0){
					link->accounted = 1;
					dependency = createDependency(link);
					addDependency(dependencyHead, dependency);
				}
			}
			if(link->type == 1 && 
			cDependency->link->type == 0 &&
			cDependency->link->transactionIndex == link->transactionIndex){
				cDependency->dependentLinks[cDependency->linkCount] = link;
				cDependency->linkCount = cDependency->linkCount + 1;
				if(link->accounted == 0){
					link->accounted = 1;
					dependency = createDependency(link);
					addDependency(dependencyHead, dependency);
				}
			}
			link = link->tail;
		}
		cDependency = cDependency->tail;
                //INFO("$### : %s\n",json);
		link = linkHead;
	}
	return 0;
}

int updateDependency(DependencyList_t** dependencyHead, Link_t* linkHead){
        //char json[1000];
	Link_t* link = linkHead;
	DependencyList_t* dependency;
	while(link != NULL){
		if(link->accelNode->accel.type == IN_NODE){
			dependency = createDependency(link);
			addDependency(dependencyHead, dependency);
		}
		link = link->tail;
	}
	updateDepend(dependencyHead, linkHead);
	printDependency(*dependencyHead);
	return 0;
}

int graphSchedule(Schedule_t** scheduleHead, DependencyList_t* dependencyHead){
	DependencyList_t* cDependency = dependencyHead;
	Schedule_t* tSchedule;
	int tSize, first, last;
	int pending = 0;
	//for(int i = 0; i < 3; i++){//)while(1){
	while(1){
		while(cDependency != NULL){
			//INFO("$$$$$$$$$$$$$$$$$$$$##########%d\n", pending);
			//printDepend(cDependency);
			tSize = cDependency->link->transactionSize - 
				cDependency->link->totalTransactionScheduled;
			if(tSize > cDependency->link->buffNode->buffer.size){
				tSize = cDependency->link->buffNode->buffer.size;
			}
			//INFO("%x %x %x\n", tSize, cDependency->link->transactionSize, cDependency->link->totalTransactionScheduled);
			if(tSize > 0){
				if(cDependency->link->totalTransactionScheduled <= 0){
					first = 1;
				}
				else{
					first = 0;
				}
				cDependency->link->totalTransactionScheduled += tSize;
				if(cDependency->link->transactionSize -
			              cDependency->link->totalTransactionScheduled <= 0){
					last = 1;
				}
				else{
					last = 0;
				}
				tSchedule = createSchedule(cDependency, cDependency->link->offset, tSize, first, last);
				addSchedule(scheduleHead, tSchedule);
				pending += tSize;
				//INFO("======== > Scheduled\n");
			}
			cDependency = cDependency->tail;
		}
		//INFO("#######################################%d\n", pending);
		if(pending == 0) break;
		pending = 0;
		cDependency = dependencyHead;
	}
	printSchedule(*scheduleHead);
	return 0;
}
//################################################################################
//################################################################################
//         AcapdGraph
//################################################################################

AccelNode_t* acapAddAccelNode(AcapGraph_t *acapGraph, char *name, int dmaType, FALLBACKFUNCTION fallbackfunction, int InterRMCompatible, int SchedulerBypassFlag){
	//INFO("%d %p %d\n", dmaType, fallbackfunction, InterRMCompatible); 
	AccelNode_t *nextAccel = createAccelNode(name, dmaType, dmaType, fallbackfunction, InterRMCompatible, SchedulerBypassFlag);
	nextAccel->accel.type = HW_NODE;	
	return addAccelNode(&(acapGraph->accelNodeHead), nextAccel);
}

AccelNode_t* acapAddAccelNodeForceFallback(AcapGraph_t *acapGraph, char *name, int dmaType,
						 FALLBACKFUNCTION fallbackfunction, int InterRMCompatible, int SchedulerBypassFlag){
	//INFO("\n"); 
	AccelNode_t *nextAccel = createAccelNode(name, dmaType, dmaType, fallbackfunction, InterRMCompatible, SchedulerBypassFlag);
	nextAccel->accel.type = SW_NODE;	
	return addAccelNode(&(acapGraph->accelNodeHead), nextAccel);
}

int forceFallback(AccelNode_t *accelNode){
	//INFO("\n"); 
	accelNode->accel.type = SW_NODE;	
	return 0;
}

AccelNode_t* acapAddInputNode(AcapGraph_t *acapGraph, uint8_t *buff, int size, int SchedulerBypassFlag, sem_t* semptr){
	//INFO("\n"); 
	AccelNode_t *nextAccel = createAccelNode("Input", NONE, NONE, NULL, 0, SchedulerBypassFlag);
	nextAccel->accel.type = IN_NODE;	
	nextAccel->accel.softBuffer = buff;	
	nextAccel->accel.softBufferSize = size;	
	nextAccel->accel.semptr = semptr;	
	return addAccelNode(&(acapGraph->accelNodeHead), nextAccel);
}

AccelNode_t* acapAddOutputNode(AcapGraph_t *acapGraph, uint8_t *buff, int size, int SchedulerBypassFlag, sem_t* semptr){
	//INFO("\n"); 
	AccelNode_t *nextAccel = createAccelNode("Output", NONE, NONE, NULL, 0, SchedulerBypassFlag);
	nextAccel->accel.type = OUT_NODE;	
	nextAccel->accel.softBuffer = buff;	
	nextAccel->accel.softBufferSize = size;	
	nextAccel->accel.semptr = semptr;	
	return addAccelNode(&(acapGraph->accelNodeHead), nextAccel);
}

BuffNode_t* acapAddBuffNode(AcapGraph_t *acapGraph, int size, char *name, int type){
	//INFO("\n"); 
	BuffNode_t* nextBuff = createBuffNode(size, name, type);
	return addBuffNode(&(acapGraph->buffNodeHead), nextBuff);
}

Link_t *acapAddOutputBuffer(AcapGraph_t *acapGraph, AccelNode_t *accelNode, BuffNode_t *buffNode,
			    int offset, int transactionSize, int transactionIndex, int channel){
	//INFO("\n"); 
	Link_t *link = addOutputBuffer(accelNode, buffNode, offset, transactionSize, transactionIndex, channel);
	addLink(&(acapGraph->linkHead), link);
	return link;
}

Link_t *acapAddInputBuffer(AcapGraph_t *acapGraph, AccelNode_t *accelNode, BuffNode_t *buffNode,
			   int offset, int transactionSize, int transactionIndex, int channel){
	//INFO("\n"); 
	Link_t *link = addInputBuffer(accelNode, buffNode, offset, transactionSize, transactionIndex, channel);
	addLink(&(acapGraph->linkHead), link);
	return link;
}

int acapGraphToJson(AcapGraph_t *acapGraph){
	char *json = (char*) malloc(sizeof(char)*0x4000);
	int len = 0;
	//INFO("\n"); 
	FILE *fp;
	fp = fopen("/home/root/plgraph.json", "w");
	len += printAccelNodesInfo(acapGraph->accelNodeHead, json + len);
	len += printBuffNodesInfo (acapGraph->buffNodeHead,  json + len);
	len += printLinksInfo     (acapGraph->linkHead,      json + len);
	//:w
	//INFO("%s", json);
	fwrite(json, 1, len, fp);
	fclose(fp);
	return 0;
}

int acapGraphConfig(AcapGraph_t *acapGraph){
	//INFO("\n"); 
	//acapGraphToJson(acapGraph);
	updateBuffers(acapGraph, acapGraph->linkHead);
	//acapGraphToJson(acapGraph);
	updateBuffersPass2(acapGraph->linkHead);
	//acapGraphToJson(acapGraph);
	updateDependency(&(acapGraph->dependencyHead), acapGraph->linkHead);
	return 0;
}

int acapGraphSchedule(AcapGraph_t *acapGraph){
	//INFO("\n"); 
	graphSchedule(&(acapGraph->scheduleHead), acapGraph->dependencyHead);
	SchedulerTrigger(acapGraph);
	SchedulerCompletion(acapGraph);
	return 0;
}

AcapGraph_t* acapGraphInit(){
	//INFO("\n"); 
	srand(time(NULL));
	AcapGraph_t*graph = malloc(sizeof(AcapGraph_t));
	SchedulerInit(graph);
	graph->drmfd = open("/dev/dri/renderD128",  O_RDWR);
	if (graph->drmfd < 0) {
		return NULL;
	}
	return graph;
}
int acapGraphResetLinks(AcapGraph_t *acapGraph){
	SchedulerFinalise(acapGraph);
	while(acapGraph->scheduleHead != NULL){
		delSchedule(&(acapGraph->scheduleHead), &(acapGraph->scheduleHead));
	}
	while(acapGraph->dependencyHead != NULL){
		delDependency(&(acapGraph->dependencyHead));
	}
	while(acapGraph->linkHead != NULL){
		delLink(&(acapGraph->linkHead));
	}
	return 0;
}

int acapGraphFinalise(AcapGraph_t *acapGraph){
	//INFO("\n"); 
	SchedulerFinalise(acapGraph);
	while(acapGraph->linkHead != NULL){
		delLink(&(acapGraph->linkHead));
	}
	acapGraph->linkHead = NULL; 
	while(acapGraph->buffNodeHead != NULL){
		delBuffNode(&(acapGraph->buffNodeHead));
	}
	acapGraph->buffNodeHead = NULL; 
	while(acapGraph->accelNodeHead != NULL){
		delAccelNode(&(acapGraph->accelNodeHead));
	}
	acapGraph->accelNodeHead = NULL; 
	while(acapGraph->dependencyHead != NULL){
		delDependency(&(acapGraph->dependencyHead));
	}
	acapGraph->dependencyHead = NULL; 
	//nacapGraphToJson(acapGraph);
	free(acapGraph);
	acapGraph = NULL;
	return 0;
}
