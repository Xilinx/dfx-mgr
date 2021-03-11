/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "jobScheduler.h"
#include "abstractGraph.h"
#include "layer0/queue.h"
#include "layer0/debug.h"


void *jobScheduler_Task(void* carg){
        INFO("\n");
	JobScheduler_t *scheduler = (JobScheduler_t *)carg; 
        Element_t *graphList = scheduler->graphList;
        queue_t *commandQueue = scheduler->CommandQueue;
        queue_t *responseQueue = scheduler->ResponseQueue;
        JobQueueBuffer_t *commandQueueBuffer, *responseQueueBuffer;
        int busy = 0;
        int enableScheduler = 0;
	_unused(busy);
	_unused(enableScheduler);
	_unused(commandQueue);
	_unused(responseQueue);
	_unused(graphList);
	_unused(commandQueueBuffer);
	_unused(responseQueueBuffer);
        while(1){
                usleep(1000);
	}
        return NULL;
}

JobScheduler_t * jobSchedulerInit(){
        INFO("\n");
        JobScheduler_t *scheduler = malloc(sizeof(JobScheduler_t));

        scheduler->CommandQueue  = malloc(sizeof(queue_t));
        scheduler->ResponseQueue = malloc(sizeof(queue_t));
        *(scheduler->CommandQueue ) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };
        *(scheduler->ResponseQueue) = (queue_t) { malloc(sizeof(void*)*1000), 1000, 0, 0, 0,
                                      PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER };

        pthread_create(scheduler->thread    , NULL, jobScheduler_Task, scheduler);
        return 0;
}


