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
#include <dfx-mgr/daemon_helper.h>
//#include "layer0/dfx-mgrd.h"
//#include "layer0/uio.h"
#include "metadata.h"
#include "layer0/utils.h"

#define GRAPH_SUBMIT		0x30
#define GRAPH_DATA_READY	0x31
#define GRAPH_FINISH		0x32
#define GRAPH_SUBMIT_DONE	0x33
#define GRAPH_FINISH_DONE	0x34

void *jobScheduler_Task(void* carg){
        //INFO("\n");
        //
        _unused(socket_d);
	JobScheduler_t *scheduler = (JobScheduler_t *)carg; 
        Element_t *graphList = NULL;
        AbstractGraph_t *graph = NULL;
        queue_t *commandQueue = scheduler->CommandQueue;
        queue_t *responseQueue = scheduler->ResponseQueue;
        JobQueueBuffer_t *commandQueueBuffer, *responseQueueBuffer;
	Json_t* json = malloc(sizeof(Json_t));
	Metadata_t *metadata = malloc(sizeof(Metadata_t));
       	char jsonfile[100];
	AcapGraph_t *currentGraph = NULL;
	Element_t *graphElement;
	int busy = 0;
        int enableScheduler = 0;
	_unused(busy);
	_unused(enableScheduler);
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
					//INFO("AGRAPH_SCHEDULED");
                                        responseQueueBuffer = malloc(sizeof(JobQueueBuffer_t));
                                        responseQueueBuffer->type = GRAPH_SUBMIT_DONE;
                                        queue_enqueue(responseQueue, responseQueueBuffer);
                                        break;
                                case GRAPH_FINISH:
                                        INFO("processing GRAPH_FINISH\n");
					graphElement = searchGraphById(&graphList, commandQueueBuffer->id);
        				graph = (AbstractGraph_t *)(graphElement->node);
				//	graph->state = AGRAPH_SCHEDULED;
				//	INFO("AGRAPH_SCHEDULED");
                                        responseQueueBuffer = malloc(sizeof(JobQueueBuffer_t));
                                        responseQueueBuffer->type = GRAPH_FINISH_DONE;
                                        queue_enqueue(responseQueue, responseQueueBuffer);
                                        break;
                                default:
                                        break;
                        }
                }
		if(graphList != NULL){
			//INFO("%p\n", graphList);
			graphElement = graphList;
			while(graphElement){
				if(graphElement!= NULL){
        			graph = (AbstractGraph_t *)(graphElement->node);
				if (graph != NULL){
					switch (graph->state){
					case AGRAPH_SCHEDULED:
						if(currentGraph != NULL){ 
							break;
						}
						currentGraph = acapGraphInit();
						//INFO("no of accelerators in this graph are: %d\n", graph->accelCount);
						Element_t* accelElement = graph->accelNodeHead;
						while(accelElement != NULL){
							AbstractAccelNode_t *abstractAccel =
								(AbstractAccelNode_t *) accelElement->node;	
							//INFO("%d\n", abstractAccel->id);
							switch(abstractAccel->type){
								case HW_NODE:
									//INFO("#####################\n");
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
        								//printMeta(metadata);
									abstractAccel->node = acapAddAccelNode(currentGraph,
										abstractAccel->name, metadata->DMA_type,
										NULL, //metadata->fallback.lib,
                                                	       			metadata->interRM.compatible, 
										0); //SchedulerBypassFlag);
									//INFO("#####################\n");
 									break;
								case IN_NODE:
									abstractAccel->node = acapAddInputNode(currentGraph,
										abstractAccel->ptr, abstractAccel->size,
										0, abstractAccel->semptr); //SchedulerBypassFlag); 
									break;
								case OUT_NODE: 
									abstractAccel->node = acapAddOutputNode(currentGraph,
										abstractAccel->ptr, abstractAccel->size,
										0, abstractAccel->semptr); //SchedulerBypassFlag); 
									//abstractAccel->node->accel.semptr = abstractAccel->semptr;
									break;
							
							}
							accelElement = accelElement->tail;
						}
						Element_t* buffElement = graph->buffNodeHead;
						while(buffElement != NULL){
							AbstractBuffNode_t *abstractBuff =
								(AbstractBuffNode_t *) buffElement->node;	
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
						acapGraphConfig(currentGraph);
						INFO("acapGraphConfig \n");
						acapGraphToJson(currentGraph);
						INFO("acapGraphToJson \n");
        					acapGraphSchedule(currentGraph);
						INFO("acapGraphSchedule \n");
						acapGraphFinalise(currentGraph);
						INFO("acapGraphFinalise \n");
						currentGraph = NULL;
						graph->state = AGRAPH_INIT;
						break;
					case AGRAPH_INIT:
						break;
					}
					graphElement = graphElement->tail;				
				}
				}
				else{
					INFO("praphElement : %p", graphElement);
				}
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
        //INFO("Submitting Graph with ID: %d\n", graph->id);
        JobQueueBuffer_t *commandBuff, *responseBuff;
        commandBuff = malloc(sizeof(JobQueueBuffer_t));
        commandBuff->type = GRAPH_SUBMIT;
        commandBuff->id = graph->id;
        queue_enqueue(scheduler->CommandQueue, commandBuff);
	while(1){
                responseBuff = (JobQueueBuffer_t *)queue_dequeue(scheduler->ResponseQueue);
                if(responseBuff->type == GRAPH_SUBMIT_DONE) break;
        }
        return 0;
}

int jobSchedulerRemove(JobScheduler_t *scheduler, Element_t *graphElement){
        AbstractGraph_t *graph = (AbstractGraph_t *)(graphElement->node);
	_unused(graph);
	_unused(scheduler);
        //INFO("Submitting Graph with ID: %d\n", graph->id);
        JobQueueBuffer_t *commandBuff, *responseBuff;
        commandBuff = malloc(sizeof(JobQueueBuffer_t));
        commandBuff->type = GRAPH_FINISH;
        commandBuff->id = graph->id;
        queue_enqueue(scheduler->CommandQueue, commandBuff);
	while(1){
                responseBuff = (JobQueueBuffer_t *)queue_dequeue(scheduler->ResponseQueue);
                if(responseBuff->type == GRAPH_FINISH_DONE) break;
        }
        return 0;
}
