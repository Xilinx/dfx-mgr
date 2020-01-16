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

#define VFIO_CONTAINER  "/dev/vfio/vfio"

void *vfio_dma_mmap(void *vaddr, size_t start_off, size_t size, acapd_chnl_t *chnl){
	int fd;
	struct acapd_vfio_chnl *vchnl_info;
	struct vfio_iommu_type1_dma_map dma_map = { .argsz = sizeof(dma_map)     };
	int ret;

	if (chnl == NULL) {
		return NULL;
	}
	dma_map.size = size;
	dma_map.iova = start_off;
	dma_map.vaddr = vaddr;
	dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;
	
	vchnl_info = (struct acapd_vfio_chnl *)chnl->sys_info;
	ret = ioctl(vchnl_info->container, VFIO_IOMMU_MAP_DMA, &dma_map);
	if(ret) {
		acapd_perror("Could not map DMA memory for vfio\n");
		return NULL;
	}

}
void *vfio_dma_munmap(void *buff_id, size_t start_off, size_t size, acapd_chnl_t *chnl){

}

int vfio_open_channel(char *name, int iommu_group, acapd_chnl_conn_t conn_type, int chnl_id, acapd_dir_t dir, acapd_chnl_t *chnl){
	
	struct acapd_vfio_chnl *vchnl_info;
	struct vfio_device_info device_info = { .argsz = sizeof(device_info) };
	char group_path[64];
	int i;

	vchnl_info = malloc(sizeof(*vchnl_info));
	snprintf(group_path, 64, "/dev/vfio/%d", iommu_group);

	if (vchnl_info == NULL) {
		return -EINVAL;
	}
	chnl->sys_info = vchnl_info;
	vchnl_info->container = open(VFIO_CONTAINER,O_RDWR);
	vchnl_info->group = open(group_path,O_RDWR);
	
	 /* Add the group to the container */
	ioctl(vchnl_info->group, VFIO_GROUP_SET_CONTAINER, &vchnl_info->container);

	/* Enable the IOMMU mode we want */
	ioctl(vchnl_info->container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);

	vchnl_info->device = ioctl(vchnl_info->group, VFIO_GROUP_GET_DEVICE_FD, AXIDMA_DEVICE);
	if (vchnl_info->device < 0) {
		acpad_perror("Failed to get vfio axidma device\n");
	}
	
	ioctl(vchnl_info->device, VFIO_DEVICE_GET_INFO, &device_info);
	for (i=0; i < device_info.num_regions; i++){
		struct vfio_region_info_reg = { .argsz = sizeof(reg)};
		reg.index = 1;
		ioctl(vchnl_info->device, VFIO_DEVICE_GET_REGION_INFO, &reg);
		if (reg.flags & VFIO_REGION_INFO_FLAG_MMAP) {
			vchnl_info->ios[i].addr = mmap(0, reg.size,
							PROT_READ | PROT_WRITE,
							MAP_SHARED, device,
							reg.offset);
			if  (!mmap_regions[i].addr) {
				fprintf(stderr, "Failed to mmap: 0x%llx.\n",
					reg.offset);
				return = -EINVAL;
			}
			vchnl_info->ios[i].size = reg.size;
			void *data = vchnl_info->ios[i].addr;
			*((unsigned int *)data) = 0xdeadbeef;
		}
	}	
}
