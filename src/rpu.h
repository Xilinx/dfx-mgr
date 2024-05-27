/*
 * Copyright (C) 2022 - 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	rpu.h
 * @brief	rpu definition.
 */

#ifndef _ACAPD_RPU_H
#define _ACAPD_RPU_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * get_number_of_rpu() - Get the number of remoteproc's created
 *
 * This function looks into /sys/class/remoteproc directory and
 * identifies the number of remoteproc entries available, this
 * will be the number of rpu's
 *
 * Return: Number of rpu
 */
int get_number_of_rpu(void);

#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_ACCEL_H */
