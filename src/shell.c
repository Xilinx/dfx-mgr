/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/shell.h>
#include <acapd/device.h>
#include <acapd/assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

const acapd_shell_regs_t shell_1x1_regs = {
	.clock_release = 0x4000U,
	.clock_status = 0x4000U,
	.reset_release = 0x4004U,
	.reset_status = 0x4004U,
	.clock_release_mask = 0x1U,
	.reset_release_mask = 0x1U,
};

static acapd_shell_t shell = {
	.regs = &shell_1x1_regs,
};

int acapd_shell_config(const char *config)
{
	return sys_shell_config(&shell, config);
}

int acapd_shell_get()
{
	if (shell.dev.va == NULL) {
		int ret;

		ret = acapd_shell_config(NULL);
		if (ret < 0) {
			acapd_perror("%s: no shell dev specified.\n",
				     __func__);
			return ACAPD_ACCEL_FAILURE;
		} else {
			return 0;
		}
		ret = acapd_device_open(&(shell.dev));
		if (ret < 0) {
			acapd_perror("%s: failed to open shell dev.\n");
			return ACAPD_ACCEL_FAILURE;
		}
	} else {
		return 0;
	}
}

int acapd_shell_put()
{
	/* Empty for now */
	return 0;
}

int acapd_shell_release_isolation(acapd_accel_t *accel)
{
	/* TODO: FIXME: hardcoded to release isolation */
	void *reg_va;
	acapd_device_t *dev;
	const acapd_shell_regs_t *regs;
	uint32_t v;

	(void)accel;
	dev = &shell.dev;
	regs = shell.regs;
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
	acapd_debug("%s(%p): release isolation\n", __func__, reg_va);
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
	acapd_debug("%s(%p): release isolation done\n", __func__, reg_va);
	return 0;
}

int acapd_shell_assert_isolation(acapd_accel_t *accel)
{
	/* TODO: FIXME: hardcoded to release isolation */
	void *reg_va;
	acapd_device_t *dev;
	uint32_t v;
	const acapd_shell_regs_t *regs;

	acapd_assert(accel != NULL);
	dev = &(shell.dev);
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
	regs = shell.regs;
	acapd_debug("%s(%p): assert isolation\n", __func__, reg_va);
	*((volatile uint32_t *)((char *)reg_va + regs->clock_release)) = 0;
	while(1) {
		v = *((volatile uint32_t *)((char *)reg_va + regs->clock_status));
		if ((v & regs->clock_release_mask) == 0) {
			break;
		}
	}
	*((volatile uint32_t *)((char *)reg_va + regs->reset_release)) = 0;
	while(1) {
		v = *((volatile uint32_t *)((char *)reg_va + regs->reset_status));
		if ((v & regs->reset_release_mask) == 0) {
			break;
		}
	}
	return 0;
}
