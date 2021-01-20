/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/shm.h>
#include <errno.h>
#include "generic-device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int sys_device_open(acapd_device_t *dev)
{
	acapd_assert(dev != NULL);
	if (dev->ops != NULL && dev->ops->open != NULL) {
		return dev->ops->open(dev);
	}
	if (dev->ops != NULL) {
		return 0;
	}
	dev->ops = &sys_generic_dev_ops;
	return dev->ops->open(dev);
}

static int sys_generic_device_open(acapd_device_t *dev)
{
	if (dev == NULL) {
		acapd_perror("%s: dev pointer is NULL.\n", __func__);
		return -EINVAL;
	}
	if (dev->va == NULL) {
		acapd_perror("%s: dev va is NULL.\n", __func__);
		return -EINVAL;
	}
	dev->priv = dev->va;
	return 0;
}

static int sys_generic_device_close(acapd_device_t *dev)
{
	if (dev == NULL) {
		acapd_perror("%s: dev is NULL.\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static void *sys_generic_device_attach(acapd_device_t *dev, acapd_shm_t *shm)
{
	(void)dev;
	if (shm == NULL) {
		acapd_perror("%s: shm is NULL.\n", __func__);
		return NULL;
	}

	return shm->va;
}

static int sys_generic_device_detach(acapd_device_t *dev, acapd_shm_t *shm)
{
	(void)dev;
	(void)shm;
	return 0;
}

static uint64_t sys_generic_device_va_to_da(acapd_device_t *dev, void *va)
{
	(void)dev;
	return (uint64_t)((uintptr_t)va);
}

acapd_device_ops_t sys_generic_dev_ops = {
	.open = sys_generic_device_open,
	.close = sys_generic_device_close,
	.attach = sys_generic_device_attach,
	.detach = sys_generic_device_detach,
	.va_to_da = sys_generic_device_va_to_da,
};

