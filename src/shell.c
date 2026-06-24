/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/shell.h>
#include <dfx-mgr/device.h>
#include <dfx-mgr/assert.h>
#include <stdio.h>
#include <string.h>

static acapd_shell_t shell;

int acapd_shell_config(const char *config)
{
	return sys_shell_config(&shell, config);
}

char *dfx_shell_uio_by_name(const char *str)
{
	if (shell.dev.dev_name && strstr(shell.dev.dev_name, str))
		return shell.dev.path;
	if (shell.clock_dev.dev_name && strstr(shell.clock_dev.dev_name, str))
		return shell.clock_dev.path;
	return NULL;
}

void dfx_shell_uio_list(char *buf, size_t sz)
{
	/*
	 * Assume strlen(..dev_name) < sizeof(shell.dev.path) and
	 * we need to print two pairs: 4 * bigger_size should work
	 */
	assert(sz > 4 * sizeof(shell.dev.path));
	if (shell.dev.dev_name)
		buf += sprintf(buf, "%-30s %s\n", shell.dev.dev_name, shell.dev.path);
	if (shell.clock_dev.dev_name)
		sprintf(buf, "%-30s %s\n", shell.clock_dev.dev_name, shell.clock_dev.path);
}
