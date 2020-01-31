/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	shell.h
 * @brief	shell primitives.
 */

#ifndef _ACAPD_SHELL_H
#define _ACAPD_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup shell SHELL Interfaces
 *  @{ */

#include <acapd/accel.h>

int acapd_shell_release_isolation(acapd_accel_t *accel, int slot);
int acapd_shell_assert_isolation(acapd_accel_t *accel, int slot);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_SHELL_H */
