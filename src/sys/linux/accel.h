/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	linux/accel.h
 * @brief	Linux specific accelerator definition.
 */

#ifndef _ACAPD_LINUX_ACCEL_H
#define _ACAPD_LINUX_ACCEL_H
#include <stdio.h>
#include <dfx-mgr/helper.h>
/**
 * @brief accel system specific information
 */
typedef struct {
	char dtbo_dir[MAX_PATH_SIZE];
	char tmp_dir[MAX_PATH_SIZE];
	int  fpga_cfg_id;
} acapd_accel_sys_t;

#endif /* _ACAPD_LINUX_ACCEL_H */
