/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/json-config.h>
#include <errno.h>
#include <dirent.h>
#include <libdfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "accel.h"

static int remove_directory(const char *path)
{
	DIR *d = opendir(path);
	int r = -1;

	DFX_DBG("%s", path);
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
				DFX_ERR("Failed to allocate memory");
				closedir(d);
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

int sys_accel_config(acapd_accel_t *accel)
{
	acapd_accel_pkg_hd_t *pkg = accel->pkg;
	char template[] = "/tmp/accel.XXXXXX";
	char *tmp_dirname;
	char cmd[512];
	int ret;
	char *env_config_path, config_path[128];
	size_t len;

	DFX_DBG("%s", pkg ? pkg->path : "");
	env_config_path = getenv("ACCEL_CONFIG_PATH");
	memset(config_path, 0, sizeof(config_path));
	if(env_config_path == NULL) {
		if(pkg->type == ACAPD_ACCEL_PKG_TYPE_TAR_GZ) {
			DFX_DBG("Found .tar.gz package, extracting %s",
				pkg->path);
			tmp_dirname = mkdtemp(template);
			if (tmp_dirname == NULL) {
				DFX_ERR("mkdtemp");
				return ACAPD_ACCEL_FAILURE;
			}
			sprintf(accel->sys_info.tmp_dir, "%s/", tmp_dirname);
			sprintf(cmd, "tar -C %s -xzf %s", tmp_dirname, pkg->path);
			ret = system(cmd);
			if (ret != 0) {
				DFX_ERR("%s", cmd);
				return ACAPD_ACCEL_FAILURE;
			}
			len = sizeof(config_path) - strlen("accel.json") - 1;
			if (len > strlen(accel->sys_info.tmp_dir)) {
				len = strlen(accel->sys_info.tmp_dir);
			} else {
				DFX_ERR("path is too long: %s", tmp_dirname);
				return ACAPD_ACCEL_FAILURE;
			}
		}
		else if(pkg->type == ACAPD_ACCEL_PKG_TYPE_NONE) {
			DFX_DBG("No need to extract pkg %s", pkg->path);
			sprintf(accel->sys_info.tmp_dir, "%s/", pkg->path);
			len = strlen(accel->sys_info.tmp_dir);
		}
		else {
			DFX_ERR("unhandled package type for accel %s",
				pkg->path);
			return ACAPD_ACCEL_FAILURE;
		}
		strncpy(config_path, accel->sys_info.tmp_dir, len);
		strcat(config_path, "accel.json");
	} else {
		if (sizeof(config_path) < strlen(env_config_path)) {
			DFX_ERR("path is too long: %s", env_config_path);
			return ACAPD_ACCEL_FAILURE;
		}
		strncpy(config_path, env_config_path, sizeof(config_path));
	}
	DFX_DBG("Json Path %s", config_path);
	parseAccelJson(accel, config_path);
	if (sys_needs_load_accel(accel) == 0) {
		for (int i = 0; i < accel->num_ip_devs; i++) {
			char *tmppath;
			char tmpstr[32];
			acapd_device_t *dev;

			dev = &(accel->ip_dev[i]);
			sprintf(tmpstr, "ACCEL_IP%d_PATH", i);
			tmppath = getenv(tmpstr);
			if (tmppath == NULL)
				break;

			if (strlen(tmppath) >= sizeof(dev->path)) {
				DFX_ERR("%s is too long: %s", tmpstr, tmppath);
				return ACAPD_ACCEL_FAILURE;
			}
			strncpy(dev->path, tmppath, sizeof(dev->path));
		}
	}
	return ACAPD_ACCEL_SUCCESS;
}

int sys_needs_load_accel(acapd_accel_t *accel)
{
	char *tmpstr;

	(void)accel;
	tmpstr = getenv("ACCEL_CONFIG_PATH");
	DFX_DBG("ACCEL_CONFIG_PATH %s", tmpstr);
	if (tmpstr != NULL) {
		DFX_DBG("no need to load");
		return 0;
	} else {
		return 1;
	}
}

int sys_fetch_accel(acapd_accel_t *accel, int flags)
{
	int ret;

	DFX_DBG("");
	acapd_assert(accel != NULL);
	if (accel->cma_path[0] != '\0') {
		DFX_PR("Using CMA path: %s", accel->cma_path);
		ret = dfx_cfg_init(accel->sys_info.tmp_dir, 0, flags, accel->cma_path);
	} else {
		DFX_PR("Using default CMA path");
		ret = dfx_cfg_init(accel->sys_info.tmp_dir, 0, flags);
	}
	if (ret < 0) {
		DFX_ERR("Failed to initialize fpga config for %s, ret=%d",
			accel->sys_info.tmp_dir, ret);
		return ACAPD_ACCEL_FAILURE;
	}
	accel->sys_info.fpga_cfg_id = ret;
	return ACAPD_ACCEL_SUCCESS;
}

int sys_load_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret;
	int fpga_cfg_id;
	(void)async;

	DFX_DBG("");
	acapd_assert(accel != NULL);
	if (accel->is_cached == 0) {
		DFX_ERR("accel is not cached.");
		return ACAPD_ACCEL_FAILURE;
	}
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	ret = dfx_cfg_load(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to load fpga config for %s (id=%d)",
			accel->sys_info.tmp_dir, fpga_cfg_id);
		dfx_cfg_destroy(fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	}
	if ( !strcmp(accel->type,"XRT_FLAT") || !strcmp(accel->type, "PL_FLAT")) {
		DFX_PR("Successfully loaded base design.");
		return ACAPD_ACCEL_SUCCESS;
	}
	for (int i = 0; i < accel->num_ip_devs; i++) {
		int ret;
		ret = acapd_device_open(&accel->ip_dev[i]);
		if (ret != 0) {
			DFX_ERR("failed to open accel ip %s",
				accel->ip_dev[i].dev_name);
			return -EINVAL;
		}
	}
	return ACAPD_ACCEL_SUCCESS;
}

int sys_close_accel(acapd_accel_t *accel)
{
	DFX_DBG("");
	/* Close devices and free memory */
	acapd_assert(accel != NULL);
	for (int i = 0; i < accel->num_chnls; i++) {
		DFX_DBG("closing channel %d", i);
		acapd_dma_close(&accel->chnls[i]);
	}
	if (accel->num_chnls > 0) {
		free(accel->chnls[0].dev->dev_name);
		free(accel->chnls[0].dev);
		free(accel->chnls);
		accel->chnls = NULL;
		accel->num_chnls = 0;
	}
	for (int i = 0; i < accel->num_ip_devs; i++) {
		DFX_DBG("closing accel ip %d %s", i,
			accel->ip_dev[i].dev_name ? accel->ip_dev[i].dev_name : "(null)");
		acapd_device_close(&accel->ip_dev[i]);
		free(accel->ip_dev[i].dev_name);
		accel->ip_dev[i].dev_name = NULL;
	}
	if (accel->num_ip_devs > 0) {
		free(accel->ip_dev);
		accel->ip_dev = NULL;
		accel->num_ip_devs = 0;
	}
	return 0;
}
int sys_remove_base(int fpga_cfg_id)
{
	int ret;

	DFX_DBG("");
	ret = dfx_cfg_remove(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to remove base (fpga_cfg_id=%d): %d",
			fpga_cfg_id, ret);
		return -1;
	}
	ret = dfx_cfg_destroy(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to destroy base (fpga_cfg_id=%d): %d",
			fpga_cfg_id, ret);
		return -1;
	}
	return 0;
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret, fpga_cfg_id;

	DFX_DBG("");
	/* TODO: for now, only synchronous mode is supported */
	(void)async;
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	DFX_DBG("Removing accel %s", accel->sys_info.tmp_dir);
	if (accel->pkg->type == ACAPD_ACCEL_PKG_TYPE_TAR_GZ) {
		DFX_DBG("Removing tmp dir for .tar.gz");
		ret = remove_directory(accel->sys_info.tmp_dir);
		if (ret != 0) {
			DFX_ERR("Failed to remove %s", accel->sys_info.tmp_dir);
		}
	}
	if (fpga_cfg_id <= 0) {
		DFX_ERR("Invalid fpga cfg id: %d", fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = dfx_cfg_remove(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to remove accel %s (fpga_cfg_id=%d): %d",
			accel->sys_info.tmp_dir, fpga_cfg_id, ret);
		goto out;
	}
	ret = dfx_cfg_destroy(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to destroy accel %s (fpga_cfg_id=%d): %d",
			accel->sys_info.tmp_dir, fpga_cfg_id, ret);
		goto out;
	}
out:
	return ACAPD_ACCEL_SUCCESS;
}
