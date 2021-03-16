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

struct RM {
	char rm_path[512];	
};
struct pl_slot {
	int uid;
	struct RM *rm;
};
struct basePLDesign {
	char base_path[512];
	char type[128];
	int num_slots;
	struct pl_slot* slots;
	int active;
};

struct pl_config {
	struct basePLDesign* base;
};
struct platform {
	char boardName[128];
	struct pl_config* PL;	
};

struct daemon_config {
	char defaul_accel_name[64];
};

#ifdef __cplusplus
}
#endif

#endif
