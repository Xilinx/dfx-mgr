/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
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

#include <dfx-mgr/accel.h>

typedef struct acapd_shell {
	acapd_device_t dev;
	acapd_device_t clock_dev;
	char *type;
	int is_configured;
} acapd_shell_t;

int acapd_shell_config(const char *config);
int acapd_shell_get();
int acapd_shell_put();
void dfx_shell_uio_list(char *, size_t);
char *dfx_shell_uio_by_name(const char *);

#ifdef ACAPD_INTERNAL
int sys_shell_config(acapd_shell_t *shell, const char *config);
#endif /* ACAPD_INTERNAL */
/** @} */
#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_SHELL_H */
