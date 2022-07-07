/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/dma.h>
#include <dfx-mgr/helper.h>
#include <dfx-mgr/print.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int acapd_generic_device_bind(acapd_device_t *dev, const char *drv)
{
	char tmpstr[384];
	DIR *d;
	struct dirent *dir;
	int fd;
	int ret;

	acapd_assert(dev != NULL);
	acapd_assert(drv != NULL);
	DFX_DBG("%s, %s", dev->dev_name, drv);
	if (dev->bus_name == NULL) {
		dev->bus_name = "platform";
	}
	sprintf(tmpstr, "/sys/bus/%s/drivers", dev->bus_name);
	d = opendir(tmpstr);
	if (d == NULL) {
		ret = -errno;
		DFX_ERR("opendir(%s)", tmpstr);
		return ret;
	}
	while((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_DIR && strlen(dir->d_name) <= 64) {
			sprintf(tmpstr, "/sys/bus/%s/drivers/%s/%s",
				dev->bus_name, dir->d_name, dev->dev_name);
			if (access(tmpstr, F_OK) == 0) {
				if (strcmp(dir->d_name, drv) == 0) {
					DFX_DBG("dev %s already bound",
						dev->dev_name);
					ret = 0;
					goto out;
				}
				/* Unbind driver */
				sprintf(tmpstr, "/sys/bus/%s/drivers/%s/unbind",
					dev->bus_name, dir->d_name);
				fd = open(tmpstr, O_WRONLY);
				if (fd < 0) {
					ret = -errno;
					DFX_ERR("open(%s)", tmpstr);
					goto out;
				}
				ret = write(fd, dev->dev_name, strlen(dev->dev_name) + 1);
				if (ret < 0) {
					DFX_ERR("write %s to %s",
						dev->dev_name, tmpstr);
					close(fd);
					goto out;
				}
				close(fd);
			}
		}
	}
	/* Bind new driver */
	sprintf(tmpstr, "/sys/bus/%s/devices/%s/driver_override",
		dev->bus_name, dev->dev_name);
	fd = open(tmpstr, O_WRONLY);
	if (fd < 0) {
		ret = -errno;
		DFX_ERR("open(%s)", tmpstr);
		goto out;
	}
	ret = write(fd, drv, strlen(drv) + 1);
	if (ret < 0) {
		DFX_ERR("write %s to %s", drv, tmpstr);
		close(fd);
		goto out;
	}
	sprintf(tmpstr, "/sys/bus/%s/drivers/%s/bind",
		dev->bus_name, drv);
	fd = open(tmpstr, O_WRONLY);
	if (fd < 0) {
		ret = -errno;
		DFX_ERR("open(%s)", tmpstr);
		goto out;
	}
	ret = write(fd, dev->dev_name, strlen(dev->dev_name) + 1);
	if (ret < 0) {
		DFX_ERR("write %s to %s", dev->dev_name, tmpstr);
		close(fd);
		goto out;
	}
	close(fd);
	ret = 0;
out:
	closedir(d);
	return ret;
}

static int acapd_generic_device_get_uio_path(acapd_device_t *dev)
{
	int ret;
	char tmpstr[384];
	DIR *d;
	struct dirent *dir;

	acapd_assert(dev != NULL);
	acapd_assert(dev->dev_name != NULL);

	sprintf(tmpstr, "/sys/bus/%s/devices/%s/uio",
		dev->bus_name, dev->dev_name);
	DFX_DBG("%s", tmpstr);
	d = opendir(tmpstr);
	if (d == NULL) {
		ret = -errno;
		DFX_ERR("opendir(%s)", tmpstr);
		return ret;
	}
	ret = -EINVAL;
	while ((dir = readdir(d)) != NULL) {
		if(dir->d_type == DT_DIR && !strncmp(dir->d_name, "uio", 3)) {
			int fd;
			char size_str[32], tmpname[8];

			memset(dev->path, 0, sizeof(dev->path));
			memset(tmpname, 0, sizeof(tmpname));
			strcpy(tmpname, dir->d_name);
			sprintf(dev->path, "/dev/%s", tmpname);

			/* get io region size */
			sprintf(tmpstr, "/sys/bus/%s/devices/%s/uio/%s/maps/map0/size",
				dev->bus_name, dev->dev_name, dir->d_name);
			fd = open(tmpstr, O_RDONLY);
			if (fd < 0) {
				ret = -errno;
				DFX_ERR("open(%s)", tmpstr);
				goto out;
			}
			memset(size_str, 0, sizeof(size_str));
			ret = read(fd, size_str, sizeof(size_str));
			if (ret < 0) {
				DFX_ERR("read(%s)", tmpstr);
				close(fd);
				goto out;
			}
			dev->reg_size = strtoull(size_str, NULL, 16);
			DFX_DBG("%s = %#lx", tmpstr, dev->reg_size);

			close(fd);
			ret = 0;
			goto out;
		}
	}
out:
	closedir(d);
	return ret;
}

static int acapd_generic_device_open(acapd_device_t *dev)
{
	int ret;

	acapd_assert(dev != NULL);
	DFX_DBG("%p %s", dev, dev->dev_name);
	if (dev->va != NULL && dev->priv != NULL) {
		/* Device is already open */
		return 0;
	}

	if (dev->bus_name == NULL) {
		dev->bus_name = "platform";
	}
	if (strlen(dev->path) == 0 || access(dev->path, F_OK) != 0) {
		if (dev->dev_name != NULL) {
			ret = acapd_generic_device_bind(dev, "uio_pdrv_genirq");
			if (ret < 0) {
				DFX_ERR("device_bind %s", dev->dev_name);
				return -EINVAL;
			}
			ret = acapd_generic_device_get_uio_path(dev);
			if (ret < 0) {
				DFX_ERR("get_uio_path %s", dev->dev_name);
				return -EINVAL;
			}
		} else {
			DFX_ERR("no dev name or path is specified");
			return -EINVAL;
		}
	}

	if (strlen(dev->path) != 0 && access(dev->path, F_OK) == 0) {
		int fd;

		fd = open(dev->path, O_RDWR);
		if (fd < 0) {
			DFX_ERR("open(%s) for %s", dev->path, dev->dev_name);
			return -EINVAL;
		}
		if (dev->reg_size == 0) {
			DFX_ERR("failed to get %s,%s size; reg_siz is 0",
				dev->dev_name, dev->path);
			close(fd);
			return -EINVAL;
		}
		dev->va = mmap(NULL, dev->reg_size, PROT_READ | PROT_WRITE,
			       MAP_SHARED, fd, 0);
		if (dev->va == MAP_FAILED) {
			DFX_ERR("mmap %s, 0x%lx", dev->path, dev->reg_size);
			dev->va = NULL;
			close(fd);
			return -EINVAL;
		}
		dev->id = fd;
		DFX_DBG("fd, mmap(%s) = %d, %p ", dev->dev_name, fd, dev->va);
	} else {
		DFX_ERR("failed to access path %s", dev->path);
		return -EINVAL;
	}
	return 0;
}

static int acapd_generic_device_close(acapd_device_t *dev)
{
	acapd_assert(dev != NULL);
	DFX_DBG("%s", dev->dev_name);
	if (dev->va == NULL) {
		return 0;
	}
	dev->va = NULL;
	close(dev->id);
	return 0;
}

acapd_device_ops_t acapd_linux_generic_dev_ops = {
	.open = acapd_generic_device_open,
	.close = acapd_generic_device_close,
	.attach = NULL,
	.detach = NULL,
	.va_to_da = NULL,
};
