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

acapd_accel_pkg_hd_t *acapd_alloc_pkg(size_t size)
{
	acapd_accel_pkg_hd_t *pkg;

	pkg = malloc(size);
	if (pkg == NULL) {
		return pkg;
	}
	memset(pkg, 0, size);
	return pkg;
}

int acapd_config_pkg(acapd_accel_pkg_hd_t *pkg, uint32_t type, char *name,
		     size_t size, void *data, int is_end)
{
	acapd_accel_pkg_hd_t *tmppkg;
	char *pkgdata;

	acapd_assert(pkg != NULL);
	if (type >= ACAPD_ACCEL_PKG_TYPE_LAST) {
		acapd_perror("Failed to config pkg, non supported type %u.\n",
			     type);
		return ACAPD_ACCEL_FAILURE;
	}
	tmppkg = pkg;
	while(tmppkg->type != 0) {
		tmppkg = (acapd_accel_pkg_hd_t *)((char *)tmppkg +
						   sizeof(*tmppkg) +
						   tmppkg->size);
	}
	tmppkg->type = type;
	tmppkg->size = (uint64_t)size;
	tmppkg->is_end = is_end;
	pkgdata = (char *)tmppkg + sizeof(*tmppkg);
	memset(tmppkg->name, 0, sizeof(tmppkg->name));
	if (name != NULL) {
		size_t nsize;

		nsize = sizeof(tmppkg->name) - 1;
		if (nsize < strlen(name)) {
			nsize = strlen(name);
		}
		strncpy(tmppkg->name, name, nsize);
	}

	memcpy(pkgdata, data, size);
	return ACAPD_ACCEL_SUCCESS;
}

void init_accel(acapd_accel_t *accel, acapd_accel_pkg_hd_t *pkg)
{
	acapd_assert(accel != NULL);
	//acapd_assert(pkg != NULL);
	memset(accel, 0, sizeof(*accel));
	accel->pkg = pkg;
	accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
}

int acapd_accel_config(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	return sys_accel_config(accel);
}

int load_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret;

	acapd_assert(accel != NULL);
	acapd_assert(accel->pkg != NULL);
	if (accel->rm_slot < 0) {
		accel->rm_slot = 0;
	}
	/* assert isolation before programming */
	acapd_debug("%s: config accel.\n", __func__);
	ret = acapd_accel_config(accel);
	if (accel->shell_dev == NULL) {
		if (accel->num_chnls == 0) {
			acapd_perror("%: no shell, and no channels.\n", __func__);
			return ACAPD_ACCEL_FAILURE;
		} else {
			acapd_debug("%s: no shell, but has channels.\n");
			return ACAPD_ACCEL_SUCCESS;
		}
	}

	acapd_debug("%s: assert isolation.\n", __func__);
	ret = acapd_shell_assert_isolation(accel, accel->rm_slot);
	if (ret < 0) {
		acapd_perror("%s, failed to assert isolaction.\n",
			     __func__);
		return ret;
	}
	/* TODO: Check if the accel is valid */
	/* For now, for now assume it is always PDI/DTB */
	acapd_debug("%s: load accel.\n", __func__);
	ret = sys_load_accel(accel, async);
	if (ret == ACAPD_ACCEL_SUCCESS) {
		accel->status = ACAPD_ACCEL_STATUS_INUSE;
	} else if (ret == ACAPD_ACCEL_INPROGRESS) {
		accel->status = ACAPD_ACCEL_STATUS_LOADING;
	} else {
		accel->load_failure = ret;
	}
	acapd_debug("%s: loaded pdi.\n", __func__);
	if (accel->status == ACAPD_ACCEL_STATUS_INUSE) {
		acapd_debug("%s: releasing isolation.\n", __func__);
		ret = acapd_shell_release_isolation(accel, accel->rm_slot);
		if (ret != 0) {
			acapd_perror("%s: failed to release isolation.\n");
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
		return ACAPD_ACCEL_SUCCESS;
	} else if (accel->status == ACAPD_ACCEL_STATUS_UNLOADING) {
		return ACAPD_ACCEL_INPROGRESS;
	} else {
		int ret;

		/* TODO: FIXME: hardcoded to assert isolation */
		if (accel->rm_slot < 0) {
			accel->rm_slot = 0;
		}
		ret = acapd_shell_assert_isolation(accel, accel->rm_slot);
		if (ret < 0) {
			acapd_perror("%s, failed to assert isolaction.\n",
				     __func__);
			return ret;
		}
		ret = sys_remove_accel(accel, async);
		if (ret == ACAPD_ACCEL_SUCCESS) {
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
