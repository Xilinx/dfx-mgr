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
/**
 * @brief accel system specific information
 */
typedef struct {
	char dtbo_dir[128];
	char tmp_dir[128];
	int  fpga_cfg_id;
} acapd_accel_sys_t;

#endif /* _ACAPD_LINUX_ACCEL_H */
