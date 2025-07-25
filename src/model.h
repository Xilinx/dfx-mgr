/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    model.h
 * @brief   runtime data model.
 */

#ifndef _ACAPD_MODEL_H
#define _ACAPD_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdbool.h>
#include <dfx-mgr/helper.h>

#define MAX_PATH_SIZE 512
#define RP_SLOTS_MAX 10
#define SLOT_HANDLE_MAX 30 /* MAX number of slot handles */
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#define EPT_DEV_LEN     32
#define EPT_NAME_LEN    32

typedef struct rpmsg_dev{
        /* name used to query - rpmsg-openamp-demo-channel.-1.1024 */
        char rpmsg_channel_name[NAME_MAX];
        /* Store ept dev name (/dev/rpmsg0) */
        char ept_rpmsg_dev_name[EPT_DEV_LEN];
        /* set to 1 if active */
        bool active;
        /* rpmsg_dev_t node list */
	acapd_list_t rpmsg_node;
}rpmsg_dev_t;


typedef struct {
	/* virtio number of rpmsg created by firmware */
        unsigned int virtio_num;
	/* to store rpmsg_ctrl_dev name */
	char rpmsg_ctrl_dev_name[NAME_MAX];
	/* rpmsg dev list for referencing to rpmsg_dev_t */
	acapd_list_t rpmsg_dev_list;
}rpu_info_t;


typedef struct {
	char name[64];
	char path[512];
	int uid;
	int parent_uid; /**< future: 2+ deep hierarchal reconfiguration */
	int is_aie;
	int is_rpu;
	/*
	 * @slot_handle - handle for the slot
	 * value - is the index of an available slot in available_slot_handle
	 * 	   array in platform_info_t structure
	 * This helps in mapping actual slot used to slot_handle
	 * The reason to add this is to differentiate between pl slots and
	 * rpu slots, as they will have the same slot values
	 * example:
	 * 	rpu has 2 slots 0,1
	 * 	pl has 3 slots 0,1,2
	 * 	slot_handle will have the following mapping(slot->slot_handle)
	 * 	rpu slot 0->0 (slot_handle)
	 * 	rpu slot 1->1 (slot_handle)
	 * 	 pl slot 0->2 (slot_handle)
	 * 	 pl slot 1->3 (slot_handle)
	 * 	 pl slot 2->4 (slot_handle)
	 *
	 * Any action on a particular slot needs to be done using slot_handle
	 * and internally the action on the actual slot.
	 */
	int slot_handle;
	acapd_accel_t *accel; // This tracks active PL dfx in this slot
	rpu_info_t rpu; //This stores the rpu specific info in this slot
}slot_info_t;

typedef struct {
	int uid;
	int pid; /**< parent's UID should match shell's UID */
	char name[64];
	char path[512];
	char parent_name[64];
	char parent_path[512];
	int wd;
	char accel_type[32];
}accel_info_t;

struct basePLDesign {
	int uid;
	int fpga_cfg_id;
	char name[64];
	char base_path[512];
	char parent_path[512];
	char type[128];
	uint8_t num_pl_slots;
	uint8_t num_aie_slots;
	int active;
	int wd; //inotify watch desc
	int load_base_design;
	slot_info_t **slots;
	uint64_t inter_rp_comm[RP_SLOTS_MAX]; /**< Inter-RP buffer addrs */
	accel_info_t accel_list[RP_SLOTS_MAX];

	/* Added for User Managed Design*/
	int is_user_load;            /* 1 if user managed design, 0 otherwise */
	int user_load_type;          /* 0 for full, 1 for partial bitstream load */
	int user_load_handle;        /* slot handle for user managed design */
	char user_load_region[128];  /* full or partial reconfiguration region of FPGA in device tree */
};

typedef struct {
	char boardName[128];
	struct basePLDesign* active_base;
	struct basePLDesign* active_rpu_base;
	/*
	 * @available_slot_handle - To track available slot handle
	 * SLOT_HANDLE_MAX - max slots possible for RPU and PL
	 * value 1 in array indicates used
	 * value 0 in array indicates un-used
	 * When accel/rpu is loaded,
	 * 1) This array is seached for an un-used index
	 * 2) The index is assigned to slot_handle which is part of slot_info_t
	 * structure
	 * 3) the value in that index is set to 1,marking as used
	 *
	 * When accel/rpu is unloaded/removed,
	 * 1) slot_handle needs to be passed
	 * 2) slot associated with the slot_handle is identified and removed
	 * 3) The value available_slot_handle[slot_handle] is set to 1
	 *    marking it as un-used and can be used for next load
	 */
	int available_slot_handle[SLOT_HANDLE_MAX]; /* To track available slot handles */
}platform_info_t;

struct daemon_config {
	char defaul_accel_name[64];
	char **firmware_locations;
	int number_locations;
	unsigned int rpu_fw_uptime_msec; /* store rpu_fw_uptime_msec from config file */
};

#ifdef __cplusplus
}
#endif

#endif
