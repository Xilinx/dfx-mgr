/*
 * Copyright (c) 2020, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    dma-vfio-common.h
 */

#ifndef _VFIO_COMMON_H
#define _VFIO_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <acapd/dma.h>
#include <acapd/shm.h>
#include <acapd/helper.h>

#define ACAPD_VFIO_MAX_REGIONS	8U /**< max IO regions of a vfio device */

typedef struct acapd_vfio_io {
	void *va;
	size_t size;
} acapd_vfio_io_t;

typedef struct acapd_vfio_mmap {
	void *va;
	uint64_t da;
	size_t size;
	acapd_list_t node;
} acapd_vfio_mmap_t;

typedef struct acapd_vfio_chnl {
	int container; /**< vfio container fd */
	int group; /**< vfio group id */
	int device; /**< vfio device fd */
	const char *dev_name; /**< vfio device name */
	acapd_vfio_io_t ios[ACAPD_VFIO_MAX_REGIONS]; /**< io regions */
	acapd_list_t mmaps; /**< memory maps list */
	acapd_list_t node; /**< list node */
	int refs;
} acapd_vfio_chnl_t;

void *vfio_dma_mmap(acapd_chnl_t *chnl, acapd_shm_t *shm);
int vfio_dma_munmap(acapd_chnl_t *chnl, acapd_shm_t *shm);
int vfio_open_channel(acapd_chnl_t *chnl);
int vfio_close_channel(acapd_chnl_t *chnl);

#endif
