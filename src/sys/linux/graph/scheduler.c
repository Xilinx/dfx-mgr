/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "layer0/utils.h"
#include "layer0/queue.h"
#include "graph.h"
#include "scheduler.h"
#include <dfx-mgr/print.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/daemon_helper.h>

int printTransaction(Schedule_t *schedule, char* message){
	if(schedule){
		_unused(socket_d);
		int dependent = schedule->dependency->linkCount;
		AccelNode_t *accelNode = schedule->dependency->link->accelNode;
		BuffNode_t *buffNode = schedule->dependency->link->buffNode;
		int interRMCompatible = buffNode->buffer.InterRMCompatible;
		BuffNode_t *dbuffNode = NULL;
		if(dependent){
			dbuffNode = schedule->dependency->dependentLinks[
				schedule->dependency->linkCount - 1]->buffNode;
		}
		INFOP("%d : %x: ", schedule->index, schedule->dependency->link->transactionIndex);
		INFOP("(%x) %s%d irm%d (%x, %x) ",
			schedule->status,
			accelNode->accel.name,
			accelNode->accel.index,
			accelNode->accel.InterRMCompatible,
			accelNode->S2MMStatus,
			accelNode->MM2SStatus);
		if(schedule->dependency->link->type){
			INFOP("====> ");
		}else{
			INFOP("<==== ");
		}
		INFOP("%s%d irm%d typ%d (%x %d)",
			buffNode->buffer.name,
			buffNode->buffer.index,
			buffNode->buffer.InterRMCompatible,
			buffNode->buffer.type,
			buffNode->status,
			schedule->dependency->link->channel);
		if(dependent){
			INFOP("depends on %s%d (%x) (%x)",
				dbuffNode->buffer.name,
				dbuffNode->buffer.index,
				dbuffNode->status,
				interRMCompatible);
		}
		INFOP(" %s\n", message);
	}
	return 0;
}

void *scheduler_Task(void* carg){
	//INFO("\n");
	AcapGraph_t *acapGraph = carg;
	Scheduler_t *scheduler = acapGraph->scheduler; 
        queue_t *commandQueue = scheduler->CommandQueue;
        queue_t *responseQueue = scheduler->ResponseQueue;
        ScQueueBuffer_t *commandQueueBuffer, *responseQueueBuffer;
        int busy = 0;
        int enableScheduler = 0;
        
        while(1){ 
		usleep(1000);
                if((busy == 0) && (queue_size(commandQueue) > 0)){
                        commandQueueBuffer = queue_dequeue(commandQueue);
                        switch (commandQueueBuffer->type){
                                case SCHEDULER_COMPLETION:
					INFOP("processing SCHEDULER_COMPLETION\n");
                                	responseQueueBuffer = malloc(sizeof(ScQueueBuffer_t));
                                	responseQueueBuffer->type = SCHEDULER_COMPLETION;
                                	queue_enqueue(responseQueue, responseQueueBuffer);
                                        break;
                                case SCHEDULER_TASKEND:
					INFOP("processing TASKEND\n");
                                        return NULL;
                                        break;
				case SCHEDULER_STATUS:
					INFOP("Status Requested\n");
                                	responseQueueBuffer = malloc(sizeof(ScQueueBuffer_t));
                                	responseQueueBuffer->type = SCHEDULER_STATUS;
                                	responseQueueBuffer->busy = busy;
                                	queue_enqueue(responseQueue, responseQueueBuffer);
					break;
                                case SCHEDULER_TRIGGER:
					INFOP("processing TRIGGER\n");
        				busy = 1;
        				enableScheduler = 1;
					//getRMInfo();
                                        break;
                                default:
                                        break;
                        }
                }
                if(enableScheduler){
			Schedule_t *schedule = acapGraph->scheduleHead;
			if(schedule != NULL){
            			while(1){
					usleep(100);
					//printf("######\n");
					if(schedule == NULL){
						break;
					}
					int dependent = schedule->dependency->linkCount;
					int readerCount = schedule->dependency->link->buffNode->buffer.readerCount;
					AccelNode_t *accelNode = schedule->dependency->link->accelNode;
					BuffNode_t *buffNode = schedule->dependency->link->buffNode;
					BuffNode_t *dbuffNode = NULL;
					int interRMCompatible = buffNode->buffer.InterRMCompatible;
					int transactionIndex = schedule->dependency->link->transactionIndex;
					if(dependent){
						dbuffNode = schedule->dependency->dependentLinks[
                                                	schedule->dependency->linkCount - 1]->buffNode;
						_unused(dbuffNode);
					}
					//printTransaction(schedule, "scheduler");
					if(accelNode->currentTransactionIndex == 0)
					{
						accelNode->currentTransactionIndex = transactionIndex; 
						//printf("######2\n");
					}
					//######################################################
                        		if(schedule->dependency->link->type){

						//printf("######3\n");
						if(schedule->status == 0 && 
							buffNode->status == 0 &&
							(((accelNode->accel.type == IO_NODE ||
							accelNode->accel.type == HW_NODE) &&
							accelNode->MM2SStatus == 0) ||
							(accelNode->accel.type == IN_NODE)) &&
							accelNode->S2MMStatus == 0 &&
							accelNode->currentTransactionIndex == transactionIndex 
							){
							//printf("%p\n", buffNode->buffer->ptr);
							accelNode->S2MMStatus = 1;
							buffNode->status = 1;
							schedule->status = 1;
							printTransaction(schedule, "S2MM Transaction Scheduled");
							break;
						}
						else if(schedule->status == 1 && 
							buffNode->status == 1 &&
							(((accelNode->accel.type == IO_NODE ||
							accelNode->accel.type == HW_NODE) &&
							accelNode->MM2SStatus == 1) ||
							(accelNode->accel.type == IN_NODE)) &&
							accelNode->S2MMStatus == 1){
							accelNode->S2MMStatus = 2;
							buffNode->status = 2;
							//printf("######5\n");
							//printf("%p\n", buffNode->buffer.ptr);
							accelNode->accel.datamover->S2MMData(
								accelNode->accel.datamover->dmstruct,
								&buffNode->buffer,
								schedule->offset,
								schedule->size,
								schedule->first
							); 
							printTransaction(schedule, "S2MM Transaction Triggered");
					
						}
						else if(schedule->status == 1 &&
							buffNode->status == 2 &&
							(((accelNode->accel.type == IO_NODE ||
							accelNode->accel.type == HW_NODE) &&
							accelNode->MM2SStatus == 2) ||
							(accelNode->accel.type == IN_NODE)) &&
							accelNode->S2MMStatus == 2){
							int status = accelNode->accel.datamover->S2MMDone(
								accelNode->accel.datamover->dmstruct,
                                                                &buffNode->buffer);
							if(status == 1){ 
								buffNode->status = 3;
								accelNode->S2MMStatus = 0;
								accelNode->currentTransactionIndex = 0; 
								printTransaction(schedule, "S2MM Transaction Done");
								delSchedule(&schedule, &(acapGraph->scheduleHead));
								continue;
							}
							else{
								printTransaction(schedule, "S2MM scheduler #");
								printf("S2MM status : %d\n", status);
							}
						}
                        		}else{
						//printf("######4\n");
						if(accelNode->currentTransactionIndex == 0)
						{
						accelNode->currentTransactionIndex = transactionIndex; 
						}
						//printf("###1\n");
						if(schedule->status == 0 && 
							(buffNode->status == 3 || 
							(
							//buffNode->buffer.type == 1 && 
							interRMCompatible == 2 && buffNode->status == 2)) &&
							(((accelNode->accel.type == IO_NODE ||
							accelNode->accel.type == HW_NODE) &&
							((dependent && accelNode->S2MMStatus == 1) || !dependent)) ||
							(accelNode->accel.type == OUT_NODE)) &&
							accelNode->MM2SStatus == 0 &&
							accelNode->currentTransactionIndex == transactionIndex){
							accelNode->MM2SStatus = 1;
							schedule->status = 1;
							printTransaction(schedule, "MM2S Transaction Scheduled");
							break;
						}
						else if(schedule->status == 1 && 
							(buffNode->status == 3 ||
							(
							//buffNode->buffer.type == 1 && 
							interRMCompatible == 2  && 
							buffNode->status == 2)) &&
							(((accelNode->accel.type == IO_NODE ||
							accelNode->accel.type == HW_NODE) &&
							((dependent && accelNode->S2MMStatus == 2) || !dependent)) ||
							(accelNode->accel.type == OUT_NODE)) &&
							accelNode->MM2SStatus == 1){
							accelNode->MM2SStatus = 2;
							accelNode->accel.datamover->MM2SData(
								accelNode->accel.datamover->dmstruct,
								&buffNode->buffer,
								schedule->offset,
								schedule->size,
								schedule->last,
								schedule->dependency->link->channel
							); 
							printTransaction(schedule, "MM2S Transaction Triggered");
						}
						else if(schedule->status == 1 && 
							(buffNode->status == 3) &&
							(((accelNode->accel.type == IO_NODE ||
							accelNode->accel.type == HW_NODE) &&
							((dependent && accelNode->S2MMStatus == 0) || !dependent)) ||
							(accelNode->accel.type == OUT_NODE)) &&
							accelNode->MM2SStatus == 2){
							int status = accelNode->accel.datamover->MM2SDone(
								accelNode->accel.datamover->dmstruct,
                                                                &buffNode->buffer);
							if(status == 1){ 
								buffNode->readStatus += 1;
								char msg[100];
								sprintf(msg, "MM2S Transaction Done %d %d", 
									buffNode->readStatus, readerCount);
								if(buffNode->readStatus == readerCount){
									buffNode->status = 0;
									buffNode->readStatus = 0;
								}
								accelNode->MM2SStatus = 0;
								accelNode->currentTransactionIndex = 0; 
								printTransaction(schedule, msg);
								delSchedule(&schedule, &(acapGraph->scheduleHead));
								continue;
							}
							else{
								printTransaction(schedule, "MM2S scheduler #");
								printf("MM2S status : %d\n", status);
							}
						}
                        		}
					
					//printf("######################################################\n");
                        		if(schedule->tail != NULL){
                                		schedule = schedule->tail;
                        		}
                        		else break;
                		}
        		}else{
        			busy = 0;
				printf("Scheduler Done !!\n");
			}
		}
        }
        return NULL;
}

int SchedulerInit(AcapGraph_t *acapGraph){
	//INFO("\n");
	Scheduler_t *scheduler = malloc(sizeof(Scheduler_t));
	acapGraph->scheduler = scheduler;

	scheduler->CommandQueue  = malloc(sizeof(queue_t));
        scheduler->ResponseQueue = malloc(sizeof(queue_t));
        *(scheduler->CommandQueue ) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(scheduler->ResponseQueue) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        
        pthread_create(scheduler->thread    , NULL, scheduler_Task, acapGraph);
        return 0;
}

int TaskEnd(Scheduler_t *scheduler){
        //INFO("\n");
	ScQueueBuffer_t *CommandBuff;
        CommandBuff = malloc(sizeof(ScQueueBuffer_t));
        CommandBuff->type = SCHEDULER_TASKEND;
        queue_enqueue(scheduler->CommandQueue, CommandBuff);
        return 0;
}

int SchedulerFinalise(AcapGraph_t *acapGraph){
        //INFO("\n");
	Scheduler_t *scheduler = acapGraph->scheduler;
	TaskEnd(scheduler);
        pthread_join(scheduler->thread[0], NULL);
        return 0;
}


int SchedulerTrigger(AcapGraph_t *acapGraph){
        //INFO("%s%d\n", buffer->name, buffer->index);
	ScQueueBuffer_t *CommandBuff;
        CommandBuff = malloc(sizeof(ScQueueBuffer_t));
        CommandBuff->type = SCHEDULER_TRIGGER;
        queue_enqueue(acapGraph->scheduler->CommandQueue, CommandBuff);
        return 0;
}
 
int SchedulerCompletion(AcapGraph_t *acapGraph){
        //INFO("\n");
        ScQueueBuffer_t *responseQueueBuffer, *commandBuff;
        commandBuff = malloc(sizeof(ScQueueBuffer_t));
        commandBuff->type = SCHEDULER_COMPLETION;
        queue_enqueue(acapGraph->scheduler->CommandQueue, commandBuff);
        while(1){
        	//INFO("%d\n",queue_size(datamover->S2MMResponseQueue))
               //if(queue_size(datamover->S2MMResponseQueue) > 0){
                responseQueueBuffer = queue_dequeue(acapGraph->scheduler->ResponseQueue);
                if(responseQueueBuffer->type == SCHEDULER_COMPLETION) break;
                //}
        }
	//getRMInfo();
        return 0;
}
        
int SchedulerStatus(AcapGraph_t *acapGraph){
        //INFO("\n");
        ScQueueBuffer_t *responseQueueBuffer, *commandBuff;
        commandBuff = malloc(sizeof(ScQueueBuffer_t));
        commandBuff->type = SCHEDULER_STATUS;
        queue_enqueue(acapGraph->scheduler->CommandQueue, commandBuff);
        while(1){
        	//INFO("%d\n",queue_size(datamover->S2MMResponseQueue))
               //if(queue_size(datamover->S2MMResponseQueue) > 0){
                responseQueueBuffer = queue_dequeue(acapGraph->scheduler->ResponseQueue);
                if(responseQueueBuffer->type == SCHEDULER_STATUS)
			return responseQueueBuffer->busy;
                //}
        }
        return 0;
}
        
