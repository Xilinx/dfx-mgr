/*
 * Copyright (c) 2020, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	linux/dma.h
 * @brief	Linux specific DMA definition.
 */

#ifndef _ACAPD_LINUX_DMA_H
#define _ACAPD_LINUX_DMA_H

extern acapd_dma_ops_t axidma_vfio_dma_ops;

int sys_acapd_create_dma_channel(char *name, int iommu_group,
				 acapd_chnl_conn_t conn_type, int chnl_id,
				 acapd_dir_t dir, acapd_chnl_t *chnl);
#endif /* _ACAPD_LINUX_DMA_H */
