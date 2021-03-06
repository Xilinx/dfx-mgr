/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <pthread.h>
#include <stdio.h>

#define QUEUE_INITIALIZER(buffer) { buffer, sizeof(buffer) / sizeof(buffer[0]), 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }


typedef struct queue queue_t; 
struct queue
{
	void **buffer;
	int capacity;
	int size;
	int in;
	int out;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
};

typedef struct biQueue
{
	queue_t *commandQueue;
	queue_t *responseQueue;
} biQueue_t;

extern void queue_enqueue(queue_t *queue, void *value);
extern void *queue_dequeue(queue_t *queue);
extern int queue_size(queue_t *queue);
