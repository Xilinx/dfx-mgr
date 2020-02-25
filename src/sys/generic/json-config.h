/*
 * Copyright (c) 2020, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	generic/json-config.h
 * @brief	Generic specific Json config definition.
 */


#ifndef _ACAPD_GENERIC_JSON_CONFIG_H
#define _ACAPD_GENERIC_JSON_CONFIG_H

#include <acapd/accel.h>
#include <acapd/shell.h>

int parseAccelJson(acapd_accel_t *accel, const char *config, size_t csize);

int parseShellJson(acapd_shell_t *shell, const char *config, size_t csize);
#endif /* _ACAPD_GENERIC_JSON_CONFIG_H */
