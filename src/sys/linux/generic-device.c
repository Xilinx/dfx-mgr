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
#include <fcntl.h>
#include <metal/device.h>
#include <metal/io.h>
#include <metal/sys.h>
#include <string.h>
#include <unistd.h>

static int libmetal_refs = 0;

static int acapd_generic_device_open(acapd_device_t *dev)
{
	int ret;
	struct metal_device *mdev;
	struct metal_io_region *io;

	acapd_assert(dev != NULL);
	if (dev->va != NULL && dev->priv != NULL) {
		/* Device is already open */
		return 0;
	}

	if (strlen(dev->path) != 0 && access(dev->path, F_OK) == 0) {
		int fd;

		fd = open(dev->path, O_RDWR);
		if (fd < 0) {
			acapd_perror("%s: failed to open %s:%s:%s.\n",
				     __func__, dev->dev_name, dev->path,
				     strerror(errno));
			return -EINVAL;
		}
	}
	if (dev->dev_name == NULL) {
		acapd_perror("%s: no device name is specified.\n");
		return -EINVAL;
	}
	if (libmetal_refs == 0) {
		struct metal_init_params init_param = METAL_INIT_DEFAULTS;

		ret = metal_init(&init_param);
		if (ret < 0) {
			acapd_perror("%s: failed to initialize libmetal.\n",
				     __func__);
			return ret;
		}
	}
	libmetal_refs++;
	if (dev->bus_name == NULL) {
		dev->bus_name = "platform";
	}

	ret = metal_device_open(dev->bus_name, dev->dev_name, &mdev);
	if (ret < 0) {
		acapd_perror("%s: failed to open %s,%s.\n",
			     __func__, dev->bus_name, dev->dev_name);
		return ret;
	}

	io = metal_device_io_region(mdev, 0);
	if (!io) {
		acapd_perror("%s: failed to get io for %s.\n",
			     __func__, dev->dev_name);
		ret = -ENODEV;
		metal_device_close(mdev);
		return ret;
	}
	dev->va = metal_io_virt(io, 0);
	dev->reg_size = metal_io_region_size(io);
	dev->priv = mdev;
	return 0;
}

static int acapd_generic_device_close(acapd_device_t *dev)
{
	struct metal_device *mdev;

	acapd_assert(dev != NULL);
	if (dev->va == NULL || dev->priv == NULL) {
		acapd_debug("%s: device %s is not open.\n",
			    __func__, dev->dev_name);
		return 0;
	}
	libmetal_refs--;

	mdev = (struct metal_device *)dev->priv;

	metal_device_close(mdev);
	dev->va = NULL;
	dev->reg_size = 0;
	dev->priv = NULL;

	if (libmetal_refs == 0) {
		metal_finish();
	}
	return 0;
}


acapd_device_ops_t acapd_linux_generic_dev_ops = {
	.open = acapd_generic_device_open,
	.close = acapd_generic_device_close,
	.attach = NULL,
	.detach = NULL,
	.va_to_da = NULL,
};

