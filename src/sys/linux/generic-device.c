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
	if (dev->bus_name == NULL) {
		dev->bus_name = "platform";
	}
	sprintf(tmpstr, "/sys/bus/%s/drivers", dev->bus_name);
	d = opendir(tmpstr);
	if (d == NULL) {
		acapd_perror("%s, failed to bind %s, no %s.\n",
			     __func__, dev->dev_name, tmpstr);
		return -EINVAL;
	}
	while((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_DIR) {
			if (strlen(dir->d_name) > 64) {
				continue;
			}
			sprintf(tmpstr, "/sys/bus/%s/drivers/%s/%s",
				dev->bus_name, dir->d_name, dev->dev_name);
			if (access(tmpstr, F_OK) == 0) {
				if (strcmp(dir->d_name, drv) == 0) {
					acapd_debug("%s: dev %s already bound.\n",
						    __func__, dev->dev_name);
					ret = 0;
					goto out;
				}
				/* Unbind driver */
				sprintf(tmpstr, "/sys/bus/%s/drivers/%s/unbind",
					dev->bus_name, dir->d_name);
				fd = open(tmpstr, O_WRONLY);
				if (fd < 0) {
					acapd_perror("%s: failed to open %s.\n",
						     __func__, tmpstr);
					ret = -EINVAL;
					goto out;
				}
				ret = write(fd, dev->dev_name, strlen(dev->dev_name) + 1);
				if (ret < 0) {
					acapd_perror("%s: failed to unbind %s from %s.\n",
						     __func__, dev->dev_name, dir->d_name);
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
		acapd_perror("%s: failed to open %s.\n",
			     __func__, tmpstr);
		ret = -EINVAL;
		goto out;
	}
	ret = write(fd, drv, strlen(drv) + 1);
	if (ret < 0) {
		acapd_perror("%s: failed to override %s to %s.\n",
			     __func__,drv, dev->dev_name);
		close(fd);
		goto out;
	}
	sprintf(tmpstr, "/sys/bus/%s/drivers/%s/bind",
		dev->bus_name, drv);
	fd = open(tmpstr, O_WRONLY);
	if (fd < 0) {
		acapd_perror("%s: failed to open %s.\n",
			     __func__, tmpstr);
		ret = -EINVAL;
		goto out;
	}
	ret = write(fd, dev->dev_name, strlen(dev->dev_name) + 1);
	if (ret < 0) {
		acapd_perror("%s: failed to bind %s to %s.\n",
			     __func__, dev->dev_name, drv);
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
	d = opendir(tmpstr);
	if (d == NULL) {
		acapd_perror("%s: cannot open %s.\n", __func__, tmpstr);
		return -EINVAL;
	}
	ret = -EINVAL;
	while ((dir = readdir(d)) != NULL) {
		if (strlen(dir->d_name) > 8) {
			continue;
		};
		if(dir-> d_type == DT_DIR && strstr(dir->d_name, "uio") != NULL) {
			int fd;
			char size_str[32], tmpname[8];
			size_t size;
			int len;

			memset(dev->path, 0, sizeof(dev->path));
			memset(tmpname, 0, sizeof(tmpname));
			if (strlen(dir->d_name) >= sizeof(tmpname)) {
				len = sizeof(tmpname) - 1;
			} else {
				len = strlen(dir->d_name);
			}
			strncpy(tmpname, dir->d_name, len);
			sprintf(dev->path, "/dev/%s", tmpname);

			/* get io region size */
			sprintf(tmpstr, "/sys/bus/%s/devices/%s/uio/%s/maps/map0/size",
				dev->bus_name, dev->dev_name, dir->d_name);
			fd = open(tmpstr, O_RDONLY);
			if (fd < 0) {
				acapd_perror("%s: failed to open %s.\n",
					     __func__, tmpstr);
				close(fd);
				ret = -EINVAL;
				goto out;
			}
			memset(size_str, 0, sizeof(size_str));
			ret = read(fd, size_str, (sizeof(size) << 1) + 3);
			if (ret < 0) {
				acapd_perror("%s: failed to read %s.\n",
					     __func__, tmpstr);
				close(fd);
				goto out;
			}
			acapd_debug("%s: size %s.\n", __func__, size_str);
			size = (size_t)strtoll(size_str, NULL, 16);
			dev->reg_size = size;

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
				acapd_perror("%s: failed to bind uio to %s.\n",
					     __func__, dev->dev_name);
				return -EINVAL;
			}
			ret = acapd_generic_device_get_uio_path(dev);
			if (ret < 0) {
				acapd_perror("%s: failed to get uio path for %s.\n",
					     __func__, dev->dev_name);
				return -EINVAL;
			}
		} else {
			acapd_perror("%s: no dev name or path is specified.\n");
			return -EINVAL;
		}
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
		if (dev->reg_size == 0) {
			acapd_perror("%s: failed to get %s,%s size, as reg_siz is 0.\n",
				     __func__, dev->dev_name, dev->path);
			close(fd);
			return -EINVAL;
		}
		dev->va = mmap(NULL, dev->reg_size, PROT_READ | PROT_WRITE,
			       MAP_SHARED, fd, 0);
		if (dev->va == MAP_FAILED) {
			acapd_perror("%s: failed to mmap %s, 0x%lx.\n",
				     __func__, dev->path, dev->reg_size);
			dev->va = NULL;
			close(fd);
			return -EINVAL;
		}
		dev->id = fd;
	} else {
		acapd_perror("%s: failed to access path %s.\n",
			     __func__, dev->path);
		return -EINVAL;
	}
	return 0;
}

static int acapd_generic_device_close(acapd_device_t *dev)
{
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

