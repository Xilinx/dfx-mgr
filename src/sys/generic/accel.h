/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	linux/accel.h
 * @brief	Linux specific accelerator definition.
 */

#ifndef _ACAPD_LINUX_ACCEL_H
#define _ACAPD_LINUX_ACCEL_H

#include <stdint.h>

/**
 * @brief accel system specific information
 */
typedef struct {
	uint64_t accel_json_pa;
	uint64_t accel_json_size;
	uint64_t accel_pdi_pa;
	uint64_t accel_pdi_size;
} acapd_accel_sys_t;

#endif /* _ACAPD_LINUX_ACCEL_H */
