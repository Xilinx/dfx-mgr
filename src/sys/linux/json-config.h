/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	linux/json-config.h
 * @brief	Linux specific Json config definition.
 */


#ifndef _ACAPD_LINUX_JSON_CONFIG_H
#define _ACAPD_LINUX_JSON_CONFIG_H

#include <acapd/accel.h>
#include <acapd/shell.h>
#include <acapd/model.h>

int parseAccelJson(acapd_accel_t *accel, char *filename);

int parseShellJson(acapd_shell_t *shell, const char *filename);

int initBaseDesign(struct basePLDesign *base, const char *shell_path);
#endif /* _ACAPD_LINUX_JSON_CONFIG_H */
