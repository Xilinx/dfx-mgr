/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/shell.h>
#include <dfx-mgr/device.h>
#include <dfx-mgr/assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static acapd_shell_t shell;

int acapd_shell_config(const char *config)
{
	return sys_shell_config(&shell, config);
}
int acapd_shell_fd(){
	printf("%s enter fd %d\n",__func__,shell.dev.id);
	return shell.dev.id;
}
int acapd_shell_clock_fd(){
	return shell.clock_dev.id;
}
//int acapd_shell_get(const char *config)
//{
//	if (shell.dev.va == NULL) {
//		int ret;
//
//		ret = acapd_shell_config(config);
//		if (ret < 0) {
//			acapd_perror("%s: no shell dev specified.\n",
//				     __func__);
//			return ACAPD_ACCEL_FAILURE;
//		} else {
//			return 0;
//		}
//		ret = acapd_device_open(&(shell.dev));
//		if (ret < 0) {
//			acapd_perror("%s: failed to open shell dev.\n");
//			return ACAPD_ACCEL_FAILURE;
//		}
//	} else {
//		return 0;
//	}
//}

//int acapd_shell_put()
//{
	/* Empty for now */
//	return 0;
//}

int acapd_shell_release_isolation(acapd_accel_t *accel)
{
	/* TODO: FIXME: hardcoded to release isolation */
	void *reg_va;
	acapd_device_t *dev;
	acapd_shell_regs_t regs;
	//uint32_t v;
	int i, ret;

	(void)accel;
	dev = &shell.dev;
	regs = shell.slot_regs[accel->rm_slot];
	acapd_debug("%s: %s.\n", __func__, dev->dev_name);
	reg_va = dev->va;
	if (reg_va == NULL) {
		printf("%s Open shell device \n",__func__);
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
		printf("Opening shell clock_dev\n");
		ret = acapd_device_open(&shell.clock_dev);
		if (ret < 0) {
			acapd_perror("%s: failed to open shell clock_dev %s.\n",
				     __func__, shell.clock_dev.dev_name);
			return ACAPD_ACCEL_FAILURE;
		}
	}
	acapd_debug("%s(%p): release isolation\n", __func__, reg_va);
	for (i=0; i<4; i++){
		*((volatile uint32_t *)((char *)reg_va + regs.offset[i])) = regs.values[i];
	}

	//while(1) {
	//	v = *((volatile uint32_t *)((char *)reg_va + regs->clock_status));
	//	if ((v & regs->clock_release_mask) != 0) {
	//		break;
	//	}
	//}
	//*((volatile uint32_t *)((char *)reg_va + regs->reset_release)) = 0x1;
	//while(1) {
	//	v = *((volatile uint32_t *)((char *)reg_va + regs->reset_status));
	//	if ((v & regs->reset_release_mask) != 0) {
	//		break;
	//	}
	//}
	acapd_debug("%s(%p): release isolation done\n", __func__, reg_va);
	return 0;
}

int acapd_shell_assert_isolation(acapd_accel_t *accel)
{
	/* TODO: FIXME: hardcoded to release isolation */
	void *reg_va;
	acapd_device_t *dev;
	//uint32_t v;
	acapd_shell_regs_t regs;
	int i;

	acapd_assert(accel != NULL);
	dev = &(shell.dev);
	regs = shell.slot_regs[accel->rm_slot];
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
	acapd_debug("%s(%p): assert isolation\n", __func__, reg_va);
	for (i=0; i<4; i++){
		*((volatile uint32_t *)((char *)reg_va + regs.offset[i])) = 0;
	}
	//while(1) {
	//	v = *((volatile uint32_t *)((char *)reg_va + regs->clock_status));
	//	if ((v & regs->clock_release_mask) == 0) {
	//		break;
	//	}
	//}
	//*((volatile uint32_t *)((char *)reg_va + regs->reset_release)) = 0;
	//while(1) {
	//	v = *((volatile uint32_t *)((char *)reg_va + regs->reset_status));
	//	if ((v & regs->reset_release_mask) == 0) {
	//		break;
	//	}
	//}
	return 0;
}

int get_shell_slots()
{
	return shell.num_slots;
}
