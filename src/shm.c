/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/dma.h>
#include <acapd/helper.h>
#include <acapd/shm.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

void *acapd_alloc_shm(char *shm_allocator_name, acapd_shm_t *shm, size_t size,
		      uint32_t attr)
{
	void *va;
	(void)shm_allocator_name;
	shm->refcount = 0;
	acapd_list_init(&shm->refs);
	va = acapd_default_shm_allocator.alloc(&acapd_default_shm_allocator,
					       shm, size, attr);
	if (va != NULL) {
		shm->size = size;
	}
	return va;
}

int acapd_free_shm(acapd_shm_t *shm)
{
	acapd_list_t *node;

	/* Detach shared memory first */
	acapd_list_for_each(&shm->refs, node) {
		acapd_chnl_t *chnl;

		chnl = (acapd_chnl_t *)acapd_container_of(node, acapd_chnl_t,
							  node);
		if (chnl->ops && chnl->ops->munmap) {
			chnl->ops->munmap(chnl, shm);
		}
	}
	shm->refcount = 0;
	acapd_list_init(&shm->refs);
	acapd_default_shm_allocator.free(&acapd_default_shm_allocator, shm);
	return 0;
}

int acapd_attach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm)
{
	int already_attached;
	acapd_list_t *node;

	if (chnl == NULL) {
		acapd_perror("%s: failed, chnl is NULL.\n", __func__);
		return -EINVAL;
	}
	if (shm == NULL) {
		acapd_perror("%s: failed due to shm is NULL\n", __func__);
		return -EINVAL;
	}
	/* Check if the memory has been attached to the channel */
	already_attached = 0;
	acapd_list_for_each(&shm->refs, node) {
		acapd_chnl_t *lchnl;

		lchnl = (acapd_chnl_t *)acapd_container_of(node, acapd_chnl_t,
							   node);
		if (chnl == lchnl) {
			already_attached = 1;
			break;
		}
	}
	if (already_attached == 0) {
		if (chnl->ops && chnl->ops->mmap != NULL) {
			acapd_debug("%s: calling channel mmap op.\n",
				    __func__);
			chnl->ops->mmap(chnl, shm);
		}
		acapd_list_add_tail(&shm->refs, &chnl->node);
		shm->refcount++;
	}
	return 0;
}

int acapd_detach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm)
{
	acapd_list_t *node;

	if (chnl == NULL) {
		acapd_perror("%s: failed, chnl is NULL.\n", __func__);
		return -EINVAL;
	}
	if (shm == NULL) {
		acapd_perror("%s: failed due to shm is NULL\n", __func__);
		return -EINVAL;
	}
	/* Check if the memory has been attached to the channel */
	acapd_list_for_each(&shm->refs, node) {
		acapd_chnl_t *lchnl;

		lchnl = (acapd_chnl_t *)acapd_container_of(node, acapd_chnl_t,
							   node);
		if (chnl == lchnl) {
			acapd_list_del(&chnl->node);
			break;
		}
	}
	shm->refcount--;
	if (chnl->ops != NULL && chnl->ops->munmap != NULL) {
		chnl->ops->munmap(chnl, shm);
	}
	return 0;
}

void *acapd_accel_alloc_shm(acapd_accel_t *accel, size_t size, acapd_shm_t *shm)
{
	(void)accel;
	if (shm == NULL) {
		acapd_perror("%s: failed due to shm is NULL\n", __func__);
		return NULL;
	}
	memset(shm, 0, sizeof(*shm));
	return acapd_alloc_shm(NULL, shm, size, 0);
}

int acapd_accel_write_data(acapd_accel_t *accel, acapd_shm_t *shm)
{
	acapd_chnl_t *chnl = NULL;
	acapd_dim_t dim;
	int ret;
	int transfered_len;

	if (accel == NULL) {
		acapd_perror("%s: fafiled due to accel is NULL.\n", __func__);
		return -EINVAL;
	}
	if (accel->chnls == NULL) {
		acapd_perror("%s: accel doesn't have channels.\n", __func__);
		return -EINVAL;
	}
	/* Assuming only two channels, tx and rx in the list */
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_chnl_t *lchnl;

		lchnl = &accel->chnls[i];
		if (lchnl->dir == ACAPD_DMA_DEV_W) {
			chnl = lchnl;
			break;
		}
	}
	if (chnl == NULL) {
		acapd_perror("%s: no write channel is found.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops == NULL || chnl->ops->open == NULL) {
		acapd_perror("%s: No function defined to open channel.\n");
		return -EINVAL;
	}
	acapd_debug("%s: opening chnnl\n", __func__);
	ret = chnl->ops->open(chnl);
	if (ret < 0) {
		acapd_perror("%s: failed to open channel.\n", __func__);
		return -EINVAL;
	}
	dim.number_of_dims = 1;
	dim.strides[0] = 1;
	acapd_debug("%s: attaching shm to channel\n", __func__);
	ret = acapd_attach_shm(chnl, shm);
	if (ret != 0) {
		acapd_perror("%s: failed to attach tx shm\n", __func__);
		return -EINVAL;
	}
	/* Check if it is ok to transfer data */
	acapd_debug("%s: polling  channel status\n", __func__);
	ret = acapd_dma_poll(chnl, 1);
	if (ret < 0) {
		acapd_perror("%s: chnl is not ready\n", __func__);
		return -EINVAL;
	}
	acapd_debug("%s: configuring channel dma\n", __func__);
	ret = acapd_dma_config(chnl, shm, &dim, 0);
	if (ret < 0) {
		acapd_perror("%s: failed to config chnl\n",
			     __func__);
		return -EINVAL;
	}
	transfered_len = ret;
	acapd_debug("%s: starting channel dma\n", __func__);
	ret = acapd_dma_start(chnl, NULL);
	if (ret != 0) {
		acapd_perror("%s: failed to start chnl\n",
			     __func__);
		return -EINVAL;
	}
	return transfered_len;
}

int acapd_accel_read_data(acapd_accel_t *accel, acapd_shm_t *shm)
{
	acapd_chnl_t *chnl = NULL;
	acapd_dim_t dim;
	int ret;
	int transfered_len;

	if (accel == NULL) {
		acapd_perror("%s: fafiled due to accel is NULL.\n", __func__);
		return -EINVAL;
	}
	if (accel->chnls == NULL) {
		acapd_perror("%s: accel doesn't have channels.\n", __func__);
		return -EINVAL;
	}
	/* Assuming only two channels, tx and rx in the list */
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_chnl_t *lchnl;

		lchnl = &accel->chnls[i];
		if (lchnl->dir == ACAPD_DMA_DEV_R) {
			chnl = lchnl;
			break;
		}
	}
	if (chnl == NULL) {
		acapd_perror("%s: no write channel is found.\n", __func__);
		return -EINVAL;
	}
	if (chnl->ops == NULL || chnl->ops->open == NULL) {
		acapd_perror("%s: No function defined to open channel.\n");
		return -EINVAL;
	}
	ret = chnl->ops->open(chnl);
	if (ret < 0) {
		acapd_perror("%s: failed to open channel.\n", __func__);
		return -EINVAL;
	}
	dim.number_of_dims = 1;
	dim.strides[0] = 1;
	/* Attach memory to the channel */
	ret = acapd_attach_shm(chnl, shm);
	if (ret != 0) {
		acapd_perror("%s: failed to attach tx shm\n", __func__);
		return -EINVAL;
	}
	/* Check if it is ok to transfer data */
	ret = acapd_dma_poll(chnl, 1);
	if (ret < 0) {
		acapd_perror("%s: chnl is not ready\n", __func__);
		return -EINVAL;
	}
	/* Config channel to receive data */
	ret = acapd_dma_config(chnl, shm, &dim, 0);
	if (ret < 0) {
		acapd_perror("%s: failed to config chnl\n",
			     __func__);
		return -EINVAL;
	}
	transfered_len = ret;
	/* Start transferring */
	ret = acapd_dma_start(chnl, NULL);
	if (ret != 0) {
		acapd_perror("%s: failed to start chnl\n",
			     __func__);
		return -EINVAL;
	}
	/* Wait until data has been received */
	ret = acapd_dma_poll(chnl, 1);
	if (ret < 0) {
		acapd_perror("%s: chnl is not done successfully\n", __func__);
		return -EINVAL;
	}
	return transfered_len;
}
