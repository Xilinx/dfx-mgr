/*
 * Copyright (c) 2020, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file    acapd-vfio-common.h
 */

#ifndef _ACAPD_LINUX_GENERIC_DEVICE_H
#define _ACAPD_LINUX_GENERIC_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <acapd/device.h>

extern acapd_device_ops_t acapd_linux_generic_dev_ops;

int acapd_generic_device_bind(acapd_device_t *dev, const char *drv);
#endif /* _ACAPD_LINUX_GENERIC_DEVICE_H */
