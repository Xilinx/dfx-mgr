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
#include <dfx-mgr/sys/linux/graph/layer0/debug.h>
//#include <dfx-mgr/sys/linux/graph/layer0/uio.h>
#include <unistd.h>
//#include "metadata.h"
#include <dfx-mgr/sys/linux/graph/graph.h>
#include <dfx-mgr/sys/linux/graph/abstractGraph.h>

uint32_t buff[] = {
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
	0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7
};
uint32_t keybuff[] = {
	0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};

int softFFT(void* inData, int inDataSize, void* inConfig, int inConfigSize, void* outData, int outDataSize){
	INFO("FALLBACK CALLED !!\n");
	_unused(inConfig);
	_unused(inConfigSize);
	_unused(outDataSize);
	memcpy(outData, inData, inDataSize);
	return 0;
}

int softFIR(void* inData, int inDataSize, void* inConfig, int inConfigSize, void* outData, int outDataSize){
	INFO("FALLBACK CALLED !!\n");
	_unused(inConfig);
	_unused(inConfigSize);
	_unused(outDataSize);
	memcpy(outData, inData, inDataSize);
	return 0;
}

int softAES128(void* inData, int inDataSize, void* inConfig, int inConfigSize, void* outData, int outDataSize){
	INFO("FALLBACK CALLED !!\n");
	_unused(inConfig);
	_unused(inConfigSize);
	_unused(outDataSize);
	memcpy(outData, inData, inDataSize);
	return 0;
}
#define BUFFSIZE 0x400000
/*
int case0(){
	//TEST0: CMA buffer without PL Accelerator
	uint8_t *p0 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(32*1024*1024);
	AcapGraph_t *acapGraph = acapGraphInit();
        AccelNode_t *accelNode0 = acapAddInputNode(acapGraph, p0, 32*1024*1024, ENABLE_SCHEDULER);
        AccelNode_t *accelNode1 = acapAddOutputNode(acapGraph, p1, 32*1024*1024, ENABLE_SCHEDULER);

	BuffNode_t *buffNode0 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
	
	acapAddOutputBuffer(acapGraph, accelNode0, buffNode0, 0x00, 32*1024*1024, 1, 0);
	
	acapAddInputBuffer (acapGraph, accelNode1, buffNode0, 0x00, 32*1024*1024, 5, 0);
	
	acapGraphToJson(acapGraph);
        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
	
	for(int i=0; i < 1024; i++){
		p0[i] = i;
	}
	printhex((uint32_t*)p0, 0x50);
        acapGraphSchedule(acapGraph);
	printhex((uint32_t*)p1, 0x50);
        acapGraphFinalise(acapGraph);
	return 0;
}

int case1(){
	INFO("TEST1: CMA buffer with single PL Accelerator\n");
	uint8_t *p0 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(32*1024*1024);

	AcapGraph_t *acapGraph = acapGraphInit();
        AccelNode_t *accelNode0 = acapAddInputNode(acapGraph, p0, 32*1024*1024, ENABLE_SCHEDULER);
        AccelNode_t *accelNode1 = acapAddAccelNode(acapGraph, "FFT4", HLS_MULTICHANNEL_DMA, softFFT,
							INTER_RM_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode4 = acapAddOutputNode(acapGraph, p1, 32*1024*1024, ENABLE_SCHEDULER);

	acapGraphToJson(acapGraph);
	BuffNode_t *buffNode0 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
        BuffNode_t *buffNode1 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
	
	acapAddOutputBuffer(acapGraph, accelNode0, buffNode0, 0x00, 32*1024*1024, 1, 0);
	
	acapAddInputBuffer (acapGraph, accelNode1, buffNode0, 0x00, 32*1024*1024, 2, 0);
        acapAddOutputBuffer(acapGraph, accelNode1, buffNode1, 0x00, 32*1024*1024, 2, 0);
        
	acapAddInputBuffer (acapGraph, accelNode4, buffNode1, 0x00, 32*1024*1024, 5, 0);
	acapGraphToJson(acapGraph);
        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
	
	for(int i=0; i < 1024; i++){
		p0[i] = i;
	}
	printhex((uint32_t*)p0, 0x50);
        acapGraphSchedule(acapGraph);
	printhex((uint32_t*)p1, 0x50);
        acapGraphFinalise(acapGraph);
	return 0;
}

int case2(){
	INFO("TEST2: \n");
	uint8_t *p0 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p2 = (uint8_t*) malloc(32*1024*1024);

	AcapGraph_t *acapGraph = acapGraphInit();
        AccelNode_t *accelNode0 = acapAddInputNode(acapGraph, p0, 32*1024*1024, ENABLE_SCHEDULER);
        AccelNode_t *accelNode1 = acapAddAccelNode(acapGraph, "fir_compiler", HLS_MULTICHANNEL_DMA, NULL,
							INTER_RM_NOT_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode2 = acapAddAccelNode(acapGraph, "FFT4", HLS_MULTICHANNEL_DMA, softFFT,
							INTER_RM_NOT_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode3 = acapAddAccelNode(acapGraph, "aes128encdec", HLS_MULTICHANNEL_DMA, softFFT,
							INTER_RM_NOT_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode4 = acapAddOutputNode(acapGraph, p1, 32*1024*1024, ENABLE_SCHEDULER);
        AccelNode_t *accelNode5 = acapAddOutputNode(acapGraph, p2, 32*1024*1024, ENABLE_SCHEDULER);
	
	BuffNode_t *buffNode0 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
        BuffNode_t *buffNode1 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
        BuffNode_t *buffNode2 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
        BuffNode_t *buffNode3 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
	
	acapAddOutputBuffer(acapGraph, accelNode0, buffNode0, 0x00, 32*1024*1024, 1, 0);
	
	acapAddInputBuffer (acapGraph, accelNode1, buffNode0, 0x00, 32*1024*1024, 2, 0);
        acapAddOutputBuffer(acapGraph, accelNode1, buffNode1, 0x00, 32*1024*1024, 2, 0);
        
	acapAddInputBuffer (acapGraph, accelNode2, buffNode0, 0x00, 32*1024*1024, 3, 0);
        acapAddOutputBuffer(acapGraph, accelNode2, buffNode2, 0x00, 32*1024*1024, 3, 0);

        acapAddInputBuffer (acapGraph, accelNode3, buffNode1, 0x00, 32*1024*1024, 4, 0);
        acapAddOutputBuffer(acapGraph, accelNode3, buffNode3, 0x00, 32*1024*1024, 4, 0);
	
	acapAddInputBuffer (acapGraph, accelNode4, buffNode2, 0x00, 32*1024*1024, 5, 0);

	acapAddInputBuffer (acapGraph, accelNode5, buffNode3, 0x00, 32*1024*1024, 6, 0);
	
	acapGraphToJson(acapGraph);
        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
	
	for(int i=0; i < 1024; i++){
		p0[i] = i;
	}
	printhex((uint32_t*)p0, 0x50);
        acapGraphSchedule(acapGraph);
	printhex((uint32_t*)p1, 0x50);
	printhex((uint32_t*)p2, 0x50);
        acapGraphFinalise(acapGraph);
	return 0;
}

int case3(){
	INFO("TEST1: CMA buffer with single PL Accelerator\n");
	uint8_t *p0 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(32*1024*1024);

	AcapGraph_t *acapGraph = acapGraphInit();
        AccelNode_t *accelNode0 = acapAddInputNode(acapGraph, p0, 32*1024*1024, ENABLE_SCHEDULER);
        AccelNode_t *accelNode1 = acapAddAccelNode(acapGraph, "fir_compiler", HLS_MULTICHANNEL_DMA, NULL,
							INTER_RM_NOT_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode2 = acapAddAccelNode(acapGraph, "FFT4", HLS_MULTICHANNEL_DMA, softFFT,
							INTER_RM_NOT_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode3 = acapAddAccelNode(acapGraph, "aes128encdec", HLS_MULTICHANNEL_DMA, softFFT,
							INTER_RM_NOT_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode5 = acapAddOutputNode(acapGraph, p1, 32*1024*1024, ENABLE_SCHEDULER);

	BuffNode_t *buffNode0 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
        BuffNode_t *buffNode1 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
        BuffNode_t *buffNode2 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
        BuffNode_t *buffNode3 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
	
	acapAddOutputBuffer(acapGraph, accelNode0, buffNode0, 0x00, 32*1024*1024, 1, 0);
	
	acapAddInputBuffer (acapGraph, accelNode1, buffNode0, 0x00, 32*1024*1024, 2, 0);
        acapAddOutputBuffer(acapGraph, accelNode1, buffNode1, 0x00, 32*1024*1024, 2, 0);
        
	acapAddInputBuffer (acapGraph, accelNode2, buffNode1, 0x00, 32*1024*1024, 3, 0);
        acapAddOutputBuffer(acapGraph, accelNode2, buffNode2, 0x00, 32*1024*1024, 3, 0);

        acapAddInputBuffer (acapGraph, accelNode3, buffNode2, 0x00, 32*1024*1024, 4, 0);
        acapAddOutputBuffer(acapGraph, accelNode3, buffNode3, 0x00, 32*1024*1024, 4, 0);
	
	acapAddInputBuffer (acapGraph, accelNode5, buffNode3, 0x00, 32*1024*1024, 6, 0);
	
	acapGraphToJson(acapGraph);
        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
	
	for(int i=0; i < 1024; i++){
		p0[i] = i;
	}
	printhex((uint32_t*)p0, 0x50);
        acapGraphSchedule(acapGraph);
	printhex((uint32_t*)p1, 0x50);
        acapGraphFinalise(acapGraph);
	return 0;
}

int case4(){
	INFO("TEST1: CMA buffer with single PL Accelerator\n");
	uint8_t *p0 = (uint8_t*) malloc(64*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(64*1024*1024);

	AcapGraph_t *acapGraph = acapGraphInit();
        AccelNode_t *accelNode0 = acapAddInputNode(acapGraph, p0, 64*1024*1024, ENABLE_SCHEDULER);
        AccelNode_t *accelNode1 = acapAddAccelNode(acapGraph, "fir_compiler", HLS_MULTICHANNEL_DMA, softFIR, 
							INTER_RM_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode2 = acapAddAccelNode(acapGraph, "FFT4", HLS_MULTICHANNEL_DMA, softFFT, 
							INTER_RM_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode3 = acapAddAccelNode(acapGraph, "aes128encdec", HLS_MULTICHANNEL_DMA, softFFT,
							INTER_RM_COMPATIBLE, ENABLE_SCHEDULER);
        AccelNode_t *accelNode4 = acapAddOutputNode(acapGraph, p1, 64*1024*1024, ENABLE_SCHEDULER);

	BuffNode_t *buffNode0 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
        BuffNode_t *buffNode1 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", PL_BASED);
        BuffNode_t *buffNode2 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", PL_BASED);
        BuffNode_t *buffNode3 = acapAddBuffNode(acapGraph, 16*1024*1024, "Buffer", DDR_BASED);
	
	acapAddOutputBuffer(acapGraph, accelNode0, buffNode0, 0x00, 64*1024*1024, 1, 0);
	
	acapAddInputBuffer (acapGraph, accelNode1, buffNode0, 0x00, 64*1024*1024, 2, 0);
        acapAddOutputBuffer(acapGraph, accelNode1, buffNode1, 0x00, 64*1024*1024, 2, 0);
        
	acapAddInputBuffer (acapGraph, accelNode2, buffNode1, 0x00, 64*1024*1024, 3, 0);
        acapAddOutputBuffer(acapGraph, accelNode2, buffNode2, 0x00, 64*1024*1024, 3, 0);

        acapAddInputBuffer (acapGraph, accelNode3, buffNode2, 0x00, 64*1024*1024, 4, 0);
        acapAddOutputBuffer(acapGraph, accelNode3, buffNode3, 0x00, 64*1024*1024, 4, 0);
	
	acapAddInputBuffer (acapGraph, accelNode4, buffNode3, 0x00, 64*1024*1024, 6, 0);
	
        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
	
	for(int i=0; i < 1024; i++){
		p0[i] = i;
	}
	printhex((uint32_t*)p0, 0x50);
        acapGraphSchedule(acapGraph);
	printhex((uint32_t*)p1, 0x50);
        acapGraphFinalise(acapGraph);
	return 0;
}

int case5(){
	INFO("TEST2: \n");
	uint8_t *p0 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p2 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *c0 = (uint8_t*) malloc(4*1024);
	uint8_t *c1 = (uint8_t*) malloc(4*1024);
	uint8_t *c2 = (uint8_t*) malloc(4*1024);

	AcapGraph_t *acapGraph = graphInit();
        AccelNode_t *config0 = addInputNode(acapGraph, c0, 4*1024, ENABLE_SCHEDULER);
        AccelNode_t *config1 = addInputNode(acapGraph, c1, 4*1024, ENABLE_SCHEDULER);
        AccelNode_t *config2 = addInputNode(acapGraph, c2, 4*1024, ENABLE_SCHEDULER);
        
	AccelNode_t *accelNode0 = addInputNode(acapGraph, p0, 32*1024*1024, ENABLE_SCHEDULER);
        
	AccelNode_t *accelNode1 = addAcceleratorNode(acapGraph, "fir_compiler", ENABLE_SCHEDULER);
        AccelNode_t *accelNode2 = addAcceleratorNode(acapGraph, "FFT4", ENABLE_SCHEDULER);
        AccelNode_t *accelNode3 = addAcceleratorNode(acapGraph, "aes128encdec", ENABLE_SCHEDULER);
        
	AccelNode_t *accelNode4 = addOutputNode(acapGraph, p1, 32*1024*1024, ENABLE_SCHEDULER);
        AccelNode_t *accelNode5 = addOutputNode(acapGraph, p2, 32*1024*1024, ENABLE_SCHEDULER);
	
	BuffNode_t *buffNode0 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        BuffNode_t *buffNode1 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        BuffNode_t *buffNode2 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        BuffNode_t *buffNode3 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        BuffNode_t *buffConf0 = addBuffer(acapGraph, 4*1024, DDR_BASED);
        BuffNode_t *buffConf1 = addBuffer(acapGraph, 4*1024, DDR_BASED);
        BuffNode_t *buffConf2 = addBuffer(acapGraph, 4*1024, DDR_BASED);
	
	addOutBuffer(acapGraph, config0, buffConf0, 0x00, 32, 0, 0);
	addOutBuffer(acapGraph, config1, buffConf1, 0x00, 32, 1, 0);
	addOutBuffer(acapGraph, config2, buffConf2, 0x00, 32, 2, 0);
	addOutBuffer(acapGraph, accelNode0, buffNode0, 0x00, 32*1024*1024, 3, 0);
	
	addInBuffer (acapGraph, accelNode1, buffConf0, 0x00, 32, 4, 1);
	addInBuffer (acapGraph, accelNode2, buffConf1, 0x00, 32, 5, 1);
	addInBuffer (acapGraph, accelNode3, buffConf2, 0x00, 32, 6, 1);

	addInBuffer (acapGraph, accelNode1, buffNode0, 0x00, 32*1024*1024, 7, 0);
        addOutBuffer(acapGraph, accelNode1, buffNode1, 0x00, 32*1024*1024, 7, 0);
        
	addInBuffer (acapGraph, accelNode2, buffNode0, 0x00, 32*1024*1024, 8, 0);
        addOutBuffer(acapGraph, accelNode2, buffNode2, 0x00, 32*1024*1024, 8, 0);

        addInBuffer (acapGraph, accelNode3, buffNode2, 0x00, 32*1024*1024, 9, 0);
        addOutBuffer(acapGraph, accelNode3, buffNode3, 0x00, 32*1024*1024, 9, 0);
	
	addInBuffer (acapGraph, accelNode4, buffNode1, 0x00, 32*1024*1024, 10, 0);

	addInBuffer (acapGraph, accelNode5, buffNode3, 0x00, 32*1024*1024, 11, 0);
	
        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
	
	for(int i=0; i < 1024; i++){
		p0[i] = i;
	}
	printhex((uint32_t*)p0, 0x50);
        acapGraphSchedule(acapGraph);
	printhex((uint32_t*)p1, 0x50);
	printhex((uint32_t*)p2, 0x50);
        graphFinalise(acapGraph);
	return 0;
}
*/
int case6(){
	INFO("TEST1: CMA buffer with single PL Accelerator\n");
	uint8_t *p0 = (uint8_t*) malloc(64*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(64*1024*1024);
	
	_unused(p0);
	_unused(p1);
	AbstractGraph_t *acapGraph = graphInit();
        AbstractAccelNode_t *accelNode0 = addInputNode(acapGraph, 64*1024*1024);
        AbstractAccelNode_t *accelNode1 = addAcceleratorNode(acapGraph, "fir_compiler");
        AbstractAccelNode_t *accelNode2 = addAcceleratorNode(acapGraph, "FFT4"); //, ENABLE_SCHEDULER);
        AbstractAccelNode_t *accelNode3 = addAcceleratorNode(acapGraph, "aes128encdec");
        AbstractAccelNode_t *accelNode4 = addOutputNode(acapGraph, 64*1024*1024);

	AbstractBuffNode_t *buffNode0 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        AbstractBuffNode_t *buffNode1 = addBuffer(acapGraph, 16*1024*1024, PL_BASED);
        AbstractBuffNode_t *buffNode2 = addBuffer(acapGraph, 16*1024*1024, PL_BASED);
        AbstractBuffNode_t *buffNode3 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
	
	addOutBuffer(acapGraph, accelNode0, buffNode0, 0x00, 64*1024*1024, 1, 0);
	
	addInBuffer (acapGraph, accelNode1, buffNode0, 0x00, 64*1024*1024, 2, 0);
        addOutBuffer(acapGraph, accelNode1, buffNode1, 0x00, 64*1024*1024, 2, 0);
        
	addInBuffer (acapGraph, accelNode2, buffNode1, 0x00, 64*1024*1024, 3, 0);
        addOutBuffer(acapGraph, accelNode2, buffNode2, 0x00, 64*1024*1024, 3, 0);

        addInBuffer (acapGraph, accelNode3, buffNode2, 0x00, 64*1024*1024, 4, 0);
        addOutBuffer(acapGraph, accelNode3, buffNode3, 0x00, 64*1024*1024, 4, 0);
	
	addInBuffer (acapGraph, accelNode4, buffNode3, 0x00, 64*1024*1024, 6, 0);
	
        /*//acapGraphConfig(acapGraph);
	//acapGraphToJson(acapGraph);*/
	//abstractGraph2Json(acapGraph, json);
	abstractGraphConfig(acapGraph);
	//for(int i=0; i < 1024; i++){
	//	p0[i] = i;
	//}
	//printhex((uint32_t*)p0, 0x50);
        //acapGraphSchedule(acapGraph);
	//printhex((uint32_t*)p1, 0x50);
        //graphFinalise(acapGraph);
									
	sleep(5);
	abstractGraphFinalise(acapGraph);
	return 0;
}
/*
int case7(){
	INFO("TEST2: \n");
	uint8_t *p0 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p2 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *c0 = (uint8_t*) malloc(4*1024);
	uint8_t *c1 = (uint8_t*) malloc(4*1024);
	uint8_t *c2 = (uint8_t*) malloc(4*1024);

	uint32_t buff[] = {
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7
	};

	uint32_t keybuff[] = {
		0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203,
		0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
	memcpy(p0, buff, 0x100);
	memcpy(c0, keybuff, 0x20);

	AcapGraph_t *acapGraph = graphInit();
        AccelNode_t *config0 = addInputNode(acapGraph, c0, 4*1024, ENABLE_SCHEDULER);
        
	AccelNode_t *ioNode0 = addInputNode(acapGraph, p0, 32*1024*1024, ENABLE_SCHEDULER);
        
	AccelNode_t *accelNode0 = addAcceleratorNode(acapGraph, "fir_compiler", ENABLE_SCHEDULER);
        AccelNode_t *accelNode1 = addAcceleratorNode(acapGraph, "FFT4", ENABLE_SCHEDULER);
        AccelNode_t *accelNode2 = addAcceleratorNode(acapGraph, "aes128encdec", ENABLE_SCHEDULER);
        
	AccelNode_t *ioNode1 = addOutputNode(acapGraph, p1, 32*1024*1024, ENABLE_SCHEDULER);
	
	BuffNode_t *buffNode0 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        BuffNode_t *buffNode1 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        
	BuffNode_t *buffConf0 = addBuffer(acapGraph, 4*1024, DDR_BASED);
	
	addOutBuffer(acapGraph, config0, buffConf0, 0x400, 0x20, 0, 0);
	addOutBuffer(acapGraph, ioNode0, buffNode0, 0x00, 0x100, 1, 0);
	
	addInBuffer (acapGraph, accelNode2, buffConf0, 0x400, 0x20, 2, 1);
	
	addInBuffer (acapGraph, accelNode2, buffNode0, 0x00, 0x100, 3, 0);
        addOutBuffer(acapGraph, accelNode2, buffNode1, 0x00, 0x100, 3, 0);

	addInBuffer (acapGraph, ioNode1, buffNode1, 0x00, 0x100, 4, 0);

	printhex((uint32_t*)p0, 0x100);
        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
	
	printhex((uint32_t*)p0, 0x100);
        acapGraphSchedule(acapGraph);
	printhex((uint32_t*)p1, 0x100);
        graphFinalise(acapGraph);
	return 0;
}

int case8(){
	int i =0;
	INFO("TEST: FFT only \n");
	uint8_t *p0 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(32*1024*1024);
	//uint8_t *p2 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *c0 = (uint8_t*) malloc(4*1024);

	uint32_t config[] = {0x0000000c,0x0000000c,0x0000000c,0x0000000c};
	
	uint32_t fftbuff[] = {
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000, 0x00000000, 0x00000000, 0x00000000};

	for (i=0;i<(0x1000000/0x100);i++){
			memcpy(p0 + (i*0x100), fftbuff, 0x100);
		}

	memcpy(c0, config, 0x10);

	AcapGraph_t *acapGraph = graphInit();
        AccelNode_t *config0 = addInputNode(acapGraph, c0, 4*1024, ENABLE_SCHEDULER);

	AccelNode_t *ioNode0 = addInputNode(acapGraph, p0, 32*1024*1024, ENABLE_SCHEDULER);

	AccelNode_t *accelNode0 = addAcceleratorNode(acapGraph, "fir_compiler", ENABLE_SCHEDULER);
        AccelNode_t *accelNode1 = addAcceleratorNode(acapGraph, "FFT4", ENABLE_SCHEDULER);
        AccelNode_t *accelNode2 = addAcceleratorNode(acapGraph, "aes128encdec", ENABLE_SCHEDULER);

	AccelNode_t *ioNode1 = addOutputNode(acapGraph, p1, 32*1024*1024, ENABLE_SCHEDULER);

	BuffNode_t *buffNode0 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        BuffNode_t *buffNode1 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);

	BuffNode_t *buffConf0 = addBuffer(acapGraph, 4*1024, DDR_BASED);
	BuffNode_t *buffConf1 = addBuffer(acapGraph, 4*1024, DDR_BASED);
	BuffNode_t *buffConf2 = addBuffer(acapGraph, 4*1024, DDR_BASED);
	BuffNode_t *buffConf3 = addBuffer(acapGraph, 4*1024, DDR_BASED);

	addOutBuffer(acapGraph, config0, buffConf0, 0x000, 0x10, 0, 0);
	addOutBuffer(acapGraph, config0, buffConf1, 0x400, 0x10, 1, 0);
	addOutBuffer(acapGraph, config0, buffConf2, 0x800, 0x10, 2, 0);
	addOutBuffer(acapGraph, config0, buffConf3, 0xc00, 0x10, 3, 0);
	addOutBuffer(acapGraph, ioNode0, buffNode0, 0x00, 0x1000000, 4, 0);

	addInBuffer (acapGraph, accelNode1, buffConf0, 0x00, 0x10, 5, 1);
	addInBuffer (acapGraph, accelNode1, buffConf1, 0x400, 0x10, 6, 2);
	addInBuffer (acapGraph, accelNode1, buffConf2, 0x800, 0x10, 7, 3);
	addInBuffer (acapGraph, accelNode1, buffConf3, 0xc00, 0x10, 8, 4);

	addInBuffer (acapGraph, accelNode1, buffNode0, 0x00, 0x1000000, 9, 0);
        addOutBuffer(acapGraph, accelNode1, buffNode1, 0x00, 0x1000000, 9, 0);

	addInBuffer (acapGraph, ioNode1, buffNode1, 0x00, 0x1000000, 10, 0);

	acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);

	printhex((uint32_t*)p0, 0x100);
	acapGraphSchedule(acapGraph);
	printhex((uint32_t*)p1, 0x100);
	graphFinalise(acapGraph);
	return 0;
}

int case9(){
	INFO("TEST2: \n");
	uint8_t *p0 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p1 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *p2 = (uint8_t*) malloc(32*1024*1024);
	uint8_t *c0 = (uint8_t*) malloc(4*1024);
	uint8_t *c1 = (uint8_t*) malloc(4*1024);
	uint8_t *c2 = (uint8_t*) malloc(4*1024);

	uint32_t buff[] = {
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7,
		0xcca5a729, 0x4b276e90, 0x9a57a7e7, 0xd0bfe1c7
	};

	uint32_t keybuff[] = {
		0x0c0d0e0f, 0x08090a0b, 0x04050607, 0x00010203,
		0x00000000, 0x00000000, 0x00000000, 0x00000000
	};
	memcpy(p0, buff, 0x100);
	memcpy(c0, keybuff, 0x20);

	AcapGraph_t *acapGraph = graphInit();
        AccelNode_t *config0 = addInputNode(acapGraph, c0, 4*1024, DISABLE_SCHEDULER);
        
	AccelNode_t *ioNode0 = addInputNode(acapGraph, p0, 32*1024*1024, DISABLE_SCHEDULER);
        
	AccelNode_t *accelNode0 = addAcceleratorNode(acapGraph, "fir_compiler", DISABLE_SCHEDULER);
        AccelNode_t *accelNode1 = addAcceleratorNode(acapGraph, "FFT4", DISABLE_SCHEDULER);
        AccelNode_t *accelNode2 = addAcceleratorNode(acapGraph, "aes128encdec", DISABLE_SCHEDULER);
        
	AccelNode_t *ioNode1 = addOutputNode(acapGraph, p1, 32*1024*1024, DISABLE_SCHEDULER);
	
	BuffNode_t *buffNode0 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        BuffNode_t *buffNode1 = addBuffer(acapGraph, 16*1024*1024, DDR_BASED);
        
	BuffNode_t *buffConf0 = addBuffer(acapGraph, 4*1024, DDR_BASED);
	
	addOutBuffer(acapGraph, config0, buffConf0, 0x400, 0x20, 0, 0);
	addOutBuffer(acapGraph, ioNode0, buffNode0, 0x00, 0x100, 1, 0);
	
	addInBuffer (acapGraph, accelNode2, buffConf0, 0x400, 0x20, 2, 1);
	
	addInBuffer (acapGraph, accelNode2, buffNode0, 0x00, 0x100, 3, 0);
        addOutBuffer(acapGraph, accelNode2, buffNode1, 0x00, 0x100, 3, 0);

	addInBuffer (acapGraph, ioNode1, buffNode1, 0x00, 0x100, 4, 0);

        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
        acapGraphSchedule(acapGraph);
	
	printhex((uint32_t*)p0, 0x100);
	printhex((uint32_t*)p1, 0x100);

	*//*acapGraphResetLinks(acapGraph);

        graphFinalise(acapGraph);
	addOutBuffer(acapGraph, config0, buffConf0, 0x400, 0x20, 0, 0);
	addOutBuffer(acapGraph, ioNode0, buffNode0, 0x00, 0x100, 1, 0);
	
	addInBuffer (acapGraph, accelNode2, buffConf0, 0x400, 0x20, 2, 1);
	
	addInBuffer (acapGraph, accelNode2, buffNode0, 0x00, 0x100, 3, 0);
        addOutBuffer(acapGraph, accelNode2, buffNode1, 0x00, 0x100, 3, 0);

	addInBuffer (acapGraph, ioNode1, buffNode1, 0x00, 0x100, 4, 0);

        acapGraphConfig(acapGraph);
	acapGraphToJson(acapGraph);
        acapGraphSchedule(acapGraph);
	
	printhex((uint32_t*)p0, 0x100);
	printhex((uint32_t*)p1, 0x100);*//*

	return 0;
}*/

int main(void){
	//case0();
	//case1();
	//case2();
	//case3();
	//case4();
	//case5();
	case6();
	//case7();
	//case8();
	//case9();
	
	return 0;
}


