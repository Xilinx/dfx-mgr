/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "dm.h"
#include "xrtbuffer.h"
#include "slotManager.h"
#include "utils.h"
#include "debug.h"

void *dm_Task(void* carg){
        dm_t *datamover = carg;
        queue_t *commandQueue = datamover->CommandQueue;
        queue_t *responseQueue = datamover->ResponseQueue;
        queue_t *cacheQueue = datamover->CacheQueue;
        QueueBuffer_t *commandQueueBuffer, *responseQueueBuffer;
	int res;
	datamover->S2MMChannelStatus = 0;
	datamover->MM2SChannelStatus = 0;
        while(1){ //for(int i=0; i < 1000; i++){
                usleep(1000);
                sleep(1);
                if(queue_size(commandQueue) > 0 || queue_size(cacheQueue) > 0){
		INFO("%d %d\n", queue_size(commandQueue), queue_size(cacheQueue));
			
			if(queue_size(commandQueue) > 0){
                        	commandQueueBuffer = queue_dequeue(commandQueue);
			}
			else{
				INFO("from Cache Queue !!\n");
                        	commandQueueBuffer = queue_dequeue(cacheQueue);
			}
			INFO("%x\n", commandQueueBuffer->type);
                        //printf("%d\n", commandQueueBuffer->type);
                        switch (commandQueueBuffer->type){
                            case TRANSACTIONEND:
                                INFO("processing TRANSACTIONEND\n");
                                if(queue_size(commandQueue) <= 1 || queue_size(cacheQueue) == 0){
                                	responseQueueBuffer = malloc(sizeof(QueueBuffer_t));
                               		responseQueueBuffer->type = TRANSACTIONEND;
                                	queue_enqueue(responseQueue, responseQueueBuffer);
				}
                                break;
                            case TASKEND:
                                INFO("processing TASKEND\n");
                                return NULL;
                                break;
                            case CMATRANSACTION:
                                INFO("processing CMATRANSACTION %s%d %d\n", commandQueueBuffer->buffer->name,
                                commandQueueBuffer->buffer->index, commandQueueBuffer->transferSize);
				if(commandQueueBuffer->status == 0){
					res = -1;
					if(commandQueueBuffer->direction == TRANS_MM2S && 
						datamover->MM2SChannelStatus == 0){
						INFO("Triggering MM2S\n");
                                		res = datamover->MM2SData(datamover->dmstruct, commandQueueBuffer->buffer,
                                			commandQueueBuffer->dependentBuffer, 
							commandQueueBuffer->transferSize, 0x00);
						if(res == 0){
							datamover->MM2SChannelStatus = 1;
					        	commandQueueBuffer->status = 1;
						}
					
					}else if(commandQueueBuffer->direction == TRANS_S2MM && 
						datamover->S2MMChannelStatus == 0){
						INFO("Triggering S2MM\n");
                                		res = datamover->S2MMData(datamover->dmstruct, commandQueueBuffer->buffer,
                                			commandQueueBuffer->transferSize);
						if(res == 0){
							datamover->S2MMChannelStatus = 1;
					        	commandQueueBuffer->status = 1;
						}
					}
					if(res == -1){INFO("Transaction rescheduled !!\n");}
					INFO("%d %d\n", queue_size(commandQueue), queue_size(cacheQueue));
				        queue_enqueue(cacheQueue, commandQueueBuffer);
					INFO("%d %d\n", queue_size(commandQueue), queue_size(cacheQueue));
				}
				else if(commandQueueBuffer->status == 1){
					if(commandQueueBuffer->direction == TRANS_MM2S && 
						datamover->MM2SChannelStatus == 1){
						INFO("Status MM2S \n");

                        			if(datamover->MM2SDone(datamover->dmstruct, commandQueueBuffer->buffer)) {
                                			INFO("MM2S Done %s%d\n", commandQueueBuffer->buffer->name,
                                                		commandQueueBuffer->buffer->index);
							datamover->MM2SChannelStatus = 0;
                                			if(commandQueueBuffer->buffer->ptr != NULL){
                                        			printhex(commandQueueBuffer->buffer->ptr, 0x20);
                                			}
                                			responseQueueBuffer = malloc(sizeof(QueueBuffer_t));
                                			responseQueueBuffer->type = DONE;
                                			queue_enqueue(responseQueue, responseQueueBuffer);
						}else {
							INFO("Transaction rescheduled !!\n");
				        		queue_enqueue(cacheQueue, commandQueueBuffer);
						}
					}else if(commandQueueBuffer->direction == TRANS_S2MM && 
						datamover->S2MMChannelStatus == 1){
						INFO("Status S2MM \n");

                        			if(datamover->S2MMDone(datamover->dmstruct, commandQueueBuffer->buffer)) {
                                			INFO("S2MM Done %s%d\n", commandQueueBuffer->buffer->name,
                                                		commandQueueBuffer->buffer->index);
							datamover->S2MMChannelStatus = 0;
                                			if(commandQueueBuffer->buffer->ptr != NULL){
                                        			printhex(commandQueueBuffer->buffer->ptr, 0x20);
                                			}
                                			responseQueueBuffer = malloc(sizeof(QueueBuffer_t));
                                			responseQueueBuffer->type = DONE;
                                			queue_enqueue(responseQueue, responseQueueBuffer);
						}else {
							INFO("Transaction rescheduled !!\n");
				        		queue_enqueue(cacheQueue, commandQueueBuffer);
						}
					}else {
						INFO("Error Occured \n");
					}
				
				}
				INFO("%d %d\n", queue_size(commandQueue), queue_size(cacheQueue));
                                break;
                            default:
                                break;
			}
		}
	}
        return NULL;
}

void *MM2S_Task(void* carg){
	INFO("\n");
	dm_t *datamover = carg; 
        queue_t *commandQueue = datamover->MM2SCommandQueue;
        queue_t *responseQueue = datamover->MM2SResponseQueue;
        QueueBuffer_t *commandQueueBuffer, *responseQueueBuffer;
        int busy = 0;
        
        while(1){ //for(int i=0; i < 1000; i++){
		usleep(1000);
                if((busy == 0) && (queue_size(commandQueue) > 0)){
                        commandQueueBuffer = queue_dequeue(commandQueue);
                        //printf("%d\n", commandQueueBuffer->type);
                        switch (commandQueueBuffer->type){
        			case TRANSACTIONEND:
					//INFO("processing TRANSACTIONEND\n");
                                	responseQueueBuffer = malloc(sizeof(QueueBuffer_t));
                                	responseQueueBuffer->type = TRANSACTIONEND;
                               	 	queue_enqueue(responseQueue, responseQueueBuffer);
                                        break;
                                case TASKEND:
					INFO("processing TASKEND\n");
                                        return NULL;
                                        break;
                                case CMATRANSACTION:
					INFO("processing CMATRANSACTION %s%d %d\n", commandQueueBuffer->buffer->name, 
						commandQueueBuffer->buffer->index, commandQueueBuffer->transferSize);
                                        datamover->MM2SData(datamover->dmstruct, commandQueueBuffer->buffer,
					commandQueueBuffer->dependentBuffer, commandQueueBuffer->transferSize, 0x00);
                			//INFO("==============> : %d\n", commandQueueBuffer->buffer->readStatus);
					busy = 1;
                                        break;
                                default:
                                        break;
                        }
                }
                if (busy){
                        //datamover->MM2SStatus(datamover->dmstruct); 
                        if(datamover->MM2SDone(datamover->dmstruct, commandQueueBuffer->buffer)) {
                                INFO("MM2S Done %s%d\n", commandQueueBuffer->buffer->name, 
						commandQueueBuffer->buffer->index);
				if(commandQueueBuffer->buffer->readStatus == 2 * commandQueueBuffer->buffer->readerCount){
					 commandQueueBuffer->buffer->readStatus = 0;
					 commandQueueBuffer->buffer->writeStatus = 0;
				}
                                busy = 0;
				if(commandQueueBuffer->buffer->ptr != NULL){
					printhex(commandQueueBuffer->buffer->ptr, 0x20);
				}
                		INFO("==============> : %d\n", commandQueueBuffer->buffer->readStatus);
                                responseQueueBuffer = malloc(sizeof(QueueBuffer_t));
                                responseQueueBuffer->type = DONE;
                                queue_enqueue(responseQueue, responseQueueBuffer);
                        }
                }
        }
        return NULL;
}
void *S2MM_Task(void* carg){
	INFO("\n");
	dm_t *datamover = carg; 
        queue_t *commandQueue = datamover->S2MMCommandQueue;
	queue_t *responseQueue = datamover->S2MMResponseQueue;
        QueueBuffer_t *commandQueueBuffer, *responseQueueBuffer;
        int busy = 0;

        while(1){
		usleep(1000);
                if((busy ==0) && (queue_size(commandQueue) > 0)){
                        commandQueueBuffer = queue_dequeue(commandQueue);
                        switch (commandQueueBuffer->type){
        			case TRANSACTIONEND:
					//INFO("processing TRANSACTIONEND\n");
                                	responseQueueBuffer = malloc(sizeof(QueueBuffer_t));
                                	responseQueueBuffer->type = TRANSACTIONEND;
                               	 	queue_enqueue(responseQueue, responseQueueBuffer);
                                        break;
        			case TASKEND:
					INFO("processing TASKEND\n");
                                        return NULL;
                                        break;
                                case CMATRANSACTION:
					INFO("processing CMATRANSACTION %s%d %d\n", commandQueueBuffer->buffer->name, 
						commandQueueBuffer->buffer->index, commandQueueBuffer->transferSize);
                                        datamover->S2MMData(datamover->dmstruct, commandQueueBuffer->buffer, 
						commandQueueBuffer->transferSize);
                			//INFO("==============> : %d\n", commandQueueBuffer->buffer->writeStatus);
					busy = 1;
                                        break;
                                default:
    					break;
                        }
                }
                if (busy){
                        if(datamover->S2MMDone(datamover->dmstruct, commandQueueBuffer->buffer)) {
                                INFO("S2MM Done %s%d\n", commandQueueBuffer->buffer->name, 
						commandQueueBuffer->buffer->index);
				if(commandQueueBuffer->buffer->ptr != NULL){
					printhex(commandQueueBuffer->buffer->ptr, 0x20);
				}
                		busy = 0;
                		INFO("==============> : %d\n", commandQueueBuffer->buffer->writeStatus);
                                responseQueueBuffer = malloc(sizeof(QueueBuffer_t));
                                responseQueueBuffer->type = DONE;
                                queue_enqueue(responseQueue, responseQueueBuffer);
                        }
                }
        }
        return 0;
}

int TaskInit(dm_t* datamover){
	INFO("\n");
	datamover->CommandQueue  = malloc(sizeof(queue_t));
        datamover->ResponseQueue = malloc(sizeof(queue_t));
        datamover->CacheQueue = malloc(sizeof(queue_t));

	datamover->MM2SCommandQueue  = malloc(sizeof(queue_t));
        datamover->MM2SResponseQueue = malloc(sizeof(queue_t));
        datamover->S2MMCommandQueue  = malloc(sizeof(queue_t));
        datamover->S2MMResponseQueue = malloc(sizeof(queue_t));
        //INFO("%p\n", datamover->S2MMCommandQueue);
        *(datamover->CommandQueue ) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(datamover->ResponseQueue ) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(datamover->CacheQueue ) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(datamover->MM2SCommandQueue ) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(datamover->MM2SResponseQueue) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(datamover->S2MMCommandQueue ) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(datamover->S2MMResponseQueue) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        
        //INFO("%p\n", datamover->S2MMCommandQueue);
        
        pthread_create(datamover->thread    , NULL, dm_Task, datamover);
        //pthread_create(datamover->thread    , NULL, S2MM_Task, datamover);
       // pthread_create(datamover->thread + 1, NULL, MM2S_Task, datamover);
        //printf("Thread Started\n");
        return 0;
}

int TaskFinalise(dm_t* datamover){
        INFO("\n");
	TaskEnd(datamover);
        pthread_join(datamover->thread[0], NULL);
        pthread_join(datamover->thread[1], NULL);
        //printf("Thread Finalised\n");
        return 0;
}

int TaskEnd(dm_t* datamover){
        INFO("\n");
	QueueBuffer_t *MM2SCommandBuff, *S2MMCommandBuff;
        MM2SCommandBuff = malloc(sizeof(QueueBuffer_t));
        MM2SCommandBuff->type = TASKEND;
        queue_enqueue(datamover->MM2SCommandQueue, MM2SCommandBuff);
        S2MMCommandBuff = malloc(sizeof(QueueBuffer_t));
        S2MMCommandBuff->type = TASKEND;
        queue_enqueue(datamover->S2MMCommandQueue, S2MMCommandBuff);
        return 0;
}

int transact(dm_t *datamover, Buffer_t* buffer, Buffer_t* dependentBuffer, int insize, int direction){
        //INFO("%s%d %x\n", buffer->name, buffer->index, CMATRANSACTION);
        QueueBuffer_t *commandBuff;
        commandBuff = malloc(sizeof(QueueBuffer_t));
        commandBuff->type = CMATRANSACTION;
        commandBuff->transferSize = insize;
        commandBuff->buffer = buffer;
        commandBuff->dependentBuffer = dependentBuffer;
        commandBuff->direction = direction;
        commandBuff->status = 0;
        buffer->status = INQUEUE;
        queue_enqueue(datamover->CommandQueue, commandBuff);
        return 0;
}

int MM2S_Buffer(dm_t *datamover, Buffer_t* buffer, Buffer_t* dependentBuffer, int insize){
        //INFO("%s%d\n", buffer->name, buffer->index);
	QueueBuffer_t *MM2SCommandBuff;
        MM2SCommandBuff = malloc(sizeof(*MM2SCommandBuff));
        MM2SCommandBuff->type = CMATRANSACTION;
        MM2SCommandBuff->transferSize = insize;
        MM2SCommandBuff->buffer = buffer;
	MM2SCommandBuff->dependentBuffer = dependentBuffer;
	buffer->status = INQUEUE;
        queue_enqueue(datamover->MM2SCommandQueue, MM2SCommandBuff);
        return 0;
}
        
int S2MM_Buffer(dm_t *datamover, Buffer_t* buffer, int outsize){
        //INFO("%s%d\n", buffer->name, buffer->index);
	QueueBuffer_t *S2MMCommandBuff;
        S2MMCommandBuff = malloc(sizeof(*S2MMCommandBuff));
        S2MMCommandBuff->type = CMATRANSACTION;
        S2MMCommandBuff->transferSize = outsize;
        S2MMCommandBuff->buffer = buffer;
	buffer->status = INQUEUE;
        queue_enqueue(datamover->S2MMCommandQueue, S2MMCommandBuff);
        return 0;
}
       
int S2MM_Completion(dm_t *datamover){
        //INFO("\n");
        QueueBuffer_t *responseQueueBuffer, *commandBuff;
        commandBuff = malloc(sizeof(QueueBuffer_t));
        commandBuff->type = TRANSACTIONEND;
        queue_enqueue(datamover->CommandQueue, commandBuff);

	while(1){
		//INFO("%d\n",queue_size(datamover->S2MMResponseQueue))
		//if(queue_size(datamover->S2MMResponseQueue) > 0){
			responseQueueBuffer = queue_dequeue(datamover->ResponseQueue);
        		if(responseQueueBuffer->type == TRANSACTIONEND) break;
		//}
        }
        return 0;
}
       
int MM2S_Completion(dm_t *datamover){
        //INFO("\n");
        QueueBuffer_t *responseQueueBuffer, *commandBuff;
        commandBuff = malloc(sizeof(QueueBuffer_t));
        commandBuff->type = TRANSACTIONEND;
        queue_enqueue(datamover->CommandQueue, commandBuff);
	while(1){
		//INFO("%d\n",queue_size(datamover->MM2SResponseQueue))
		//if(queue_size(datamover->MM2SResponseQueue) > 0){
			responseQueueBuffer = queue_dequeue(datamover->ResponseQueue);
        		if(responseQueueBuffer->type == TRANSACTIONEND) break;
		//}
        }
        return 0;
}
