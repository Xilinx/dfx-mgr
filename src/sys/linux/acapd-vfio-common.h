/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    acapd-vfio-common.h
 */

#ifndef _ACAPD_VFIO_COMMON_H
#define _ACAPD_VFIO_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dfx-mgr/device.h>
#include <dfx-mgr/helper.h>

typedef struct acapd_vfio_mmap {
	void *va; /**< logical address */
	uint64_t da; /**< device address */
	size_t size; /**< size of the mmapped memory */
	acapd_list_t node; /**< node */
} acapd_vfio_mmap_t;

typedef struct acapd_vfio {
	int container; /**< vfio container fd */
	int group; /**< vfio group id */
	int device; /**< vfio device fd */
	acapd_list_t mmaps; /**< memory maps list */
} acapd_vfio_t;

extern acapd_device_ops_t acapd_vfio_dev_ops;
#endif /* _ACAPD_VFIO_COMMON_H */
