/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/assert.h>

int compare(int counts, uint32_t *address, uint32_t *reference){
	int i;
	int error = 0;
	uint32_t value;
	for (i = 0; i < counts >> 2; i++){
		value = *(address + i);
		if (value != reference[i]){
			printf("\n %x != %x Error: Doesn't matched with the reference !!!!\n", value, reference[i]);
			error = 1;
			break;
		}
	}
	if (error==0){
		printf("\n Success: Matched with the reference !!!!\n");
	}
	printf("\n");
	return 0;
}

// prints string as hex
void printhex(uint32_t* data, uint32_t len)
{
	uint32_t i;
	for (i = 0; i < len >> 2; ++i){
		if(i%4==0) fprintf(stderr, "\n%.8x: ", i << 2);
		fprintf(stderr, "0x%.8x, ", data[i]);
	}
	fprintf(stderr, "\n");
}

