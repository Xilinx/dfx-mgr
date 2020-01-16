/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/helper.h>
#include <acapd/print.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>
#include <libfpga.h>
#include <linux/vfio.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

struct acapd_vfio_chnl {
	acapd_chnl_t *chnl;
	int container;
	int group;
};

void *vfio_dma_mmap(void *vaddr, size_t start_off, size_t size, acapd_chnl_t *chnl){
	int fd;
	struct acapd_vfio_chnl *vfio_chnl;
	struct vfio_iommu_type1_dma_map dma_map = { .argsz = sizeof(dma_map)     };
	int ret;

	if (chnl == NULL) {
		return NULL;
	}
	dma_map.size = size;
	dma_map.iova = start_off;
	dma_map.vaddr = vaddr;
	dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;
	
	vfio_chnl = acapd_container_of(chnl, struct acapd_vfio_chnl, chnl);
	ret = ioctl(vfio_chnl->container, VFIO_IOMMU_MAP_DMA, &dma_map);
	if(ret) {
		printf("Could not map DMA memory for vfio\n");
		return NULL;
	}

}
void *vfio_dma_munmap(void *buff_id, size_t start_off, size_t size, acapd_chnl_t *chnl){

}
