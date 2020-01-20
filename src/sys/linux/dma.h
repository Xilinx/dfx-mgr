/*
 * Copyright (c) 2020, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	linux/dma.h
 * @brief	Linux specific DMA definition.
 */

#include <acapd/assert.h>
#include <errno.h>
#include <string.h>

#ifndef _ACAPD_LINUX_DMA_H
#define _ACAPD_LINUX_DMA_H

int sys_acapd_create_dma_channel(char *name, int iommu_group,
				 acapd_chnl_conn_t conn_type, int chnl_id,
				 acapd_dir_t dir, acapd_chnl_t *chnl);
#endif /* _ACAPD_LINUX_DMA_H */
