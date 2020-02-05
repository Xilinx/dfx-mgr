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

typedef struct acapd_shell_regs {
	uint32_t clock_release;
	uint32_t clock_status;
	uint32_t reset_release;
	uint32_t reset_status;
	uint32_t clock_release_mask;
	uint32_t reset_release_mask;
} acapd_shell_regs_t;

typedef struct acapd_shell {
	acapd_device_t dev;
	char *type;
	const acapd_shell_regs_t *regs;
	int is_configured;
} acapd_shell_t;

int acapd_shell_config(const char *config);
int acapd_shell_get();
int acapd_shell_put();
int acapd_shell_release_isolation(acapd_accel_t *accel);
int acapd_shell_assert_isolation(acapd_accel_t *accel);

#ifdef ACAPD_INTERNAL
int sys_shell_config(acapd_shell_t *shell, const char *config);
#endif /* ACAPD_INTERNAL */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_SHELL_H */
