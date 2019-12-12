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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <xilfpga.h>

#define DTBO_ROOT_DIR "/sys/kernel/config/device-tree/overlays"

int sys_load_accel(acapd_accel_t *accel, unsigned int async)
{
	XFpga XFpgaInst = {0};
	int ret;
	acapd_accel_pkg_hd_t *pkg;

	/* TODO: only support synchronous mode for now */
	(void)async;

	pkg = accel->pkg;
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
	ret = XFpga_PL_BitStream_Load(&XFpgaInst, (UINTPTR)((char *)pkg), 0, 0);
#endif
	if (ret == XFPGA_SUCCESS) {
		return ACAPD_ACCEL_FAILURE;
	} else {
		/* TODO: FIXME: hardcoded to release isolation */
		Xil_Out32(0x90010000, 0x1);
		Xil_Out32(0x90000000, 0x1);
		return ACAPD_ACCEL_SUCCESS;
	}
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{

	/* TODO: Do nothing for now */
	(void)async;
	(void)accel;
	return ACAPD_ACCEL_SUCCESS;
}
