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

int softgAES128(void** inData, int* inDataSize, void** outData, int* outDataSize){
        INFO("soft AES128 FALLBACK CALLED !!\n");
        _unused(inConfig);
        _unused(inConfigSize);
        _unused(outDataSize);
	INFO("%d %d \n", inConfigSize, inDataSize);
	if(inData[1] != NULL){
		printhex((uint32_t*) inData[1], inDataSize[1]);
	}
	printhex((uint32_t*) inData, 0x200);

        memcpy((void*)outData[0], (void*)inData[0], inDataSize[0]);
        return 0;
}

