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

void *vfio_dma_mmap(void *buff_id, size_t start_off, size_t size, acapd_c    hnl_t *chnl);
void *vfio_dma_munmap(void *buff_id, size_t start_off, size_t size, acapd_c    hnl_t *chnl);

