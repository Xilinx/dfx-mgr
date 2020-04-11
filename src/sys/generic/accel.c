/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/print.h>
#include <errno.h>
#include <fcntl.h>
#include "json-config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <xilfpga.h>

#define DTBO_ROOT_DIR "/sys/kernel/config/device-tree/overlays"

int sys_needs_load_accel(acapd_accel_t *accel)
{
	/* Always loads accel */
	(void)accel;
	return 1;
}

int sys_accel_config(acapd_accel_t *accel)
{
	acapd_accel_sys_t *sys_pkg;
	const char *json_config;
	int ret;

	acapd_assert(accel != NULL);
	acapd_assert(accel->pkg != NULL);
	sys_pkg = (acapd_accel_sys_t *)(accel->pkg);
	acapd_assert(sys_pkg->accel_json_pa != 0);
	acapd_assert(sys_pkg->accel_json_size != 0);
	json_config = (const char *)((uintptr_t)sys_pkg->accel_json_pa);
	ret = parseAccelJson(accel, json_config,
			     (size_t)sys_pkg->accel_json_size);
	for (int i = 0; i < accel->num_ip_devs; i++) {
		acapd_device_t *dev;

		dev = &(accel->ip_dev[i]);
		dev->va = (void*)((uintptr_t)(dev->reg_pa));
		acapd_debug("%s: ipdev[%d], %p:0x%lx.\n",
			    __func__, i, dev->va, dev->reg_pa);
	}
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_chnl_t *chnl;
		acapd_device_t *dev;

		chnl = &(accel->chnls[i]);
		dev = chnl->dev;
		dev->va = (void *)((uintptr_t)(dev->reg_pa));
		acapd_debug("%s: channel[%d], %p:0x%lx.\n",
			    __func__, i, dev->va, dev->reg_pa);
	}
	return ret;
}

int sys_fetch_accel(acapd_accel_t *accel)
{
	(void)accel;
	return ACAPD_ACCEL_SUCCESS;
}

int sys_load_accel(acapd_accel_t *accel, unsigned int async)
{
	XFpga XFpgaInst = {0};
	//const char *pdi;
	int ret;
	acapd_accel_sys_t *sys_pkg;

	acapd_assert(accel != NULL);
	acapd_assert(accel->pkg != NULL);
	/* TODO: only support synchronous mode for now */
	(void)async;

	sys_pkg = (acapd_accel_sys_t *)(accel->pkg);
	acapd_assert(sys_pkg->accel_pdi_pa != 0);
	acapd_assert(sys_pkg->accel_pdi_size != 0);
	acapd_debug("%s: pdi pa: 0x%llx, size=0x%llx.\n",
		    __func__, sys_pkg->accel_pdi_pa, sys_pkg->accel_pdi_size);
	//pdi = (const char *)((uintptr_t)sys_pkg->accel_pdi_pa);
#if 0
	if (pkg->type != ACAPD_ACCEL_PKG_TYPE_PDI) {
		acapd_perror("Failed to load accel, none supported type %u.\n",
			     pkg->type);
		return ACAPD_ACCEL_FAILURE;
	}
#endif

	ret = XFpga_Initialize(&XFpgaInst);
	if (ret != XST_SUCCESS) {
		acapd_perror("Failed to init fpga instance.\n");
		accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
		accel->load_failure = ACAPD_ACCEL_INVALID;
		return ACAPD_ACCEL_FAILURE;
	}

#if 0
	ret = XFpga_PL_BitStream_Load(&XFpgaInst, (UINTPTR)((char *)pkg + sizeof(*pkg)), 0, 1);
#else
	ret = XFpga_PL_BitStream_Load(&XFpgaInst, sys_pkg->accel_pdi_pa, 0, 0);
#endif
	if (ret != XFPGA_SUCCESS) {
		return ACAPD_ACCEL_FAILURE;
	} else {
		/* TODO: FIXME: hardcoded to release isolation */
		//Xil_Out32(0x90010000, 0x1);
		//Xil_Out32(0x90000000, 0x1);
		return ACAPD_ACCEL_SUCCESS;
	}
}

int sys_load_accel_post(acapd_accel_t *accel)
{
	(void)accel;
	return 0;
}

int sys_close_accel(acapd_accel_t *accel)
{
	(void)accel;
	return 0;
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{

	/* TODO: Do nothing for now */
	(void)async;
	(void)accel;
	return ACAPD_ACCEL_SUCCESS;
}


