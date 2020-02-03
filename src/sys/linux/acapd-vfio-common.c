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
#include <acapd/shm.h>
#include <errno.h>
#include "acapd-vfio-common.h"
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>
#include "generic-device.h"
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

static int acapd_vfio_bind(acapd_device_t *dev)
{
	return acapd_generic_device_bind(dev, "vfio-platform");
}

static int acapd_vfio_device_open(acapd_device_t *dev)
{
	struct vfio_device_info device_info = { .argsz = sizeof(device_info)};
	char group_path[64];
	uint32_t i;
	acapd_vfio_t *vdev;
	int ret;

	if (dev == NULL) {
		acapd_perror("%s: dev pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	if (dev->priv != NULL) {
		acapd_debug("%s: dev %s already opened.\n",
			    __func__, dev->dev_name);
		return 0;
	}
	/* Check if vfio device has been opened */
	vdev = (acapd_vfio_t *)malloc(sizeof(*vdev));
	if (vdev == NULL) {
		acapd_perror("%s: failed to allocate memory.\n", __func__);
		return -ENOMEM;
	}
	memset(vdev, 0, sizeof(*vdev));
	acapd_list_init(&vdev->mmaps);
	snprintf(group_path, 64, "/dev/vfio/%d", dev->iommu_group);
	acapd_debug("%s: %s, bind vfio driver.\n",
		    __func__, dev->dev_name);
	ret = acapd_vfio_bind(dev);
	if (ret < 0) {
		acapd_perror("%s: %s failed to bind driver.\n",
			     __func__, dev->dev_name);
		ret = -EINVAL;
		goto error;
	}
	acapd_debug("%s: %s, open container.\n",
		    __func__, dev->dev_name);
	vdev->container = open(VFIO_CONTAINER,O_RDWR);
	if (vdev->container < 0) {
		acapd_perror("%s: failed to open container.\n",
			     __func__);
		ret = -EINVAL;
		goto error;
	}
	acapd_debug("%s: open group.\n", __func__);
	vdev->group = open(group_path, O_RDWR);
	if (vdev->group < 0) {
		acapd_perror("%s:%s failed to open group.\n", __func__, dev->dev_name);
		ret = -EINVAL;
		goto error;
	}

	 /* Add the group to the container */
	acapd_debug("%s: add group to container.\n", __func__);
	ret = ioctl(vdev->group, VFIO_GROUP_SET_CONTAINER,
		    &vdev->container);
	if (ret < 0) {
		acapd_perror("%s: %s failed to group set container ioctl:%s.\n", 
			     __func__, dev->dev_name, strerror(errno));
		ret = -EINVAL;
		goto error;
	}

	/* Enable the IOMMU mode we want */
	acapd_debug("%s: enable IOMMU mode.\n", __func__);
	ret = ioctl(vdev->container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);
	if (ret <0) {
		ret = -EINVAL;
		goto error;
	}

	acapd_debug("%s: get vfio device.\n", __func__);
	ret = ioctl(vdev->group,
		    VFIO_GROUP_GET_DEVICE_FD, dev->dev_name);
	if (ret < 0) {
		acapd_perror("%s: Failed to get device %s, %s.\n",
			     __func__, dev->dev_name, strerror(ret));
		ret = -EINVAL;
		goto error;
	}
	vdev->device = ret;

	ret = ioctl(vdev->device, VFIO_DEVICE_GET_INFO, &device_info);
	if (ret < 0) {
		acapd_perror("%s: Failed to get %s device information:%s.\n",
			     __func__, dev->dev_name, strerror(ret));
		ret = -EINVAL;
		goto error;
	}
	for (i = 0; i < device_info.num_regions; i++){
		struct vfio_region_info reg = { .argsz = sizeof(reg)};

		reg.index = 0;
		ret = ioctl(vdev->device, VFIO_DEVICE_GET_REGION_INFO, &reg);
		if (ret < 0) {
			acapd_perror("%s: Failed to get io of %s, %s.\n",
				     __func__, dev->dev_name, strerror(ret));
			ret = -EINVAL;
			goto error;
		}
		if (reg.flags & VFIO_REGION_INFO_FLAG_MMAP) {
			dev->va = mmap(0, reg.size,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED,
				       vdev->device,
				       reg.offset);
			if (dev->va == MAP_FAILED) {
				acapd_perror("%s:%s Failed to mmap: 0x%llx.\n",
					     __func__, dev->dev_name,
					     reg.offset);
				dev->va = NULL;
				ret = -EINVAL;
				goto error;
			}
			dev->reg_size = reg.size;
		}
	}
	dev->priv = vdev;
	return 0;
error:
	free(vdev);
	return ret;
}

static int acapd_vfio_device_close(acapd_device_t *dev)
{
	acapd_vfio_t *vdev;

	if (dev == NULL) {
		acapd_perror("%s: dev is NULL.\n", __func__);
		return -EINVAL;
	}
	vdev = (acapd_vfio_t *)dev->priv;
	if (vdev == NULL) {
		return 0;
	}
	dev->priv = NULL;

	while (1) {
		acapd_list_t *node;
		struct vfio_iommu_type1_dma_unmap dma_unmap = {.argsz = sizeof(dma_unmap)};
		acapd_vfio_mmap_t *mmap;
		int ret;

		node = acapd_list_first(&vdev->mmaps);
		if (node == NULL) {
			break;
		}
		mmap = (acapd_vfio_mmap_t *)(acapd_container_of(node, acapd_vfio_mmap_t, node));
		dma_unmap.iova = mmap->da;
		dma_unmap.size = mmap->size;
		ret = ioctl(vdev->container, VFIO_IOMMU_UNMAP_DMA, dma_unmap);
		if (ret < 0) {
			acapd_perror("%s: failed to unmap vfio memory.\n");
		}
		acapd_list_del(&mmap->node);
		free(mmap);
	}
	free(vdev);
	return 0;
}

static void *acapd_vfio_device_attach(acapd_device_t *dev, acapd_shm_t *shm)
{
	acapd_vfio_t *vdev;
	struct vfio_iommu_type1_dma_map dma_map = {.argsz = sizeof(dma_map)};
	int ret;
	acapd_list_t *node;
	acapd_vfio_mmap_t *mmap;
	uint64_t da;
	size_t size;

	if (dev == NULL) {
		acapd_perror("%s: dev is NULL.\n", __func__);
		return NULL;
	}
	if (shm == NULL) {
		acapd_perror("%s: shm is NULL.\n", __func__);
		return NULL;
	}

	vdev = (acapd_vfio_t *)dev->priv;
	if (vdev == NULL) {
		acapd_perror("%s: %s vfio dev is NULL.\n", __func__,
			     dev->dev_name);
		return NULL;
	}
	size = acapd_align_up(shm->size, (4 * 1024));
	dma_map.size = size;
	dma_map.vaddr = (uint64_t)((uintptr_t)(shm->va));
	dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;
	/* Calculate da address */
	da = 0;
	acapd_list_for_each(&vdev->mmaps, node) {
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
	ret = ioctl(vdev->container, VFIO_IOMMU_MAP_DMA, &dma_map);
	if (ret) {
		acapd_perror("%s: Could not map DMA memory %d,%s. %p, 0x%llx, 0x%lx\n",
			     __func__, vdev->container, strerror(ret),
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
	acapd_list_add_tail(&vdev->mmaps, &mmap->node);
	acapd_debug("%s: mmap shm done, 0x%llx, 0x%llx\n", __func__, da, size);
	return shm->va;
}

static int acapd_vfio_device_detach(acapd_device_t *dev, acapd_shm_t *shm)
{
	acapd_vfio_t *vdev;
	struct vfio_iommu_type1_dma_unmap dma_unmap = {.argsz = sizeof(dma_unmap)};
	int ret;
	acapd_list_t *node;
	acapd_vfio_mmap_t *mmap;

	if (dev == NULL) {
		acapd_perror("%s: dev is NULL.\n", __func__);
		return -EINVAL;
	}
	if (shm == NULL) {
		acapd_perror("%s: shm is NULL.\n", __func__);
		return -EINVAL;
	}

	vdev = (acapd_vfio_t *)dev->priv;
	if (vdev == NULL) {
		acapd_perror("%s: %s vfio dev is NULL.\n", __func__,
			     dev->dev_name);
		return -EINVAL;
	}

	ret = -EINVAL;
	dma_unmap.size = shm->size;
	acapd_list_for_each(&vdev->mmaps, node) {
		mmap = (acapd_vfio_mmap_t *)(acapd_container_of(node, acapd_vfio_mmap_t, node));
		if (mmap->va == shm->va) {
			dma_unmap.iova = mmap->da;
			ret = ioctl(vdev->container, VFIO_IOMMU_UNMAP_DMA, dma_unmap);
			if (ret < 0) {
				acapd_perror("%s: %s failed ioctl unmap.\n",
					     __func__, dev->dev_name);
			}
			acapd_list_del(&mmap->node);
			free(mmap);
			break;
		}
	}
	return ret;
}

static uint64_t acapd_vfio_device_va_to_da(acapd_device_t *dev, void *va)
{
	acapd_vfio_t *vdev;
	acapd_list_t *node;
	acapd_vfio_mmap_t *mmap;
	char *lva = va;

	if (dev == NULL) {
		acapd_perror("%s: dev is NULL.\n", __func__);
		return -EINVAL;
	}
	vdev = (acapd_vfio_t *)dev->priv;
	if (vdev == NULL) {
		acapd_perror("%s: %s vfio device is NULL.\n", __func__,
			     dev->dev_name);
		return -EINVAL;
	}

	acapd_list_for_each(&vdev->mmaps, node) {
		mmap = (acapd_vfio_mmap_t *)(acapd_container_of(node, acapd_vfio_mmap_t, node));
		if (lva >= (char *)mmap->va && lva < ((char *)mmap->va + mmap->size)) {
			uint64_t offset;

			offset = lva - (char *)mmap->va;
			return mmap->da + offset;
		}
	}
	return (uint64_t)(-1);
}

acapd_device_ops_t acapd_vfio_dev_ops = {
	.open = acapd_vfio_device_open,
	.close = acapd_vfio_device_close,
	.attach = acapd_vfio_device_attach,
	.detach = acapd_vfio_device_detach,
	.va_to_da = acapd_vfio_device_va_to_da,
};

