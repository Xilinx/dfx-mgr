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

/**
 * TODO
 */
static int axidma_vfio_dma_config(acapd_chnl_t *chnl, acapd_shm_t *shm,
				  acapd_dim_t *dim, uint32_t auto_repeat)
{
	(void)dim;
	(void)buff_id;
	(void)va;
	(void)size;
	(void)chnl;
	return 0;
}

static int axidma_vfio_dma_start(acapd_chnl_t *chnl, acapd_fence_t *fence)
{
	(void)chnl;
	(void)fence;
	return 0;
}

static int axidma_vfio_dma_stop(acapd_chnl_t *chnl)
{
	(void)chnl;
	return 0;
}

static int axidma_vfio_dma_poll(acapd_chnl_t *chnl, uint32_t wait_for_complete)
{
	(void)chnl;
	(void)wait_for_complete;
	return 0;
}

static int axidma_vfio_dma_close_chnl(acapd_chnl_t *chnl)
{
	(void)chnl;
	return 0;
}

acapd_dma_ops_t axidma_vfio_dma_ops = {
	.name = "axidma_vfio_dma";
	.mmap = vfio_dma_mmap;
	.munmap = vfio_dma_munmap;
	.config = axidma_vfio_dma_config;
	.start = axidma_vfio_dma_start;
	.stop = axidma_vfio_dma_stop;
	.poll = axidma_vfio_dma_poll;
	.open_chnl = NULL;
	.close_chnl = axidma_vfio_dma_close_chnl;
};
