/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/dma.h>
#include <acapd/assert.h>
#include <acapd/shm.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int acapd_dma_config(acapd_chnl_t *chnl, acapd_shm_t *shm,
		     acapd_shape_t *stride,
		     uint32_t auto_repeat)
{
	if (chnl == NULL) {
		acapd_perror("%s: channel pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops == NULL) {
		acapd_perror("%s: channel ops is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops->config == NULL) {
		acapd_perror("%s: channel config dma op is NULL.\n", __func__);
		return -EINVAL;
	}
	return chnl->ops->config(chnl, shm, stride, auto_repeat);
}

int acapd_dma_start(acapd_chnl_t *chnl, acapd_fence_t *fence)
{
	if (chnl == NULL) {
		acapd_perror("%s: channel pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops == NULL) {
		acapd_perror("%s: channel ops is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops->start == NULL) {
		acapd_perror("%s: channel start dma op is NULL.\n", __func__);
		return -EINVAL;
	}
	return chnl->ops->start(chnl, fence);
}

int acapd_dma_stop(acapd_chnl_t *chnl)
{
	if (chnl == NULL) {
		acapd_perror("%s: channel pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops == NULL) {
		acapd_perror("%s: channel ops is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops->stop == NULL) {
		acapd_perror("%s: channel stop dma op is NULL.\n", __func__);
		return -EINVAL;
	}
	return chnl->ops->stop(chnl);
}

int acapd_dma_poll(acapd_chnl_t *chnl, uint32_t wait_for_complete)
{
	if (chnl == NULL) {
		acapd_perror("%s: channel pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops == NULL) {
		acapd_perror("%s: channel ops is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops->poll == NULL) {
		acapd_perror("%s: channel poll dma op is NULL.\n", __func__);
		return -EINVAL;
	}
	return chnl->ops->poll(chnl, wait_for_complete);
}

int acapd_dma_open(acapd_chnl_t *chnl)
{
	int ret;

	acapd_debug("%s\n", __func__);
	if (chnl == NULL) {
		acapd_perror("%s: channel pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	if (chnl->is_open != 0) {
		return 0;
	}
	if (chnl->ops == NULL || chnl->ops->open == NULL) {
		acapd_perror("%s: no open channel ops.\n", __func__);
		return -EINVAL;
	}
	ret = chnl->ops->open(chnl);
	if (ret != 0) {
		acapd_perror("%s: system failed to open DMA channel.\n",
			     __func__);
		return -EINVAL;
	}
	chnl->is_open = 1;
	return 0;
}

int acapd_dma_close(acapd_chnl_t *chnl)
{
	acapd_debug("%s\n", __func__);
	if (chnl->is_open == 0) {
		return 0;
	}
	chnl->is_open = 0;
	if (chnl->ops == NULL) {
		return 0;
	}
	if (chnl->ops->close == NULL) {
		return 0;
	}
	return chnl->ops->close(chnl);
}

int acapd_create_dma_channel(const char *name, const char *dev_name,
			     int iommu_group,
			     acapd_chnl_conn_t conn_type, int chnl_id,
			     acapd_dir_t dir, acapd_chnl_t *chnl)
{
	if (chnl == NULL) {
		acapd_perror("%s: channel pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	chnl->dev_name = dev_name;
	chnl->iommu_group = iommu_group;
	chnl->chnl_id = chnl_id;
	chnl->dir = dir;
	chnl->conn_type = conn_type;
	if (name != NULL) {
		unsigned int len;

		len = strlen(name);
		if (len > sizeof(chnl->name) - 1) {
			len = sizeof(chnl->name) - 1;
		}
		memset(chnl->name, 0, sizeof(chnl->name));
		strncpy(chnl->name, name, len);
	}
	return acapd_dma_open(chnl);
}

int acapd_destroy_dma_channel(acapd_chnl_t *chnl)
{
	if (chnl == NULL) {
		acapd_perror("%s: channel pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	return acapd_dma_close(chnl);
}
