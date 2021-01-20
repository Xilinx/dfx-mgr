/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/shell.h>
#include <dfx-mgr/device.h>
#include <dfx-mgr/assert.h>
#include <errno.h>
#include "json-config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern unsigned char __START_SHELL_JSON[];
extern unsigned char __END_SHELL_JSON[];

int sys_shell_config(acapd_shell_t *shell, const char *config)
{
	const char *shell_config;
	size_t csize;
	int ret;

	(void)config;
	acapd_assert(shell != NULL);
	shell_config = (const char *)__START_SHELL_JSON;
	csize = __END_SHELL_JSON - __START_SHELL_JSON;
	ret = parseShellJson(shell, shell_config, csize);
	if (ret != 0) {
		return ACAPD_ACCEL_FAILURE;
	}
	shell->dev.va = (void *)((uintptr_t)shell->dev.reg_pa);
	acapd_debug("%s: shell va = %p, pa = 0x%lx.\n",
		    __func__, shell->dev.va, shell->dev.reg_pa);

	return ACAPD_ACCEL_SUCCESS;
}

