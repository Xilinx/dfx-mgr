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

#include <xrt.h>
#include <xrt/xrt_bo.h>
#include <xclbin.h>
#include <xrt/xrt_device.h>

typedef struct xrt_device_info {
	char xclbin[100];
	uint8_t xrt_device_id;
	xclDeviceHandle device_hdl;
	xuid_t xrt_uid;
} xrt_device_info_t;

typedef struct {
	char name[64];
	char path[512];
	int uid;
	int parent_uid;
	int is_aie;
	acapd_accel_t *accel; // This tracks active PL dfx in this slot
	xrt_device_info_t *aie; // This tracks active AIE dfx in this slot
}slot_info_t;

typedef struct {
	int uid;
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
	accel_info_t accel_list[10];
};

typedef struct {
	char boardName[128];
	struct basePLDesign* active_base;
}platform_info_t;

struct daemon_config {
	char defaul_accel_name[64];
};

#ifdef __cplusplus
}
#endif

#endif
