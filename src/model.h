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

typedef struct {
	int uid;
	int parent_uid;
	acapd_accel_t *accel;
}slot_info_t;

typedef struct {
	int uid;
	char name[64];
	char path[512];
	char parent_path[512];
	int wd;
}accel_info_t;

struct basePLDesign {
	int uid;
	char name[64];
	char base_path[512];
	char parent_path[512];
	char type[128];
	int num_slots;
	slot_info_t **slots;
	int active;
	int wd; //inotify watch desc
	accel_info_t accel_list[10];
};

typedef struct {
	char boardName[128];
	struct basePLDesign* active_base;
}platform_info_t;

#ifdef __cplusplus
}
#endif

#endif
