/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/dma.h>
#include <acapd/assert.h>
#include <acapd/print.h>
#include <acapd/shm.h>
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
#include "dma-vfio-common.h"

/**
 * TODO
 */
static int axidma_vfio_dma_config(acapd_chnl_t *chnl, acapd_shm_t *shm,
				  acapd_shape_t *stride, uint32_t auto_repeat)
{
#if 0
	acapd_vfio_chnl_t *vchnl_info;
	void *base_va; /**< AXI DMA reg mmaped base va address */

	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	base_va = vchnl_info->ios[0].va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		/* write to stream, setup tx DMA */

	} else {
		/* read from stream, setup rx DMA */
	}
#else
	(void)chnl;
	(void)shm;
	(void)stride;
	(void)auto_repeat;

#endif
	return 0;
}

static int axidma_vfio_dma_start(acapd_chnl_t *chnl, acapd_fence_t *fence)
{
#if 0
	acapd_vfio_chnl_t *vchnl_info;
	void *base_va; /**< AXI DMA reg mmaped base va address */

	(void)fence;
	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	base_va = vchnl_info->ios[0].va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		/* write to stream, start tx DMA */

	} else {
		/* read from stream, start rx DMA */
	}
#else
	(void)chnl;
	(void)fence;
#endif
	return 0;
}

static int axidma_vfio_dma_stop(acapd_chnl_t *chnl)
{
#if 0
	acapd_vfio_chnl_t *vchnl_info;
	void *base_va; /**< AXI DMA reg mmaped base va address */

	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	base_va = vchnl_info->ios[0].va;

	if (chnl->dir == ACAPD_DMA_DEV_W) {
		/* write to stream, stop tx DMA */

	} else {
		/* read from stream, stop rx DMA */
	}
#else
	(void)chnl;
#endif
	return 0;
}

static int axidma_vfio_dma_poll(acapd_chnl_t *chnl, uint32_t wait_for_complete)
{
#if 0
	acapd_vfio_chnl_t *vchnl_info;
	void *base_va; /**< AXI DMA reg mmaped base va address */
	uint32_t is_complete;

	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	base_va = vchnl_info->ios[0].va;

	is_complete = 0;
	do {
		if (chnl->dir == ACAPD_DMA_DEV_W) {
			/* write to stream, read tx DMA status */
		} else {
			/* read from stream, read rx DMA status */
		}
	} while((!is_complete) && wait_for_complete);
	return is_complete;
#else
	(void)chnl;
	(void)wait_for_complete;
	return 0;
#endif
}

static int axidma_vfio_dma_open(acapd_chnl_t *chnl)
{
	int ret;
	//int need_reset;
	struct stat s;
	char tmpstr[256];

	/* unbind the driver if required */
	sprintf(tmpstr, "/sys/bus/platform/drivers/xilinx-vdma/%s",
		chnl->dev_name);
	ret = stat(tmpstr, &s);
	if(ret >= 0) {
		/* Need to unbind the driver */
		sprintf(tmpstr,
			"echo %s > /sys/bus/platform/drivers/xilinx-vdma/unbind",
			chnl->dev_name);
		system(tmpstr);
	}
	sprintf(tmpstr, "/sys/bus/platform/drivers/vfio-platform/%s",
		chnl->dev_name);
	ret = stat(tmpstr, &s);
	if(ret < 0) {
		/* Need to bind the driver with vfio  */
		sprintf(tmpstr,
			"echo vfio-platform > /sys/bus/platform/devices/%s/driver_override",
			chnl->dev_name);
		system(tmpstr);
		sprintf(tmpstr,
			"echo %s > /sys/bus/platform/drivers_probe",
			chnl->dev_name);
		system(tmpstr);
	}
	ret = vfio_open_channel(chnl);
	if (ret < 0) {
		acapd_perror("%s: failed to open vfio channel.\n", __func__);
		return -EINVAL;
	}
#if 0
	if (chnl->refs == 1) {
		/* First time to open channel, reset it */
	}
#endif
	return 0;

}

acapd_dma_ops_t axidma_vfio_dma_ops = {
	.name = "axidma_vfio_dma",
	.mmap = vfio_dma_mmap,
	.munmap = vfio_dma_munmap,
	.config = axidma_vfio_dma_config,
	.start = axidma_vfio_dma_start,
	.stop = axidma_vfio_dma_stop,
	.poll = axidma_vfio_dma_poll,
	.open = axidma_vfio_dma_open,
	.close = vfio_close_channel,
};
