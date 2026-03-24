/*
 * Copyright (C) 2022 - 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    user_load.h
 * @brief   sysfs/configfs FPGA loading and platform detection helpers.
 */

#ifndef _ACAPD_USER_LOAD_H
#define _ACAPD_USER_LOAD_H

#include <dfx-mgr/model.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DTBO_ROOT_DIR
#define DTBO_ROOT_DIR "/sys/kernel/config/device-tree/overlays"
#endif

int is_user_load_platform(void);
const char *path_basename(const char *path);
int user_load_bitstream(const char *bitstream, const char *overlay,
			const char *region, int is_partial);
int user_load_from_dir(const char *search_path, const char *region,
		       int is_partial);
void user_load_init_accel(acapd_accel_t *pl_accel,
			  acapd_accel_pkg_hd_t *pkg,
			  int slot_num, const char *accel_type);
void remove_overlay_dir(const char *dir);

#ifdef __cplusplus
}
#endif

#endif
