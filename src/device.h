/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	device.h
 * @brief	device definition.
 */

#ifndef _ACAPD_DEVICE_H
#define _ACAPD_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <acapd/helper.h>
#include <stddef.h>
#include <stdint.h>

#define ACAPD_ACCEL_STATUS_UNLOADED	0U
#define ACAPD_ACCEL_STATUS_LOADING	1U
#define ACAPD_ACCEL_STATUS_INUSE	2U
#define ACAPD_ACCEL_STATUS_UNLOADING	3U

#define ACAPD_ACCEL_SUCCESS		0
#define ACAPD_ACCEL_FAILURE		(-1)
#define ACAPD_ACCEL_TIMEDOUT		(-2)
#define ACAPD_ACCEL_MISMATCHED		(-3)
#define ACAPD_ACCEL_RSCLOCKED		(-4)
#define ACAPD_ACCEL_NOTSUPPORTED	(-5)
#define ACAPD_ACCEL_INVALID		(-6)
#define ACAPD_ACCEL_LOAD_INUSE		(-7)

#define ACAPD_ACCEL_INPROGRESS		1

#define ACAPD_ACCEL_PKG_TYPE_NONE	0U
#define ACAPD_ACCEL_PKG_TYPE_PDI	1U
#define ACAPD_ACCEL_PKG_TYPE_LAST	2U

typedef struct acapd_device acapd_device_t;
typedef struct acapd_shm acapd_shm_t;

typedef struct acapd_device_ops {
	int (*open)(acapd_device_t *dev);
	int (*close)(acapd_device_t *dev);
	void *(*attach)(acapd_device_t *dev, acapd_shm_t *shm);
	int (*detach)(acapd_device_t *dev, acapd_shm_t *shm);
	uint64_t (*va_to_da)(acapd_device_t *dev, void *va);
} acapd_device_ops_t;

typedef struct acapd_device {
	char *dev_name; /**< device name */
	char *bus_name; /**< bus name */
	char path[64]; /**< file path */
	uint64_t reg_pa; /**< physical base address */
	size_t reg_size; /**< size of the registers */
	int id; /**< device id. In Linux, it can be file id */
	int intr_id; /**< interrupt id */
	void *va; /**< logical address */
	char *version; /**< device version */
	char *driver; /**< name of the driver */
	int iommu_group; /**< iommu group */
	int refs; /**< references to this device */
	int dma_hls_fd;
	acapd_device_ops_t *ops; /**< device operation */
	void *priv; /**< device private information */
	acapd_list_t node; /**< node to link to shm reference list */
} acapd_device_t;

int acapd_device_open(acapd_device_t *dev);
int acapd_device_close(acapd_device_t *dev);
int acapd_device_get(acapd_device_t *dev);
int acapd_device_put(acapd_device_t *dev);
void *acapd_device_attach_shm(acapd_device_t *dev, acapd_shm_t *shm);
int acapd_device_detach_shm(acapd_device_t *dev, acapd_shm_t *shm);
void *acapd_device_get_reg_va(acapd_device_t *dev);
int acapd_device_get_id(acapd_device_t *dev);

#ifdef ACAPD_INTERNAL
int sys_device_open(acapd_device_t *dev);

#endif /* ACAPD_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_DEVICE_H */
