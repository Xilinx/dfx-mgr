/*
 * Copyright (C) 2022 - 2026 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    user_load.h
 * @brief   sysfs/configfs FPGA loading helpers.
 */

#ifndef _ACAPD_USER_LOAD_H
#define _ACAPD_USER_LOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DTBO_ROOT_DIR
#define DTBO_ROOT_DIR "/sys/kernel/config/device-tree/overlays"
#endif

const char *path_basename(const char *path);
int user_load_bitstream(const char *bitstream, const char *overlay,
			const char *region, int is_partial);
void remove_overlay_dir(const char *dir);

#ifdef __cplusplus
}
#endif

#endif
