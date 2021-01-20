/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/dma.h>
#include <dfx-mgr/device.h>
#include <dfx-mgr/helper.h>
#include <dfx-mgr/shm.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int acapd_alloc_shm(char *shm_allocator_name, acapd_shm_t *shm,
		    size_t size, uint32_t attr)
{
	(void)shm_allocator_name;
	shm->refcount = 0;
	acapd_list_init(&shm->refs);
	return acapd_default_shm_allocator.alloc(&acapd_default_shm_allocator,
					         shm, size, attr);
}

int acapd_free_shm(acapd_shm_t *shm)
{
	acapd_list_t *node;

	/* Detach shared memory first */
	acapd_list_for_each(&shm->refs, node) {
		acapd_device_t *dev;

		dev = (acapd_device_t *)acapd_container_of(node, acapd_chnl_t,
							  node);
		if (dev->ops && dev->ops->detach) {
			dev->ops->detach(dev, shm);
		}
	}
	shm->refcount = 0;
	acapd_list_init(&shm->refs);
	acapd_default_shm_allocator.free(&acapd_default_shm_allocator, shm);
	return 0;
}

void *acapd_attach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm)
{
	if (chnl == NULL) {
		acapd_perror("%s: failed, chnl is NULL.\n", __func__);
		return NULL;
	}
	if (shm == NULL) {
		acapd_perror("%s: failed due to shm is NULL\n", __func__);
		return NULL;
	}
	return acapd_dma_attach(chnl, shm);
}

int acapd_detach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm)
{
	if (chnl == NULL) {
		acapd_perror("%s: failed, chnl is NULL.\n", __func__);
		return -EINVAL;
	}
	if (shm == NULL) {
		acapd_perror("%s: failed due to shm is NULL\n", __func__);
		return -EINVAL;
	}
	return acapd_dma_detach(chnl, shm);
}

void *acapd_accel_alloc_shm(acapd_accel_t *accel, size_t size, acapd_shm_t *shm)
{
	int ret;

	(void)accel;

	if (shm == NULL) {
		acapd_perror("%s: failed due to shm is NULL\n", __func__);
		return NULL;
	}
	/* We can not use memset to clear memory here as some variables of shm
	 * structure are required to be defined at this point for example FD.
	 */
	ret = acapd_alloc_shm(NULL, shm, size, 0);
	if (ret < 0) {
		acapd_perror("%s: failed to allocal memory.\n", __func__);
		return NULL;
	}
	/* TODO: if it is DMA buf, it will need to import the DMA buf
	 * to a device e.g. DMA device before it can get the va.
	 */
	if (shm->va == NULL) {
		acapd_perror("%s: va is NULL.\n", __func__);
		return NULL;
	}

	return shm->va;
}

int acapd_accel_write_data(acapd_accel_t *accel, acapd_shm_t *shm,
			   void *va, size_t size, int wait_for_complete, uint8_t tid)
{
	acapd_chnl_t *chnl = NULL;
	acapd_dma_config_t config;
	int ret;
	void *retva;
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
	acapd_debug("%s: opening chnnl\n", __func__);
	ret = acapd_dma_open(chnl);
	if (ret < 0) {
		acapd_perror("%s: failed to open channel.\n", __func__);
		return -EINVAL;
	}
	acapd_debug("%s: attaching shm to channel\n", __func__);
	retva = acapd_attach_shm(chnl, shm);
	if (retva == NULL) {
		acapd_perror("%s: failed to attach tx shm\n", __func__);
		return -EINVAL;
	}
	/* Check if it is ok to transfer data */
	acapd_debug("%s: polling channel status\n", __func__);
	ret = acapd_dma_poll(chnl, 0, NULL, 0);
	if (ret < 0) {
		acapd_perror("%s: chnl is not ready\n", __func__);
		return -EINVAL;
	} else if (ret == (int)ACAPD_CHNL_INPROGRESS) {
		return -EBUSY;
	}
	acapd_debug("%s: transfer data\n", __func__);
	acapd_dma_init_config(&config, shm, va, size, tid);
	ret = acapd_dma_transfer(chnl, &config);
	if (ret < 0) {
		acapd_perror("%s: failed to transfer data\n",
			     __func__);
		return -EINVAL;
	}
	transfered_len = ret;
	if (wait_for_complete != 0) {
		/* Wait until data has been sent*/
		acapd_debug("%s: wait for chnl to complete\n", __func__);
		ret = acapd_dma_poll(chnl, 1, NULL, 0);
		if (ret < 0) {
			acapd_perror("%s: chnl is not done successfully\n",
				     __func__);
			return -EINVAL;
		}
	}
	return transfered_len;
}

int acapd_accel_read_data(acapd_accel_t *accel, acapd_shm_t *shm,
			  void *va, size_t size, int wait_for_complete)
{
	acapd_chnl_t *chnl = NULL;
	acapd_dma_config_t config;
	int ret;
	int transfered_len = 0;
	void *retva;

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
	acapd_debug("%s: opening chnnl\n", __func__);
	ret = acapd_dma_open(chnl);
	if (ret < 0) {
		acapd_perror("%s: failed to open channel.\n", __func__);
		return -EINVAL;
	}
	/* Attach memory to the channel */
	acapd_debug("%s: attaching memory to chnnl\n", __func__);
	retva = acapd_attach_shm(chnl, shm);
	if (retva == NULL) {
		acapd_perror("%s: failed to attach rx shm\n", __func__);
		return -EINVAL;
	}
	/* Check if it is ok to transfer data */
	acapd_debug("%s: check if chnl is ready to receive data.\n", __func__);
	ret = acapd_dma_poll(chnl, 0, NULL, 0);
	if (ret < 0) {
		acapd_perror("%s: chnl is not ready\n", __func__);
		return -EINVAL;
	} else if (ret == (int)ACAPD_CHNL_INPROGRESS) {
		return -EBUSY;
	}
	/* Config channel to receive data */
	acapd_debug("%s: transfer data\n", __func__);
	acapd_dma_init_config(&config, shm, va, size, 0);
	ret = acapd_dma_transfer(chnl, &config);
	if (ret < 0) {
		acapd_perror("%s: failed to transfer data\n",
			     __func__);
		return -EINVAL;
	}
	transfered_len = ret;
	if (wait_for_complete != 0) {
		/* Wait until data has been received */
		acapd_debug("%s: wait for chnl to complete\n", __func__);
		ret = acapd_dma_poll(chnl, 1, NULL, 0);
		if (ret < 0) {
			acapd_perror("%s: chnl is not done successfully\n",
				     __func__);
			return -EINVAL;
		}
	}
	return transfered_len;
}

int acapd_accel_read_complete(acapd_accel_t *accel)
{
	acapd_chnl_t *chnl = NULL;
	int ret;
	int transfered_len = 0;
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
	acapd_debug("%s: opening chnnl\n", __func__);
	ret = acapd_dma_open(chnl);
	if (ret < 0) {
		acapd_perror("%s: failed to open channel.\n", __func__);
		return -EINVAL;
	}
	/* Wait until data has been received */
	acapd_debug("%s: wait for chnl to complete\n", __func__);
	ret = acapd_dma_poll(chnl, 1, NULL, 0);
	if (ret < 0) {
		acapd_perror("%s: chnl is not done successfully\n",
			     __func__);
		return -EINVAL;
	}
	acapd_debug("%s: channel is complete\n", __func__);
	return transfered_len;
}

