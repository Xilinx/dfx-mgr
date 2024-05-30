/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
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

#define MAX_PATH_SIZE 512
#define RP_SLOTS_MAX 10
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

typedef struct {
	char name[64];
	char path[512];
	int uid;
	int parent_uid; /**< future: 2+ deep hierarchal reconfiguration */
	int is_aie;
	int is_rpu;
	acapd_accel_t *accel; // This tracks active PL dfx in this slot
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
};

typedef struct {
	char boardName[128];
	struct basePLDesign* active_base;
	struct basePLDesign* active_rpu_base;
}platform_info_t;

struct daemon_config {
	char defaul_accel_name[64];
	char **firmware_locations;
	int number_locations;
};

#ifdef __cplusplus
}
#endif

#endif
