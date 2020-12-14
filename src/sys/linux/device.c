/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/print.h>
#include "acapd-vfio-common.h"
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>
#include "generic-device.h"
#include <libfpga.h>
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
	if (dev->driver == NULL) {
		dev->driver = "uio_pdrv_genirq";
	}
	if (strcmp(dev->driver, "uio_pdrv_genirq") == 0) {
		dev->ops = &acapd_linux_generic_dev_ops;
	} else if (strcmp(dev->driver, "vfio-platform") == 0) {
		dev->ops = &acapd_vfio_dev_ops;
	} else {
		acapd_perror("%s: no ops found for device %s.\n",
			     __func__, dev->dev_name);
		return -EINVAL;
	}
	return dev->ops->open(dev);
}

