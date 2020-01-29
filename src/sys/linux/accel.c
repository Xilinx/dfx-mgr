/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/print.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>
#include <libfpga.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DTBO_ROOT_DIR "/sys/kernel/config/device-tree/overlays"

static int remove_directory(const char *path)
{
	DIR *d = opendir(path);
	int r = -1;

	if (d) {
		struct dirent *p;
		size_t path_len;

		path_len = strlen(path);
		r = 0;
		while (!r && (p=readdir(d))) {
			int r2 = -1;
			char *buf;
			size_t len;
			struct stat statbuf;

			/* Skip the names "." and ".." as we don't want
			 * to recurse on them. */
			if (!strcmp(p->d_name, ".") ||
			    !strcmp(p->d_name, "..")) {
				continue;
			}
			len = path_len + strlen(p->d_name) + 2;
			buf = malloc(len);
			if (buf == NULL) {
				acapd_perror("Failed to allocate memory.\n");
				return -1;
			}

			sprintf(buf, "%s/%s", path, p->d_name);
			if (!stat(buf, &statbuf)) {
				if (S_ISDIR(statbuf.st_mode)) {
					r2 = remove_directory(buf);
				} else {
					r2 = unlink(buf);
				}
			}
			r = r2;
			free(buf);
		}
		closedir(d);
		if (r == 0) {
			r = rmdir(path);
		}
	}
	return r;
}

int sys_load_accel(acapd_accel_t *accel, unsigned int async)
{
	acapd_accel_pkg_hd_t *pkg;
	char template[] = "/tmp/accel.XXXXXX";
	char *tmp_dirname;
	char cmd[512];
	int ret;
	int fpga_cfg_id;
	char *pkg_name;

	/* TODO: only support synchronous mode for now */
	(void)async;
	/* Use timestamp to name the tmparary directory */
	acapd_debug("Creating tmp dir for package.\n");
	tmp_dirname = mkdtemp(template);
	if (tmp_dirname == NULL) {
		acapd_perror("Failed to create tmp dir for package:%s.\n",
			     strerror(errno));
		return ACAPD_ACCEL_FAILURE;
	}
	sprintf(accel->sys_info.tmp_dir, "%s/", tmp_dirname);
	pkg = accel->pkg;
	pkg_name = (char *)pkg;
	/* TODO: Assuming the package is a tar.gz format */
	sprintf(cmd, "tar -C %s -xzf %s", tmp_dirname, pkg_name);
	ret = system(cmd);
	if (ret != 0) {
		acapd_perror("Failed to extract package %s.\n", pkg_name);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = fpga_cfg_init(accel->sys_info.tmp_dir, 0, 0);
	if (ret < 0) {
		acapd_perror("Failed to initialize fpga config, %d.\n", ret);
		return ACAPD_ACCEL_FAILURE;
	}
	fpga_cfg_id = ret;
	accel->sys_info.fpga_cfg_id = fpga_cfg_id;
	acapd_print("loading %d.\n",  fpga_cfg_id);
	ret = fpga_cfg_load(fpga_cfg_id);
	if (ret < 0) {
		acapd_perror("Failed to load fpga config: %d\n",
			     fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	} else {
		return ACAPD_ACCEL_SUCCESS;
	}
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret, fpga_cfg_id;

	/* TODO: for now, only synchronous mode is supported */
	(void)async;
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	if (accel->sys_info.tmp_dir != NULL) {
		ret = remove_directory(accel->sys_info.tmp_dir);
		if (ret != 0) {
			acapd_perror("Failed to remove %s, %s\n",
				     accel->sys_info.tmp_dir, strerror(errno));
		}
	}
	if (fpga_cfg_id <= 0) {
		acapd_perror("Invalid fpga cfg id: %d.\n", fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	};
	ret = fpga_cfg_remove(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to remove accel: %d.\n", ret);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = fpga_cfg_destroy(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to destroy accel: %d.\n", ret);
		return ACAPD_ACCEL_FAILURE;
	}
	return ACAPD_ACCEL_SUCCESS;
}
