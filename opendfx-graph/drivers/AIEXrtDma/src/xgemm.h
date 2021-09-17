/******************************************************************************
* Copyright (C) 2019 - 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

#include <adf.h>
#include "kernels.h"
#include "kernels/config.h"

using namespace adf;

class XGeMM : public adf::graph {
	private:
  		kernel krnl[NUM_HW_ROWS][NUM_HW_COLS];
	public:
		input_port matrix_ab[NUM_HW_ROWS];
		output_port result[NUM_HW_ROWS];

		XGeMM() {
			for (int r = 0; r < NUM_HW_ROWS; r++) {
				krnl[r][0] = kernel::create(OneInput);
				source(krnl[r][0]) = "kernels/one_input.cc";
				runtime<ratio>(krnl[r][0]) = 0.9;
				location<kernel>(krnl[r][0]) = tile(0, r);

				for (int i = 1; i < NUM_HW_COLS - 1; i++) {
					krnl[r][i] = kernel::create(TwoInputs);
					source(krnl[r][i]) = "kernels/two_inputs.cc";
					runtime<ratio>(krnl[r][i]) = 0.9;
					location<kernel>(krnl[r][i]) = tile(i, r);
				}

				krnl[r][NUM_HW_COLS - 1] = kernel::create(OneOutput);
				source(krnl[r][NUM_HW_COLS - 1]) = "kernels/one_output.cc";
				runtime<ratio>(krnl[r][NUM_HW_COLS - 1]) = 0.9;
				location<kernel>(krnl[r][NUM_HW_COLS - 1]) = tile(NUM_HW_COLS - 1, r);

				connect<window<WIN_SIZE_BYTES>> (matrix_ab[r], async(krnl[r][0].in[0]));

				for (int i = 0; i < NUM_HW_COLS - 1; i++) {
   					connect<window<WIN_SIZE_BYTES>> (async(krnl[r][i].out[0]), async(krnl[r][i + 1].in[0]));
					connect<window<WIN_SIZE_BYTES>> (async(krnl[r][i].out[1]), async(krnl[r][i + 1].in[1]));
				}

				connect<window<WIN_SIZE_BYTES>> (async(krnl[r][NUM_HW_COLS - 1].out[0]), result[r]);
			}
		}
};
