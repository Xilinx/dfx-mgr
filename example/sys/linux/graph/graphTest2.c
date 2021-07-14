/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <dfx-mgr/sys/linux/graph/layer0/utils.h>
#include <unistd.h>
#include <dfx-mgr/sys/linux/graph/graph.h>
#include <dfx-mgr/sys/linux/graph/abstractGraph.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/assert.h>
#include <fcntl.h>

int main(void){
	int status;
	INFO("TEST1: CMA buffer with single PL Accelerator\n");

	AbstractGraph_t *acapGraph = graphInit();

	AbstractAccelNode_t *accelNode0 = addInputNode(acapGraph, 32*1024*1024);
	AbstractAccelNode_t *accelNode1 = addAcceleratorNode(acapGraph, "FFT4"); 
	AbstractAccelNode_t *accelNode2 = addOutputNode(acapGraph, 32*1024*1024);

	AbstractBuffNode_t *buffNode0 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
	AbstractBuffNode_t *buffNode1 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);

	addOutBuffer(acapGraph, accelNode0, buffNode0, 0x00, 32*1024*1024, 1, 0);
	addInBuffer (acapGraph, accelNode1, buffNode0, 0x00, 32*1024*1024, 2, 0);
	addOutBuffer(acapGraph, accelNode1, buffNode1, 0x00, 32*1024*1024, 2, 0);
	addInBuffer (acapGraph, accelNode2, buffNode1, 0x00, 32*1024*1024, 3, 0);

	status = abstractGraphConfig(acapGraph);
	if(status < 0){
		printf("Seems like GraphDaemon not running ...!!\n");
		return -1;
	}
	for(int i=0; i < 1024; i++){
		accelNode0->ptr[i] = i;
	}
	sem_post(accelNode0->semptr);
	sem_wait(accelNode2->semptr);

	printhex((uint32_t*)accelNode0->ptr, 0x50);
	printhex((uint32_t*)accelNode2->ptr, 0x50);

	abstractGraphFinalise(acapGraph);
	return 0;
}

