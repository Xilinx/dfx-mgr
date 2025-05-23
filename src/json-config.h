/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	linux/json-config.h
 * @brief	Linux specific Json config definition.
 */


#ifndef _ACAPD_LINUX_JSON_CONFIG_H
#define _ACAPD_LINUX_JSON_CONFIG_H

#include <dfx-mgr/accel.h>
#include <dfx-mgr/shell.h>
#include <dfx-mgr/model.h>

int parseAccelJson(acapd_accel_t *accel, char *filename);

int parseShellJson(acapd_shell_t *shell, const char *filename);
int initBaseDesign(struct basePLDesign *base, const char *shell_path);
void parse_config(char *config_path, struct daemon_config *config);
int initAccel(accel_info_t *accel, const char *path);

#endif /* _ACAPD_LINUX_JSON_CONFIG_H */
