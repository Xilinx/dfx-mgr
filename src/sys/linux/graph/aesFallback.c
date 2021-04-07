/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "layer0/utils.h"
#include "aes128.h"
#include <dfx-mgr/print.h>
#include <dfx-mgr/assert.h>

#define BLOCKSIZE 16

int encrypt128Buffer(unsigned char* key, unsigned char* buffer, int bytes){
    struct AES_ctx ctx;
	int i;
    AES_init_ctx(&ctx, key);
	for (i = 0; i < bytes/BLOCKSIZE; i++){
		AES_ECB_encrypt(&ctx, &buffer[BLOCKSIZE * i]);  
	}
	return 0;
}

int decrypt128Buffer(unsigned char* key, unsigned char* buffer, int bytes){
    struct AES_ctx ctx;
	int i;
    AES_init_ctx(&ctx, key);

	for (i = 0; i < bytes/BLOCKSIZE; i++){	
		AES_ECB_decrypt(&ctx, &buffer[BLOCKSIZE * i]);  
	}
	return 0;
}
/*
int encrypt192Buffer(unsigned char* key, unsigned char* buffer, int bytes){
    struct AES_192_ctx ctx;
	int i;
    AES_192_init_ctx(&ctx, key);
	for (i = 0; i < bytes/BLOCKSIZE; i++){
		AES_192_ECB_encrypt(&ctx, &buffer[BLOCKSIZE * i]);  
	}
	return 0;
}

int decrypt192Buffer(unsigned char* key, unsigned char* buffer, int bytes){
    struct AES_192_ctx ctx;
	int i;
    AES_192_init_ctx(&ctx, key);

	for (i = 0; i < bytes/BLOCKSIZE; i++){	
		AES_192_ECB_decrypt(&ctx, &buffer[BLOCKSIZE * i]);  
	}
	return 0;
}
*/
int softgAES128(void** inData, int* inDataSize, void** outData, int* outDataSize){
        INFO("soft AES128 FALLBACK CALLED !!\n");
        _unused(outDataSize);
	int DOENC = 0;
	uint8_t key[32];
	if(inData[1] != NULL){
		memcpy(key, inData[1], 16);
		DOENC = *((uint8_t*)inData[1] + 16);
	}
	if (DOENC){
		encrypt128Buffer(key, inData[0], inDataSize[0]);
	}
	else{
		decrypt128Buffer(key, inData[0], inDataSize[0]);
	}
        memcpy((void*)outData[0], (void*)inData[0], inDataSize[0]);
        return 0;
}
/*
int softgAES192(void** inData, int* inDataSize, void** outData, int* outDataSize){
        INFO("soft AES128 FALLBACK CALLED !!\n");
        _unused(outDataSize);
	int DOENC = 0;
	uint8_t key[32];
	if(inData[1] != NULL){
		memcpy(key, inData[1], 16);
		DOENC = *((uint8_t*)inData[1] + 16);
	}
	if (DOENC){
		encrypt192Buffer(key, inData[0], inDataSize[0]);
	}
	else{
		decrypt192Buffer(key, inData[0], inDataSize[0]);
	}
        memcpy((void*)outData[0], (void*)inData[0], inDataSize[0]);
        return 0;
}
*/
