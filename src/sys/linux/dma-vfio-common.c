/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/dma.h>
#include <acapd/helper.h>
#include <acapd/print.h>
#include <errno.h>
#include "dma-vfio-common.h"
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

static ACAPD_DECLARE_LIST(vfio_devs);

void *vfio_dma_mmap(acapd_chnl_t *chnl, acapd_shm_t *shm)
{
	acapd_vfio_chnl_t *vchnl_info;
	struct vfio_iommu_type1_dma_map dma_map = {.argsz = sizeof(dma_map)     };
	int ret;
	acapd_list_t *node;
	acapd_vfio_mmap_t *mmap;
	uint64_t da;
	size_t size;

	if (chnl == NULL) {
		acapd_perror("%s: chnl is NULL.\n", __func__);
		return NULL;
	}
	if (shm == NULL) {
		acapd_perror("%s: shm is NULL.\n", __func__);
		return NULL;
	}

	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	if (vchnl_info == NULL) {
		acapd_perror("%s: vfio chnl info is NULL.\n", __func__);
		return NULL;
	}
	size = acapd_align_up(shm->size, (4 * 1024));
	dma_map.size = size;
	dma_map.vaddr = (uint64_t)((uintptr_t)(shm->va));
	dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;
	/* Calculate da address */
	da = 0;
	acapd_list_for_each(&vchnl_info->mmaps, node) {
		uint64_t tmp;

		mmap = (acapd_vfio_mmap_t *)(acapd_container_of(node, acapd_vfio_mmap_t, node));
		tmp = mmap->da + mmap->size;
		if ((da + size) < mmap->da) {
			continue;
		}
		if ((da + size) <= tmp) {
			da = tmp;
		}
	}
	dma_map.iova = da;
	ret = ioctl(vchnl_info->container, VFIO_IOMMU_MAP_DMA, &dma_map);
	if (ret) {
		acapd_perror("%s: Could not map DMA memory %d,%s. %p, 0x%llx, 0x%lx\n",
			     __func__, vchnl_info->container, strerror(ret),
			     shm->va, da, size);
		return NULL;
	}
	mmap = malloc(sizeof(*mmap));
	if (mmap == NULL) {
		acapd_perror("%s: failed to alloc memory for mmap struct.\n",
			     __func__);
		return NULL;
	}
	mmap->va = shm->va;
	mmap->da = da;
	mmap->size = size;
	acapd_list_add_tail(&vchnl_info->mmaps, &mmap->node);
	acapd_debug("%s: mmap shm done, 0x%llx, 0x%llx\n", __func__, da, size);
	return shm->va;
}

int vfio_dma_munmap(acapd_chnl_t *chnl, acapd_shm_t *shm)
{
	acapd_vfio_chnl_t *vchnl_info;
	struct vfio_iommu_type1_dma_unmap dma_unmap = {.argsz = sizeof(dma_unmap)};
	int ret;
	acapd_list_t *node;
	acapd_vfio_mmap_t *mmap;

	if (chnl == NULL) {
		acapd_perror("%s: chnl is NULL.\n", __func__);
		return -EINVAL;
	}
	if (shm == NULL) {
		acapd_perror("%s: shm is NULL.\n", __func__);
		return -EINVAL;
	}

	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	if (vchnl_info == NULL) {
		acapd_perror("%s: vfio chnl info is NULL.\n", __func__);
		return -EINVAL;
	}

	ret = -EINVAL;
	dma_unmap.size = shm->size;
	acapd_list_for_each(&vchnl_info->mmaps, node) {
		mmap = (acapd_vfio_mmap_t *)(acapd_container_of(node, acapd_vfio_mmap_t, node));
		if (mmap->va == shm->va) {
			dma_unmap.iova = mmap->da;
			ret = ioctl(vchnl_info->container, VFIO_IOMMU_UNMAP_DMA, dma_unmap);
			if (ret < 0) {
				acapd_perror("%s: failed to unmap vfio memory.\n");
			} else {
				acapd_list_del(&mmap->node);
				free(mmap);
			}
			break;
		}
	}
	return ret;
}

int vfio_open_channel(acapd_chnl_t *chnl)
{
	acapd_vfio_chnl_t *vchnl_info;
	struct vfio_device_info device_info = { .argsz = sizeof(device_info) };
	char group_path[64];
	uint32_t i;
	acapd_list_t *node;
	int ret;

	/* Check if vfio device has been opened */
	vchnl_info = NULL;
	acapd_debug("%s: check if device has already opened.\n",  __func__);
	acapd_list_for_each(&vfio_devs, node) {
		acapd_vfio_chnl_t *lvchnl_info;

		lvchnl_info = (acapd_vfio_chnl_t *)acapd_container_of(node,
					acapd_vfio_chnl_t, node);
		acapd_debug("%s: checking %s.\n", __func__,
			    lvchnl_info->dev_name);
		if (strcmp(chnl->dev_name, lvchnl_info->dev_name) == 0) {
			vchnl_info = lvchnl_info;
			break;
		}
	}

	/* Device is already opened */
	if (vchnl_info != NULL) {
		acapd_debug("%s: device is already opened.\n", __func__);
		vchnl_info->refs++;
		chnl->sys_info = vchnl_info;
		return 0;
	}
	vchnl_info = malloc(sizeof(*vchnl_info));
	if (vchnl_info == NULL) {
		acapd_perror("%s: failed to alloc memory.\n", __func__);
		return -EINVAL;
	}
	memset(vchnl_info, 0, sizeof(*vchnl_info));
	vchnl_info->dev_name = chnl->dev_name;
	snprintf(group_path, 64, "/dev/vfio/%d", chnl->iommu_group);

	acapd_list_init(&vchnl_info->mmaps);
	chnl->sys_info = vchnl_info;
	acapd_debug("%s: open container.\n", __func__);
	vchnl_info->container = open(VFIO_CONTAINER,O_RDWR);
	if (vchnl_info->container < 0) {
		acapd_perror("%s: failed to open container.\n", __func__);
		return -EINVAL;
	}
	acapd_debug("%s: open group.\n", __func__);
	vchnl_info->group = open(group_path,O_RDWR);
	if (vchnl_info->group < 0) {
		acapd_perror("%s: failed to open group.\n", __func__);
		return -EINVAL;
	}

	 /* Add the group to the container */
	acapd_debug("%s: add group to container.\n", __func__);
	ret = ioctl(vchnl_info->group, VFIO_GROUP_SET_CONTAINER,
		    &vchnl_info->container);
	if (ret < 0) {
		acapd_perror("%s: failed to group set container ioctl.\n", __func__);
		return -EINVAL;
	}

	/* Enable the IOMMU mode we want */
	acapd_debug("%s: enable IOMMU mode.\n", __func__);
	ret = ioctl(vchnl_info->container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);
	if (ret < 0) {
		acapd_perror("%s: failed to set iommu.\n");
		return -EINVAL;
	}

	acapd_debug("%s: get vfio device.\n", __func__);
	vchnl_info->device = ioctl(vchnl_info->group,
				   VFIO_GROUP_GET_DEVICE_FD,
				   chnl->dev_name);
	if (vchnl_info->device < 0) {
		acapd_perror("%s: Failed to get vfio device %s.\n",
			     __func__, chnl->dev_name);
		return -EINVAL;
	}

	memset(vchnl_info->ios, 0, sizeof(vchnl_info->ios));
	ioctl(vchnl_info->device, VFIO_DEVICE_GET_INFO, &device_info);
	for (i = 0; i < device_info.num_regions; i++){
		struct vfio_region_info reg = { .argsz = sizeof(reg)};

		reg.index = i;
		ioctl(vchnl_info->device, VFIO_DEVICE_GET_REGION_INFO, &reg);
		if (reg.flags & VFIO_REGION_INFO_FLAG_MMAP) {
			vchnl_info->ios[i].va = mmap(0, reg.size,
						PROT_READ | PROT_WRITE,
						MAP_SHARED,
						vchnl_info->device,
						reg.offset);
			if  (!vchnl_info->ios[i].va) {
				fprintf(stderr, "Failed to mmap: 0x%llx.\n",
					reg.offset);
				return -EINVAL;
			}
			vchnl_info->ios[i].size = reg.size;
		}
	}
	vchnl_info->refs++;
	acapd_list_add_tail(&vfio_devs, &vchnl_info->node);
	return 0;
}

int vfio_close_channel(acapd_chnl_t *chnl)
{
	acapd_vfio_chnl_t *vchnl_info;

	if (chnl == NULL) {
		acapd_perror("%s: chnl is NULL.\n", __func__);
		return -EINVAL;
	}
	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	if (vchnl_info == NULL) {
		return 0;
	}
	chnl->sys_info = NULL;
	vchnl_info->refs--;
	if (vchnl_info->refs != 0) {
		return 0;
	}
	/* TODO: Close VFIO device */
	/* Remove the vchnl_info from vchnls list */
	acapd_list_del(&vchnl_info->node);
	free(vchnl_info);
	return 0;
}

uint64_t vfio_va_to_da(acapd_chnl_t *chnl, void *va)
{
	acapd_vfio_chnl_t *vchnl_info;
	acapd_list_t *node;
	acapd_vfio_mmap_t *mmap;
	char *lva = va;

	if (chnl == NULL) {
		acapd_perror("%s: chnl is NULL.\n", __func__);
		return -EINVAL;
	}
	vchnl_info = (acapd_vfio_chnl_t *)chnl->sys_info;
	if (vchnl_info == NULL) {
		acapd_perror("%s: vfio chnl info is NULL.\n", __func__);
		return -EINVAL;
	}

	acapd_list_for_each(&vchnl_info->mmaps, node) {
		mmap = (acapd_vfio_mmap_t *)(acapd_container_of(node, acapd_vfio_mmap_t, node));
		if (lva >= (char *)mmap->va && lva < ((char *)mmap->va + mmap->size)) {
			uint64_t offset;

			offset = lva - (char *)mmap->va;
			return mmap->da + offset;
		}
	}
	return (uint64_t)(-1);
}
