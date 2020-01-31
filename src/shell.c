/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/shell.h>
#include <acapd/device.h>
#include <acapd/assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef struct acapd_shell_regs {
	uint32_t clock_release;
	uint32_t clock_status;
	uint32_t reset_release;
	uint32_t reset_status;
	uint32_t clock_release_mask;
	uint32_t reset_release_mask;
} acapd_shell_regs_t;

const acapd_shell_regs_t shel_1x1_regs = {
	.clock_release = 0x10000U,
	.clock_status = 0x10008U,
	.reset_release = 0x0U,
	.reset_status = 0x8U,
	.clock_release_mask = 0x1U,
	.reset_release_mask = 0x1U,
};

int acapd_shell_release_isolation(acapd_accel_t *accel, int slot)
{
	/* TODO: FIXME: hardcoded to release isolation */
	void *reg_va;
	acapd_device_t *dev;
	uint32_t v;
	const acapd_shell_regs_t *regs;

	(void)slot;
	acapd_assert(accel != NULL);
	acapd_assert(accel->shell_dev != NULL);

	if (accel->shell_dev == NULL) {
		acapd_debug("%s: no shell dev specified.\n",
			     __func__);
		return 0;
	}

	dev = accel->shell_dev;
	acapd_debug("%s: %s.\n", __func__, dev->dev_name);
	reg_va = dev->va;
	if (reg_va == NULL) {
		int ret;

		ret = acapd_device_open(dev);
		if (ret < 0) {
			acapd_perror("%s: failed to open shell dev %s.\n",
				     __func__, dev->dev_name);
			return ACAPD_ACCEL_FAILURE;
		}
		reg_va = dev->va;
		if (reg_va == NULL) {
			acapd_perror("%s: shell dev %s va is NULL.\n",
				     __func__, dev->dev_name);
			return ACAPD_ACCEL_FAILURE;
		}
	}
	regs = &shel_1x1_regs;
	*((volatile uint32_t *)((char *)reg_va + regs->clock_release)) = 0x1;
	while(1) {
		v = *((volatile uint32_t *)((char *)reg_va + regs->clock_status));
		if ((v & regs->clock_release_mask) != 0) {
			break;
		}
	}
	*((volatile uint32_t *)((char *)reg_va + regs->reset_release)) = 0x1;
	while(1) {
		v = *((volatile uint32_t *)((char *)reg_va + regs->reset_status));
		if ((v & regs->reset_release_mask) != 0) {
			break;
		}
	}
	return 0;
}

int acapd_shell_assert_isolation(acapd_accel_t *accel, int slot)
{
	/* TODO: FIXME: hardcoded to release isolation */
	void *reg_va;
	acapd_device_t *dev;
	uint32_t v;
	const acapd_shell_regs_t *regs;

	(void)slot;
	acapd_assert(accel != NULL);
	acapd_assert(accel->shell_dev != NULL);

	if (accel->shell_dev == NULL) {
		acapd_debug("%s: no shell dev specified.\n",
			     __func__);
		return 0;
	}

	dev = accel->shell_dev;
	reg_va = dev->va;
	if (reg_va == NULL) {
		int ret;

		ret = acapd_device_open(dev);
		if (ret < 0) {
			acapd_perror("%s: failed to open shell dev %s.\n",
				     __func__, dev->dev_name);
			return ACAPD_ACCEL_FAILURE;
		}
		reg_va = dev->va;
		if (reg_va == NULL) {
			acapd_perror("%s: shell dev %s va is NULL.\n",
				     __func__, dev->dev_name);
			return ACAPD_ACCEL_FAILURE;
		}
	}
	regs = &shel_1x1_regs;
	*((volatile uint32_t *)((char *)reg_va + regs->clock_release)) = 0;
	while(1) {
		v = *((volatile uint32_t *)((char *)reg_va + regs->clock_status));
		if ((v & regs->clock_release_mask) != 0) {
			break;
		}
	}
	*((volatile uint32_t *)((char *)reg_va + regs->reset_release)) = 0;
	while(1) {
		v = *((volatile uint32_t *)((char *)reg_va + regs->reset_status));
		if ((v & regs->reset_release_mask) != 0) {
			break;
		}
	}
	return 0;
}
