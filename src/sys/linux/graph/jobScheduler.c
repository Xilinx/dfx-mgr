/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "graph.h"
#include "jobScheduler.h"
#include "abstractGraph.h"
//#include "graph.h"
#include "layer0/queue.h"
#include "layer0/debug.h"
#include "layer0/dfx-mgrd.h"
//#include "layer0/uio.h"
#include "metadata.h"

#define GRAPH_SUBMIT		0x30
#define GRAPH_DATA_READY	0x31
#define GRAPH_SUBMIT_FINISH	0x3F

void *jobScheduler_Task(void* carg){
        INFO("\n");
	JobScheduler_t *scheduler = (JobScheduler_t *)carg; 
        Element_t *graphList = NULL;
        AbstractGraph_t *graph = NULL;
        queue_t *commandQueue = scheduler->CommandQueue;
        queue_t *responseQueue = scheduler->ResponseQueue;
        JobQueueBuffer_t *commandQueueBuffer, *responseQueueBuffer;
	Json_t* json = malloc(sizeof(Json_t));
	Metadata_t *metadata = malloc(sizeof(Metadata_t));
       	char jsonfile[100];
	AcapGraph_t *currentGraph;
	Element_t *graphElement;
	int busy = 0;
        int enableScheduler = 0;
	_unused(busy);
	_unused(enableScheduler);
	_unused(currentGraph);
        while(1){
                usleep(1000);
                if((busy == 0) && (queue_size(commandQueue) > 0)){
                        commandQueueBuffer = queue_dequeue(commandQueue);
                        switch (commandQueueBuffer->type){
                                case GRAPH_SUBMIT:
                                        INFO("processing GRAPH_SUBMIT\n");
        				graphList = scheduler->graphList;
					graphElement = searchGraphById(&graphList, commandQueueBuffer->id);
        				graph = (AbstractGraph_t *)(graphElement->node);
					graph->state = AGRAPH_SCHEDULED;
					INFO("AGRAPH_SCHEDULED");
                                        responseQueueBuffer = malloc(sizeof(JobQueueBuffer_t));
                                        responseQueueBuffer->type = GRAPH_SUBMIT_FINISH;
                                        queue_enqueue(responseQueue, responseQueueBuffer);
                                        break;
                                //case GRAPH_DATA_READY:
                                //        INFO("processing GRAPH_SUBMIT\n");
				//	graphElement = searchGraphById(&graphList, commandQueueBuffer->id);
        			//	graph = (AbstractGraph_t *)(graphElement->node);
				//	graph->state = AGRAPH_SCHEDULED;
				//	INFO("AGRAPH_SCHEDULED");
                                  //      responseQueueBuffer = malloc(sizeof(JobQueueBuffer_t));
                                //        responseQueueBuffer->type = GRAPH_SUBMIT_FINISH;
                                 //       queue_enqueue(responseQueue, responseQueueBuffer);
                                //        break;
                                default:
                                        break;
                        }
                }
		if(graphList != NULL){
			//INFO("%p\n", graphList);
			graphElement = graphList;
			while(graphElement){
        			graph = (AbstractGraph_t *)(graphElement->node);
				if (graph->state == AGRAPH_SCHEDULED){
					currentGraph = acapGraphInit();
					INFO("no of accelerators in this graph are: %d\n", graph->accelCount);
					Element_t* accelElement = graph->accelNodeHead;
					while(accelElement != NULL){
						AbstractAccelNode_t *abstractAccel =
							(AbstractAccelNode_t *) accelElement->node;	
						INFO("%d\n", abstractAccel->id);
						switch(abstractAccel->type){
							case HW_NODE:
       								if(strcmp(abstractAccel->name, "FFT4")){
        							        strcpy(jsonfile, 
										"/media/test/home/root/accel.json");
        							}
       								else if(strcmp(abstractAccel->name, "aes128encdec")){
                							strcpy(jsonfile, 
										"/media/test/home/root/accel.json");
        							}
        							else if(strcmp(abstractAccel->name, "fir_compiler")){
             								strcpy(jsonfile, 
										"/media/test/home/root/accel.json");
        							}
 								file2json(jsonfile, json);
        							json2meta(json, metadata);
        							printMeta(metadata);
								abstractAccel->node = acapAddAccelNode(currentGraph,
									abstractAccel->name, metadata->DMA_type,
									NULL, //metadata->fallback.lib,
                                                       			metadata->interRM.compatible, 
									0); //SchedulerBypassFlag);
								INFO("%p\n", abstractAccel);
 								break;
							case IN_NODE:
								abstractAccel->node = acapAddInputNode(currentGraph,
									abstractAccel->ptr, abstractAccel->size,
									0); //SchedulerBypassFlag); 
								break;
							case OUT_NODE: 
								abstractAccel->node = acapAddOutputNode(currentGraph,
									abstractAccel->ptr, abstractAccel->size,
									0); //SchedulerBypassFlag); 
								break;
							
						}
						accelElement = accelElement->tail;
					}
					Element_t* buffElement = graph->buffNodeHead;
					while(buffElement != NULL){
						AbstractBuffNode_t *abstractBuff =
							(AbstractBuffNode_t *) buffElement->node;	
						INFO("%d\n", abstractBuff->id);
						INFO("####################### Assign Buffers\n");
						abstractBuff->node = acapAddBuffNode(currentGraph, 
											abstractBuff->size, 
											abstractBuff->name, 
											abstractBuff->type);
						buffElement = buffElement->tail;
					}
					Element_t* linkElement = graph->linkHead;
					while(linkElement != NULL){
						AbstractLink_t *abstractLink =
							(AbstractLink_t *) linkElement->node;	
						INFO("%d\n", abstractLink->id);
						INFO("%d\n", abstractLink->transactionIndex);
						INFO("####################### Assign Links\n");
						INFO("%p\n", abstractLink->accelNode);
						//INFO("%p\n", abstractLink->buffNode);
						INFO("%s\n", abstractLink->accelNode->name);
						if(abstractLink->type){
							 abstractLink->node = acapAddOutputBuffer(currentGraph, 
									abstractLink->accelNode->node, 
									abstractLink->buffNode->node, 
									abstractLink->offset, 
									abstractLink->transactionSize, 
									abstractLink->transactionIndex, 
									abstractLink->channel);
						}
						else{
							 abstractLink->node = acapAddInputBuffer(currentGraph, 
									abstractLink->accelNode->node, 
									abstractLink->buffNode->node, 
									abstractLink->offset, 
									abstractLink->transactionSize, 
									abstractLink->transactionIndex, 
									abstractLink->channel);
						}
						linkElement = linkElement->tail;
					}
					graph->state = AGRAPH_EXECUTING;
				}
				graphElement = graphElement->tail;				
			}
		}
	
	}
        return NULL;
}

JobScheduler_t * jobSchedulerInit(){
        //INFO("\n");
        JobScheduler_t *scheduler = malloc(sizeof(JobScheduler_t));
	dfx_init();
        scheduler->graphList = NULL;
	//INFO("%p\n", scheduler->graphList);
        scheduler->CommandQueue  = malloc(sizeof(queue_t));
        scheduler->ResponseQueue = malloc(sizeof(queue_t));
        *(scheduler->CommandQueue ) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(scheduler->ResponseQueue) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };

        pthread_create(scheduler->thread    , NULL, jobScheduler_Task, scheduler);
        return scheduler;
}


int jobSchedulerSubmit(JobScheduler_t *scheduler, Element_t *graphElement){
        AbstractGraph_t *graph = (AbstractGraph_t *)(graphElement->node);
        INFO("Submitting Graph with ID: %d\n", graph->id);
        JobQueueBuffer_t *commandBuff, *responseBuff;
        commandBuff = malloc(sizeof(JobQueueBuffer_t));
        commandBuff->type = GRAPH_SUBMIT;
        commandBuff->id = graph->id;
        queue_enqueue(scheduler->CommandQueue, commandBuff);
	while(1){
                responseBuff = (JobQueueBuffer_t *)queue_dequeue(scheduler->ResponseQueue);
                if(responseBuff->type == GRAPH_SUBMIT_FINISH) break;
        }
        return 0;
}
