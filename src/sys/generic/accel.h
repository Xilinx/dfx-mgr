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
	uint32_t reserved;
} acapd_accel_sys_t;

#endif /* _ACAPD_LINUX_ACCEL_H */
