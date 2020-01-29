/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/device.h>
#include <acapd/print.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	acapd_assert(pkg != NULL);
	memset(accel, 0, sizeof(*accel));
	accel->pkg = pkg;
	accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
}

int load_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret;

	acapd_assert(accel != NULL);
	acapd_assert(accel->pkg != NULL);
	/* TODO: Check if the accel is valid */
	/* For now, for now assume it is always PDI/DTB */
	ret = sys_load_accel(accel, async);
	if (ret == ACAPD_ACCEL_SUCCESS) {
		accel->status = ACAPD_ACCEL_STATUS_INUSE;
	} else if (ret == ACAPD_ACCEL_INPROGRESS) {
		accel->status = ACAPD_ACCEL_STATUS_LOADING;
	} else {
		accel->load_failure = ret;
	}
	if (accel->status == ACAPD_ACCEL_SUCCESS) {
		/* TODO: FIXME: hardcoded to release isolation */
		void *reg_va;
		acapd_device_t *dev;

		if (accel->rm_dev == NULL) {
			acapd_debug("%s: no rm dev specified.\n",
				     __func__);
			return ret;
		}

		dev = accel->rm_dev;
		reg_va = dev->va;
		if (reg_va == NULL) {
			ret = acapd_device_open(dev);
			if (ret < 0) {
				acapd_perror("%s: failed to open rm dev %s.\n",
					     __func__, dev->dev_name);
				return ACAPD_ACCEL_FAILURE;
			}
			reg_va = dev->va;
			if (reg_va == NULL) {
				acapd_perror("%s: rm dev %s va is NULL.\n",
					     __func__, dev->dev_name);
				return ACAPD_ACCEL_FAILURE;
			}
		}
		*((volatile uint32_t *)((char *)reg_va + 0x10000)) = 0x1;
		*((volatile uint32_t *)((char *)reg_va + 0x0)) = 0x1;
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

int remove_accel(acapd_accel_t *accel, unsigned int async)
{
	acapd_assert(accel != NULL);
	if (accel->status == ACAPD_ACCEL_STATUS_UNLOADED) {
		return ACAPD_ACCEL_SUCCESS;
	} else if (accel->status == ACAPD_ACCEL_STATUS_UNLOADING) {
		return ACAPD_ACCEL_INPROGRESS;
	} else {
		int ret;
		void *reg_va;
		acapd_device_t *dev;

		/* TODO: FIXME: hardcoded to assert isolation */
		if (accel->rm_dev == NULL) {
			acapd_debug("%s: no rm dev specified. Failed isolation.\n",
				     __func__);
		} else {
			dev = accel->rm_dev;
			reg_va = dev->va;
			if (reg_va == NULL) {
				ret = acapd_device_open(dev);
				if (ret < 0) {
					acapd_perror("%s: failed to open rm dev %s.\n",
						     __func__, dev->dev_name);
					return ACAPD_ACCEL_FAILURE;
				}
				reg_va = dev->va;
				if (reg_va == NULL) {
					acapd_perror("%s: rm dev %s va is NULL.\n",
						     __func__, dev->dev_name);
						return ACAPD_ACCEL_FAILURE;
				}
			}
			*((volatile uint32_t *)((char *)reg_va + 0x10000)) = 0x1;
			*((volatile uint32_t *)((char *)reg_va + 0x0)) = 0x1;

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
