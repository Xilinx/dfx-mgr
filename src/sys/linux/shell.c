/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/shell.h>
#include <dfx-mgr/device.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/json-config.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int sys_shell_config(acapd_shell_t *shell, const char *config)
{
	int ret;

	if (config != NULL) {
		if (access(config, F_OK) != 0) {
			acapd_debug("%s: config %s doesn't exist, check config env.\n",
				    __func__, config);
		}
	}
	if (config == NULL) {
		config = getenv("ACAPD_SHELL_CONFIG");
	}
	if (config == NULL || access(config, F_OK) != 0) {
		config = "./shell.json";
	}
	if (access(config, F_OK) == 0) {
		ret = parseShellJson(shell, config);
		if (ret < 0) {
			acapd_perror("%s: failed to parse Shell json %s.\n",
					 __func__, config);
			return ACAPD_ACCEL_FAILURE;
		}
	}
	return ACAPD_ACCEL_SUCCESS;
}

