/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/device.h>
#include <acapd/print.h>
#include <acapd/shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	
	ret = sys_accel_config(accel);
	if (ret < 0) {
		acapd_perror("%s: failed to config accel.\n", __func__);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = acapd_shell_config(shell_config);
	if (ret < 0) {
		acapd_perror("%s: failed to config shell.\n", __func__);
		return ACAPD_ACCEL_FAILURE;
	}
	return ret;
}
int load_full_bitstream(char *base_path)
{
	acapd_accel_t *accel = malloc(sizeof(acapd_accel_t));;
	int ret;

	memset(accel, 0, sizeof(*accel));
	sprintf(accel->sys_info.tmp_dir,"%s",base_path);
	ret = sys_fetch_accel(accel, 0);
	if (ret != ACAPD_ACCEL_SUCCESS) {
		acapd_perror("%s: Failed to fetch Full Bitstream.\n",__func__);
		return ret;
	}
	ret = sys_load_accel(accel, 0, 1);
	if (ret < 0)
		acapd_perror("%s: Error loading full bitstream\n",__func__);
	return ret;
}

int load_accel(acapd_accel_t *accel, const char *shell_config, unsigned int async)
{
	int ret;

	acapd_assert(accel != NULL);

	acapd_debug("%s: config accel.\n", __func__);
	ret = acapd_parse_config(accel, shell_config);
	if (ret < 0) {
		acapd_perror("%s: failed to parse config files.\n", __func__);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = sys_needs_load_accel(accel);
	if (ret == 0) {
		acapd_debug("%s: no need to load accel.\n", __func__);
		return 0;
	//} else {
	//	ret = acapd_shell_get(shell_config);
	//	if (ret < 0) {
	//		acapd_perror("%s: failed to get shell.\n", __func__);
	//		return ACAPD_ACCEL_FAILURE;
	//	}
	}
	/* assert isolation before programming */
	ret = acapd_shell_assert_isolation(accel);
	if (ret < 0) {
		acapd_perror("%s, failed to assert isolaction.\n",
			     __func__);
		return ret;
	}
	/* TODO: Check if the accel is valid */
	/* For now, for now assume it is always PDI/DTB */
	if (accel->is_cached == 0) {
		ret = sys_fetch_accel(accel, 1);
		if (ret != ACAPD_ACCEL_SUCCESS) {
			acapd_perror("%s, failed to fetch partial bistream\n",__func__);
			return ret;
		}
		accel->is_cached = 1;
	}
	ret = sys_load_accel(accel, async, 0);
	if (ret == ACAPD_ACCEL_SUCCESS) {
		accel->status = ACAPD_ACCEL_STATUS_INUSE;
	} else if (ret == ACAPD_ACCEL_INPROGRESS) {
		accel->status = ACAPD_ACCEL_STATUS_LOADING;
	} else {
		acapd_perror("%s: Failed to load partial bitstream\n",__func__);
		accel->load_failure = ret;
	}
	if (accel->status == ACAPD_ACCEL_STATUS_INUSE) {
		ret = acapd_shell_release_isolation(accel);
		if (ret != 0) {
			acapd_perror("%s: failed to release isolation.\n",__func__);
			return ACAPD_ACCEL_FAILURE;
		}
		acapd_debug("%s: releasing isolation done.\n", __func__);
	}
	ret = sys_load_accel_post(accel);
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

int remove_accel(acapd_accel_t *accel, unsigned int async)
{
	acapd_assert(accel != NULL);
	if (accel->status == ACAPD_ACCEL_STATUS_UNLOADED) {
		acapd_perror("%s: accel is not loaded.\n", __func__);
		return ACAPD_ACCEL_SUCCESS;
	} else if (accel->status == ACAPD_ACCEL_STATUS_UNLOADING) {
		acapd_perror("%s: accel is unloading .\n", __func__);
		return ACAPD_ACCEL_INPROGRESS;
	} else {
		int ret;

		ret = sys_close_accel(accel);
		if (ret < 0) {
			acapd_perror("%s: failed to close accel.\n", __func__);
			return ACAPD_ACCEL_FAILURE;
		}
		ret = sys_needs_load_accel(accel);
		if (ret == 0) {
			acapd_debug("%s: no need to load accel.\n", __func__);
			accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
			return ACAPD_ACCEL_SUCCESS;
		} else {
			ret = acapd_shell_assert_isolation(accel);
			if (ret < 0) {
				acapd_perror("%s, failed to assert isolaction.\n",
				             __func__);
				return ret;
			}
			ret = sys_remove_accel(accel, async);
			//acapd_shell_put();
		}
		if (ret == ACAPD_ACCEL_SUCCESS) {
			acapd_debug("%s:Succesfully removed accel\n");
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
	acapd_device_t *dev;

	acapd_assert(accel != NULL);
	dev = NULL;
	if (name == NULL) {
		dev = &accel->ip_dev[0];
	} else {
		for (int i = 0; i < accel->num_ip_devs; i++) {
			acapd_device_t *tmpdev;

			tmpdev = &accel->ip_dev[i];
			if (strcmp(tmpdev->dev_name, name) == 0) {
				dev = tmpdev;
				break;
			}
		}
	}
	if (dev == NULL) {
		acapd_perror("%s: failed to get %s device.\n", __func__, name);
		return NULL;
	}
	if (dev->va == NULL) {
		int ret;

		ret = acapd_device_open(dev);
		if (ret < 0) {
			acapd_perror("%s: failed to open dev %s.\n",
			     __func__, dev->dev_name);
			return NULL;
		}
	}
	return dev->va;
}

void get_fds(acapd_accel_t *accel, int slot)
{
	acapd_assert(accel != NULL);
	sys_get_fds(accel, slot);
}

void get_PA(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	sys_get_PA(accel);
}
void get_shell_fd(acapd_accel_t *accel)
{
	sys_get_fd(accel, acapd_shell_fd());	
}

void get_shell_clock_fd(acapd_accel_t *accel)
{
	sys_get_fd(accel, acapd_shell_clock_fd());
}
