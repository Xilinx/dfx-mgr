/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "dma.h"

int sys_acapd_create_dma_channel(char *name, int iommu_group,
				 acapd_chnl_conn_t conn_type, int chnl_id,
				 acapd_dir_t dir, acapd_chnl_t *chnl)
{
	int ret;

	if ((conn_type & ACAPD_CHNL_VADDR) != 0) {
		ret = vfio_open_channel(name, iommu_group, conn_type, chnl_id,
					dir, chnl);
		if (ret != 0) {
			acapd_perror("%s: failed to open vfio channel %s\n",
				     __func__, name);
			return -EINVAL;
		}
	}
}
