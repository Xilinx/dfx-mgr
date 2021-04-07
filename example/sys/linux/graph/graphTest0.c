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
	INFO("TEST0: Just load CMA buffer without PL Accelerator");
	AbstractGraph_t *acapGraph = graphInit();
        AbstractAccelNode_t *accelNode0 = addInputNode(acapGraph, 32*1024*1024);
        AbstractAccelNode_t *accelNode1 = addOutputNode(acapGraph, 32*1024*1024);

	_unused(accelNode0);
	_unused(accelNode1);
        abstractGraphConfig(acapGraph);
	for(int i=0; i < 1024; i++){
		accelNode0->ptr[i] = i;
	}
								
	abstractGraphFinalise(acapGraph);
	return 0;
}
