/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/assert.h>
#include <acapd/device.h>
#include <acapd/print.h>
#include <acapd/shm.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int acapd_device_open(acapd_device_t *dev)
{
	acapd_assert(dev != NULL);
	return sys_device_open(dev);
}

int acapd_device_close(acapd_device_t *dev)
{
	acapd_assert(dev != NULL);
	if (dev->ops != NULL && dev->ops->close != NULL) {
		return dev->ops->close(dev);
	}
	return 0;
}

void *acapd_device_attach_shm(acapd_device_t *dev, acapd_shm_t *shm)
{
	acapd_list_t *node;
	void *va;

	acapd_assert(dev != NULL);
	acapd_assert(shm != NULL);
	acapd_list_for_each(&shm->refs, node) {
		acapd_device_t *tmpdev;

		tmpdev = (acapd_device_t *)acapd_container_of(node,
							      acapd_device_t,
							      node);
		if (dev == tmpdev) {
			/* TODO: in some cases, different device
			 * can map different the same memory differently.
			 */
			return shm->va;
		}
	}
	if (dev->ops != NULL && dev->ops->attach != NULL) {
		va = dev->ops->attach(dev, shm);
		if (va == NULL) {
			return NULL;
		}
		if (shm->va == NULL) {
			shm->va = va;
		}
	} else {
		va = shm->va;
	}
	acapd_list_add_tail(&shm->refs, &dev->node);
	shm->refcount++;
	return va;
}

int acapd_device_detach_shm(acapd_device_t *dev, acapd_shm_t *shm)
{
	acapd_list_t *node;

	acapd_assert(dev != NULL);
	acapd_assert(shm != NULL);
	acapd_list_for_each(&shm->refs, node) {
		acapd_device_t *tmpdev;

		tmpdev = (acapd_device_t *)acapd_container_of(node,
							      acapd_device_t,
							      node);
		if (tmpdev == dev) {
			if (dev->ops != NULL && dev->ops->detach != NULL) {
				int ret;

				ret = dev->ops->detach(dev, shm);
				if (ret < 0) {
					return ret;
				}
			}
			acapd_list_del(&dev->node);
			shm->refcount--;
			return 0;
		}
	}
	return -EINVAL;
}

void *acapd_device_get_reg_va(acapd_device_t *dev)
{
	acapd_assert(dev != NULL);
	if (dev->va == NULL) {
		acapd_perror("%s: %s is not opened.\n", __func__,
			     dev->dev_name);
	}
	return dev->va;
}

int acapd_device_get_id(acapd_device_t *dev)
{
	acapd_assert(dev != NULL);
	return dev->id;
}

int acapd_device_get(acapd_device_t *dev)
{
	int ret;

	acapd_assert(dev != NULL);
	if (dev->priv == NULL) {
		dev->refs = 0;
		ret = acapd_device_open(dev);
		if (ret < 0) {
			acapd_perror("%s, failed to open %s.\n", __func__,
				     dev->dev_name);
			return -EINVAL;
		}
	}
	dev->refs++;
	return 0;
}

int acapd_device_put(acapd_device_t *dev)
{
	acapd_assert(dev != NULL);
	if (dev->priv == NULL) {
		return 0;
	}
	if (dev->refs == 0) {
		return 0;
	}
	dev->refs--;
	if (dev->refs == 0) {
		/* Close device */
		/* TODO: shall we force to close device
		 * if device is not referenced ?
		 */
		return acapd_device_close(dev);
	}
	return 0;
}
