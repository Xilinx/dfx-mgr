/******************************************************************************
* Copyright (C) 2019 - 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

#include <adf.h>
#include "config.h"

void TwoInputs(input_window_int32 * dataIn, input_window_int32 * bypassResult,
	       output_window_int32 * dataOut, output_window_int32 * result)
{
	static unsigned a[NUM_A_ELMNTS_PER_TILE];
	static unsigned b[NUM_COLS];
	static unsigned intrmdtResult[WIN_SIZE];
	static unsigned count = 0;
	static unsigned currentCol;

	currentCol = (get_coreid() & 0x7F0000) >> 16;

	for (unsigned i = 0; i < NUM_A_ELMNTS_PER_TILE / WIN_SIZE; i++) {
		window_acquire(dataIn);
		for (unsigned w = 0; w < WIN_SIZE; w++)
			a[i * WIN_SIZE + w] = window_readincr(dataIn);
		window_release(dataIn);
	}

	for (unsigned i = 0;
		      i < NUM_A_ELMNTS_PER_TILE *
		      		(NUM_HW_COLS - currentCol - 1) / WIN_SIZE;
		      i++)
	{
		window_acquire(dataOut);
		window_acquire(dataIn);
		for (unsigned w = 0; w < WIN_SIZE; w++)
			window_writeincr(dataOut, window_readincr(dataIn));
		window_release(dataIn);
		window_release(dataOut);
	}

	/*
	 * read one column of b, pass the same to the next core,
	 * compute matrix multiplication of 'a' rows x 'b' col and
	 * finally output the result
	 */
	for (unsigned i = 0; i < NUM_COLS; i++) {
		/* read 1 entire column of b */
		for (unsigned w = 0; w < (NUM_COLS / WIN_SIZE); w++) {
			window_acquire(dataOut);
			window_acquire(dataIn);
			for (unsigned x = 0; x < (WIN_SIZE); x++) {
				b[w * WIN_SIZE + x] = window_readincr(dataIn);
				window_writeincr(dataOut, b[w * WIN_SIZE + x]);
			}
			window_release(dataIn);
			window_release(dataOut);
		}

		for (unsigned k = 0; k < NUM_ROWS_PER_TILE; k++) {
			for (unsigned l = 0; l < NUM_COLS; l++)
				intrmdtResult[count] += a[k * NUM_COLS + l] * b[l];
			count++;
		}

		if (count == WIN_SIZE) {
			/*
			 * copy the results from previous cores to the output
			 * window
			 */
			for (unsigned j = 0; j < currentCol; j++) {
				window_acquire(result);
				window_acquire(bypassResult);
				for (unsigned k = 0; k < WIN_SIZE; k++)
					window_writeincr(result,
							 window_readincr(bypassResult));
				window_release(bypassResult);
				window_release(result);
			}

			window_acquire(result);
			for (unsigned z = 0; z < WIN_SIZE; z++) {
				window_writeincr(result, intrmdtResult[z]);
				intrmdtResult[z] = 0;
			}
			window_release(result);
			count = 0;
		}
	}
}
