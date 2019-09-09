/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/print.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DTBO_ROOT_DIR "/sys/kernel/config/device-tree/overlays"

int sys_load_accel(acapd_accel_t *accel, unsigned int async)
{
	struct timeval tv;
	char dtbo_path[256];
	char cmd[512];
	int ret;
	int fd;

	/* TODO: only support synchronous mode for now */
	(void)async;
	acapd_assert(accel->pkg.path != NULL);

	/* Use timestamp to name the DTBO directory */
	gettimeofday(&tv, NULL);
	sprintf(accel->sys_info.dtbo_dir, "accel.%ld.%ld",
		tv.tv_sec, tv.tv_usec);
	sprintf(dtbo_path, "%s/%s", DTBO_ROOT_DIR, accel->sys_info.dtbo_dir);
	acapd_debug("Creating %s.\n", dtbo_path);
	sprintf(cmd, "mkdir %s", dtbo_path);
	ret = system(cmd);
	if (ret != 0) {
		acapd_perror("Failed to create %s: %s.\n",
			     accel->sys_info.dtbo_dir, strerror(errno));
		return ACAPD_ACCEL_FAILURE;
	}
	sprintf(dtbo_path, "%s/%s/path", DTBO_ROOT_DIR,
		accel->sys_info.dtbo_dir);
	accel->status = ACAPD_ACCEL_STATUS_LOADING;
	fd = open(dtbo_path, O_WRONLY);
	if (fd < 0) {
		acapd_perror("Failed to open %s: %s.\n", dtbo_path,
			     strerror(errno));
		accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
		accel->load_failure = ACAPD_ACCEL_INVALID;
		return ACAPD_ACCEL_FAILURE;
	}
	ret = write(fd, accel->pkg.path, strlen(accel->pkg.path) + 1);
	if (ret < 0) {
		acapd_perror("Failed to apply device tree overlay, %s\n",
			     strerror(errno));
		accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
		accel->load_failure = ACAPD_ACCEL_INVALID;
		return ACAPD_ACCEL_FAILURE;
	}
	/* We only apply device tree overlay for now.
	 * TODO: Launch container, call post accel configure callback?
	 * post accel configure callback is required, as user will need
	 * to define what to pass to the container.
	 */
	accel->status = ACAPD_ACCEL_STATUS_INUSE;
	return ACAPD_ACCEL_SUCCESS;
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{
	char dtbo_path[256];
	struct stat s;
	int ret;

	/* TODO: for now, only synchronous mode is supported */
	(void)async;
	acapd_assert(accel != NULL);
	/* Unload dtbo for now.
	 * TODO: remove container service.
	 */
	sprintf(dtbo_path, "%s/%s", DTBO_ROOT_DIR, accel->sys_info.dtbo_dir);
	/* Unload dtbo.
	 * TODO: will clear bitstream when FPGA util is ready.
	 */
	rmdir(dtbo_path);
	ret = stat(dtbo_path, &s);
	if (ret < 0) {
		if (errno == ENOENT) {
			accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
			return ACAPD_ACCEL_SUCCESS;
		}
	}
	acapd_perror("Failed to unload accel, not able to remove dtbo.\n");
	return ACAPD_ACCEL_FAILURE;
}
