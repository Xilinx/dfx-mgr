/*
 * Copyright (C) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
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

/**
 * get_new_rpmsg_ctrl_dev() - return new rpmsg control dev found
 * @struct basePLDesign *base - PL base design
 *
 * This function checks if new rpmsg control dev is created by
 * RPU firmware and return the same, it does this by parsing through
 * /sys/device/rpmsg/devices directory takes each entries and comparing
 * with the previously stored data in base design structure
 *
 * Return: rpmsg_ctrl_dev_name on success
 *         NULL on failure
 *
 */
char* get_new_rpmsg_ctrl_dev(struct basePLDesign *base);

/**
 * get_virtio_number() - return virtio number
 * @rpmsg_dev_name - rpmsg device or control name
 *
 * This function taken input rpmsg dev or control name
 * and returns the virtio number
 *
 * Return: virtio number on success
 *         0 if not found
 */
int get_virtio_number(char* rpmsg_dev_name);

/**
 * update_rpmsg_dev_list() - updates rpmsg device list and setup
 * end point
 * @acapd_list_t *rpmsg_dev_list -- pointer to rpmsg_dev_list
 * @rpmsg_ctrl_dev -- rpmsg control dev
 * @virtio_num -- virtio number
 *
 * This function updates rpmsg_dev_list by parsing through /sys/bus/rpmsg/devices
 * directory and does the following:
 * 1) remove deleted rpmsg dev from list
 * 2) setups up rpmsg end point dev for new rpmsg virtio dev and ctrl
 * 3) add new entry to rpmsg dev list
 *
 * Return:  void
 *
 */
void update_rpmsg_dev_list(acapd_list_t *rpmsg_dev_list, char* rpmsg_ctrl_dev, int virtio_num);

/**
 * delete_rpmsg_dev_list - delete complete rpmsg_dev_list
 * @acapd_list_t *rpmsg_dev_list -- pointer to rpmsg_dev_list
 *
 * This function delete complete list
 * Called when RPU is removed
 *
 */
void delete_rpmsg_dev_list(acapd_list_t *rpmsg_dev_list);


#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_RPU_H */
