/******************************************************************************
* Copyright (C) 2019 - 2021 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

#ifndef XGEMM_START_H
#define XGEMM_START_H

#include <adf.h>

void OneInput(input_window_int32 * dataIn,
		output_window_int32 * dataOut,
		output_window_int32 * result);

void TwoInputs(input_window_int32 * dataIn,
		input_window_int32 * bypassResult,
		output_window_int32 * dataOut,
		output_window_int32 * result);


void OneOutput(input_window_int32 * dataIn,
		input_window_int32 * bypassResult,
		output_window_int32 * result);

#endif
