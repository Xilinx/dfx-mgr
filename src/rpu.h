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

/* Constants for RPU handling */
#define RPU_TYPE_STR "RPU"
#define RPU_FIRMWARE_EXT ".elf"
#define RPU_FIRMWARE_NAME_MAX 128

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
 * @*firmware_name - optional specific firmware filename (for new structure)
 *                   NULL for old structure (loads all .elf files)
 *
 * This function does the following
 * 1 - Set the firmware location path
 * 2 - Load the firmware (specific file or all files based on firmware_name)
 * 3 - start the firmware
 *
 * Return: slot_number on success
 *         -1 on error
 */
int load_rpu(char *rpu_path, int rpu_slot, char *firmware_name);

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
 * get_new_rpmsg_ctrl_dev() - find a newly created rpmsg control device
 * @*base - PL base design containing existing slot records
 * @*buf - caller-provided buffer to store the device name
 * @buflen - size of @buf in bytes
 *
 * This function checks if new rpmsg control dev is created by
 * RPU firmware and return the same, it does this by parsing through
 * /sys/device/rpmsg/devices directory takes each entries and comparing
 * with the previously stored data in base design structure
 *
 * Return: @buf on success, NULL if no new control device is found
 */
char* get_new_rpmsg_ctrl_dev(struct basePLDesign *base, char *buf,
			     size_t buflen);

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

/**
 * validate_rpu_slot_availability() - Check if an RPU slot is valid and available
 * @base: Base design containing the RPU slots
 * @slot_num: RPU slot number to validate
 *
 * Return: 0 on success (slot is valid and available)
 *        -DFX_MGR_NO_EMPTY_SLOT_ERROR if slot is out of bounds or occupied
 */
int validate_rpu_slot_availability(struct basePLDesign *base, int slot_num);

/**
 * finalize_rpu_slot_setup() - Complete RPU slot setup after firmware load
 * @base: Base design containing the RPU
 * @slot: Slot structure to initialize
 * @pl_accel: Accelerator structure (can be NULL for RPU)
 * @accel_name: Name of the accelerator
 * @slot_num: Slot number where RPU was loaded
 * @rpu_fw_uptime_msec: Time in ms to wait for firmware initialization
 *
 * Return: slot_handle on success, -1 on error
 */
int finalize_rpu_slot_setup(struct basePLDesign *base, slot_info_t *slot,
			    acapd_accel_t *pl_accel, const char *accel_name,
			    int slot_num, unsigned int rpu_fw_uptime_msec);

/**
 * parse_rpu_slot_dir() - Parse a single RPU slot directory for .elf files
 * @base: Pointer to base design structure
 * @slot_path: Path to slot directory
 * @slot_num: Slot number
 * @rpu_path: Parent RPU path
 */
void parse_rpu_slot_dir(struct basePLDesign *base, char *slot_path,
			int slot_num, char *rpu_path);


#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_RPU_H */
