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

/**
 * load_rpu() - load and start RPU firmware
 * @*rpu_path - location of the firmware
 * @rpu_slot - RPU number to load the firmware
 *
 * This function does the following
 * 1 - Set the firmware location path
 * 2 - Load the firmware
 * 3 - start the firmware
 *
 * Return: slot_number on success
 *         -1 on error
 */
int load_rpu( char *rpu_path, int rpu_slot);

/**
 * remove_rpu() - remove/stop a loaded RPU firmware
 * @rpu_num - remoteproc number
 *
 * This function takes remoteproc number as input and
 * stops/removes the firmware running on it
 *
 * Return: 0 on success
 *        -1 on error
 */
int remove_rpu(int slot);
#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_ACCEL_H */
