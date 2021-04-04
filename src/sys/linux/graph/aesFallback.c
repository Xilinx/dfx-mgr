/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "layer0/debug.h"
#include "layer0/utils.h"

int softgAES128(void* inData, int inDataSize, void* inConfig, int inConfigSize, void* outData, int outDataSize){
        INFO("soft AES128 FALLBACK CALLED !!\n");
        _unused(inConfig);
        _unused(inConfigSize);
        _unused(outDataSize);
	INFO("%d %d \n", inConfigSize, inDataSize);
	if(inConfig != NULL){
		printhex((uint32_t*) inConfig, inConfigSize);
	}
	printhex((uint32_t*) inData, 0x200);

        memcpy((void*)outData, (void*)inData, inDataSize);
        return 0;
}

