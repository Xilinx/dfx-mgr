/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/dma.h>
#include <acapd/shm.h>
#include <errno.h>
#include <unistd.h>
#include <xil_printf.h>

#define DATA_SIZE_BYTES (4*1024)

extern unsigned char __START_ACCEL0[];
static acapd_shm_t tx_shm, rx_shm;

int main(void)
{
	void *tx_va, *rx_va;
	uint32_t *dptr;
	int ret;

	acapd_accel_pkg_hd_t *pkg1;
	//acapd_accel_pkg_hd_t *pkg2;
	acapd_accel_t accel;
	//int ret;

	/* Configure packages */
	/* TODO: This step should be replaced by host tool. */
	xil_printf("allocate memory for package\r\n");

	pkg1 = (acapd_accel_pkg_hd_t *)__START_ACCEL0;

	/* Initialize accelerator with package */
	xil_printf("Initialize accelerator with packge.\r\n");
	init_accel(&accel, pkg1);

	/* Load accelerator */
	xil_printf("Load accelerator with packge.\r\n");
	ret = load_accel(&accel, 0);
	if (ret != 0) {
		xil_printf("ERROR: failed to load accel.\n");
		return -1;
	}

	memset(&tx_shm, 0, sizeof(tx_shm));
	memset(&rx_shm, 0, sizeof(rx_shm));
	tx_va = acapd_accel_alloc_shm(&accel, DATA_SIZE_BYTES, &tx_shm);
	if (tx_va == NULL) {
		xil_printf("ERROR: Failed to allocate tx memory.\n");
		ret = -EINVAL;
		goto error;
	}
	dptr = (uint32_t *)tx_va;
	for (uint32_t i = 0; i < DATA_SIZE_BYTES/4; i++) {
		*((uint32_t *)dptr) = i + 1;
		dptr++;
	}
	rx_va = acapd_accel_alloc_shm(&accel, DATA_SIZE_BYTES, &rx_shm);
	if (rx_va == NULL) {
		xil_printf("ERROR: allocate rx memory.\n");
		ret = -EINVAL;
		goto error;
	}

	/* user can use acapd_accel_get_reg_va() to get accelerator address */

	/* Transfer data */
	ret = acapd_accel_write_data(&accel, &tx_shm, tx_va, DATA_SIZE_BYTES, 0);
	if (ret < 0) {
		xil_printf("ERROR: Failed to write to accelerator.\n");
		ret = -EINVAL;
		goto error;
	}
	/* TODO: Execute acceleration (optional as load_accel can also start
	 * it from CDO */

	/* Wait for output data ready */
	/* For now, this function force to return 1 as no ready pin can poke */
	ret = acapd_accel_wait_for_data_ready(&accel);
	if (ret < 0) {
		xil_printf("Failed to check if accelerator is ready.\n");
		ret = -EINVAL;
		goto error;
	}
	/* Read data */
	ret = acapd_accel_read_data(&accel, &rx_shm, rx_va, DATA_SIZE_BYTES, 1);
	if (ret < 0) {
		xil_printf("Failed to read from accelerator.\n");
		return -EINVAL;
	}
	dptr = (uint32_t *)rx_va;
	for (uint32_t i = 0; i < DATA_SIZE_BYTES/4; i++) {
		if (*((uint32_t *)dptr) != (i + 1)) {
			xil_printf("ERROR: wrong data: [%d]: 0x%x.\n",
				i, *((volatile uint32_t *)dptr));
			ret = -EINVAL;
			goto error;
		}
		dptr++;
	}
	ret = 0;
	printf("Test Done.\n");
	sleep(2);
error:
	remove_accel(&accel, 0);
	return ret;
}

