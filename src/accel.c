/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/device.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/shell.h>
#include <libdfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dfx-mgr/user_load.h>

void init_accel(acapd_accel_t *accel, acapd_accel_pkg_hd_t *pkg)
{
	acapd_assert(accel != NULL);
	memset(accel, 0, sizeof(*accel));
	accel->pkg = pkg;
	accel->rm_slot = -1;
	accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
}

int acapd_parse_config(acapd_accel_t *accel, const char *shell_config)
{
	int ret;

	/* Flat design don't have accel.json so do not parse */
	if(!strcmp(accel->type,"XRT_AIE_DFX")){
		ret = sys_accel_config(accel);
		if (ret < 0) {
			DFX_ERR("failed to config accel");
			return ACAPD_ACCEL_FAILURE;
		}
	}
	ret = acapd_shell_config(shell_config);
	if (ret < 0) {
		DFX_ERR("failed to config shell");
		return ACAPD_ACCEL_FAILURE;
	}
	return ret;
}

static int get_fpga_flags(acapd_accel_t *accel)
{
	int flags = DFX_NORMAL_EN;
	char *dtbo_path;
	FILE *fp;
	long size;
	char *buf;

	dtbo_path = find_overlay_file(accel->sys_info.tmp_dir);
	if (!dtbo_path)
		return DFX_NORMAL_EN;

	fp = fopen(dtbo_path, "rb");
	if (!fp) {
		free_overlay_file_path(dtbo_path);
		return DFX_NORMAL_EN;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	if (size > 0) {
		buf = malloc(size);
		if (buf && fread(buf, 1, size, fp) == (size_t)size) {
			const char *needle = "external-fpga-config";
			size_t nlen = strlen(needle);

			for (long i = 0; i <= size - (long)nlen; i++) {
				if (memcmp(buf + i, needle, nlen) == 0) {
					flags = DFX_EXTERNAL_CONFIG_EN;
					break;
				}
			}
		}
		free(buf);
	}

	fclose(fp);
	free_overlay_file_path(dtbo_path);
	return flags;
}

int load_accel(acapd_accel_t *accel, const char *shell_config, unsigned int async)
{
	int ret;

	acapd_assert(accel != NULL);
	DFX_DBG("accel=%s shell_config=%s", accel->pkg->name, shell_config);
	ret = acapd_parse_config(accel, shell_config);
	if (ret < 0) {
		DFX_ERR("failed to parse config files");
		return ACAPD_ACCEL_FAILURE;
	}
	ret = sys_needs_load_accel(accel);
	if (ret == 0) {
		DFX_DBG("no need to load accel");
		return 0;
	}

	if (accel->is_cached == 0) {
		ret = sys_fetch_accel(accel, get_fpga_flags(accel));
		if (ret != ACAPD_ACCEL_SUCCESS) {
			DFX_ERR("failed to fetch partial bistream");
			return ret;
		}
		accel->is_cached = 1;
	}
	ret = sys_load_accel(accel, async);
	if (ret == ACAPD_ACCEL_SUCCESS) {
		accel->status = ACAPD_ACCEL_STATUS_INUSE;
	} else if (ret == ACAPD_ACCEL_INPROGRESS) {
		accel->status = ACAPD_ACCEL_STATUS_LOADING;
	} else {
		DFX_ERR("Failed to load partial bitstream ret %d", ret);
		accel->load_failure = ret;
		return ret;
	}
	return ret;
}

int accel_load_status(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	if (accel->load_failure != ACAPD_ACCEL_SUCCESS) {
		return accel->load_failure;
	} else if (accel->status != ACAPD_ACCEL_STATUS_INUSE) {
		return ACAPD_ACCEL_INVALID;
	} else {
		return ACAPD_ACCEL_SUCCESS;
	}
}
int remove_base(int fpga_cfg_id)
{
	if (fpga_cfg_id <= 0) {
		DFX_ERR("Invalid fpga_cfg_id: %d", fpga_cfg_id);
		return -1;
	}
	return sys_remove_base(fpga_cfg_id);
}

int remove_accel(acapd_accel_t *accel, unsigned int async)
{
	acapd_assert(accel != NULL);
	if (accel->status == ACAPD_ACCEL_STATUS_UNLOADED) {
		DFX_ERR("accel is not loaded");
		return ACAPD_ACCEL_SUCCESS;
	} else if (accel->status == ACAPD_ACCEL_STATUS_UNLOADING) {
		DFX_ERR("accel is unloading");
		return ACAPD_ACCEL_INPROGRESS;
	} else {
		int ret;
		ret = sys_close_accel(accel);
		if (ret < 0) {
			DFX_ERR("failed to close accel");
			return ACAPD_ACCEL_FAILURE;
		}
		ret = sys_needs_load_accel(accel);
		if (ret == 0) {
			DFX_DBG("no need to load accel");
			accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
			return ACAPD_ACCEL_SUCCESS;
		} else {
			ret = sys_remove_accel(accel, async);
		}
		if (ret == ACAPD_ACCEL_SUCCESS) {
			DFX_DBG("Succesfully removed accel");
			accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
		} else if (ret == ACAPD_ACCEL_INPROGRESS) {
			accel->status = ACAPD_ACCEL_STATUS_UNLOADING;
		} else {
			accel->status = ACAPD_ACCEL_STATUS_UNLOADING;
		}
		return ret;
	}
}

int acapd_accel_wait_for_data_ready(acapd_accel_t *accel)
{
	/* TODO: always ready */
	(void)accel;
	return 1;
}

int acapd_accel_open_channel(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	acapd_assert(accel->chnls != NULL);
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_chnl_t *chnl = NULL;
		int ret;

		chnl = &accel->chnls[i];
		acapd_debug("%s: opening chnnl\n", __func__);
		ret = acapd_dma_open(chnl);
		if (ret < 0) {
			acapd_perror("%s: failed to open channel.\n", __func__);
			return ret;
		}
	}
	return 0;
}

/* This function only reset AXIS DMA channel not the accelerator
 */
int acapd_accel_reset_channel(acapd_accel_t *accel)
{
	int ret;

	acapd_assert(accel != NULL);
	acapd_assert(accel->chnls != NULL);
	ret = acapd_accel_open_channel(accel);
	if (ret < 0) {
		acapd_perror("%s: open channel fails.\n", __func__);
		return ret;
	}
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_chnl_t *chnl = NULL;

		chnl = &accel->chnls[i];
		acapd_dma_stop(chnl);
		acapd_dma_reset(chnl);
	}
	return 0;
}

void *acapd_accel_get_reg_va(acapd_accel_t *accel, const char *name)
{
	acapd_device_t *dev = NULL;

	acapd_assert(accel != NULL);
	if (name == NULL) {
		DFX_ERR("empty device name");
		return NULL;
	}
	for (int i = 0; i < accel->num_ip_devs; i++) {
		if (strstr(accel->ip_dev[i].dev_name, name)) {
			dev = &accel->ip_dev[i];
			break;
		}
	}

	if (dev == NULL) {
		DFX_ERR("failed to find device: %s", name);
		return NULL;
	}

	DFX_DBG("%s: match %s", name, dev->dev_name);
	if (dev->va == NULL) {
		int ret = acapd_device_open(dev);

		if (ret < 0) {
			DFX_ERR("failed to open dev %s", dev->dev_name);
			return NULL;
		}
	}
	return dev->va;
}

