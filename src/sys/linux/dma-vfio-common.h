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

#define ACAPD_VFIO_MAX_REGIONS	8U /**< max IO regions of a vfio device */

typedef struct acapd_vfio_io {
	void *addr;
	size_t size;
} acapd_vfio_io_t;

typedef struct acapd_vfio_chnl {
	acapd_chnl_t *chnl;
	int container;
	int group;
	int device;
	acapd_vfio_io ios[ACAPD_VFIO_MAX_REGIONS];
} acapd_vfio_chnl_t;

void *vfio_dma_mmap(void *buff_id, size_t start_off, size_t size, acapd_chnl_t *chnl);
void *vfio_dma_munmap(void *buff_id, size_t start_off, size_t size, acapd_chnl_t *chnl);
int vfio_open_channel(char *name, int iommu_group, acapd_chnl_conn_t conn_type, int chnl_id, acapd_dir_t dir, acapd_chnl_t *chnl);
