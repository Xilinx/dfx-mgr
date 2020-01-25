/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
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
#include <libfpga.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
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
		dev->driver = "vfio-platform";
	}
	if (strcmp(dev->driver, "vfio-platform") == 0) {
		int ret;
		char tmpstr[256];
		struct stat s;

		/* TODO: There should be a better way to see if the device
		 * has driver bounded */
		if (strstr(dev->dev_name, ".dma") != NULL) {

			sprintf(tmpstr,
				"/sys/bus/platform/drivers/xilinx-vdma/%s",
				dev->dev_name);
			ret = stat(tmpstr, &s);
			if(ret >= 0) {
				/* Need to unbind the driver */
				sprintf(tmpstr,
					"echo %s > /sys/bus/platform/drivers/xilinx-vdma/unbind",
					dev->dev_name);
				system(tmpstr);
			}
		}
		/* bound driver with VFIO
		 * TODO: Should move to vfio device operation.
		 */
		sprintf(tmpstr,
			"/sys/bus/platform/drivers/vfio-platform/%s",
			dev->dev_name);
		ret = stat(tmpstr, &s);
		if(ret < 0) {
			/* Need to bind the driver with vfio  */
			sprintf(tmpstr,
				"echo vfio-platform > /sys/bus/platform/devices/%s/driver_override",
				dev->dev_name);
			system(tmpstr);
			sprintf(tmpstr,
				"echo %s > /sys/bus/platform/drivers_probe",
				dev->dev_name);
			system(tmpstr);
		}
		dev->ops = &acapd_vfio_dev_ops;
		return dev->ops->open(dev);
	}
	acapd_perror("%s: no ops found for device %s.\n", __func__, dev->dev_name);
	return -EINVAL;
}

