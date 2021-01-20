/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/dma.h>
#include <dfx-mgr/shm.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DATA_SIZE_BYTES (4*1024)

static acapd_accel_t bzip2_accel;
static acapd_shm_t tx_shm, rx_shm;

void usage (const char *cmd)
{
	fprintf(stdout, "Usage %s -p <pkg_path>\n", cmd);
}

void sig_handler(int signo)
{
	(void)signo;
	remove_accel(&bzip2_accel, 0);
}

int main(int argc, char *argv[])
{
	int opt;
	char *pkg_path = NULL;
	int ret;
	void *tx_va, *rx_va;
	uint32_t *dptr;

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			pkg_path = optarg;
			break;
		default:
			usage(argv[0]);
			return -EINVAL;
		}
	}

	if (pkg_path == NULL) {
		usage(argv[0]);
		return -EINVAL;
	}

	printf("Setting accel devices.\n");

	/* allocate memory */
	printf("Initializing accel with %s.\n", pkg_path);
	init_accel(&bzip2_accel, (acapd_accel_pkg_hd_t *)pkg_path);

	signal(SIGINT, sig_handler);
	printf("Loading accel %s.\n", pkg_path);
	ret = load_accel(&bzip2_accel, NULL, 0);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to load accel.\n");
		goto error;
	}

	memset(&tx_shm, 0, sizeof(tx_shm));
	memset(&rx_shm, 0, sizeof(rx_shm));
	tx_va = acapd_accel_alloc_shm(&bzip2_accel, DATA_SIZE_BYTES, &tx_shm);
	if (tx_va == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate tx memory.\n");
		ret = -EINVAL;
		goto error;
	}
	dptr = (uint32_t *)tx_va;
	for (uint32_t i = 0; i < DATA_SIZE_BYTES/4; i++) {
		*((uint32_t *)dptr) = i + 1;
		dptr++;
	}
	rx_va = acapd_accel_alloc_shm(&bzip2_accel, DATA_SIZE_BYTES, &rx_shm);
	if (rx_va == NULL) {
		fprintf(stderr, "ERROR: allocate rx memory.\n");
		ret = -EINVAL;
		goto error;
	}

	/* user can use acapd_accel_get_reg_va() to get accelerator address */

	/* Transfer data */
	ret = acapd_accel_write_data(&bzip2_accel, &tx_shm, tx_va, DATA_SIZE_BYTES, 0, 0);
	if (ret < 0) {
		fprintf(stderr, "ERROR: Failed to write to accelerator.\n");
		ret = -EINVAL;
		goto error;
	}
	/* TODO: Execute acceleration (optional as load_accel can also start
	 * it from CDO */

	/* Wait for output data ready */
	/* For now, this function force to return 1 as no ready pin can poke */
	ret = acapd_accel_wait_for_data_ready(&bzip2_accel);
	if (ret < 0) {
		fprintf(stderr, "Failed to check if accelerator is ready.\n");
		ret = -EINVAL;
		goto error;
	}
	/* Read data */
	ret = acapd_accel_read_data(&bzip2_accel, &rx_shm, rx_va, DATA_SIZE_BYTES, 1);
	if (ret < 0) {
		fprintf(stderr, "Failed to read from accelerator.\n");
		return -EINVAL;
	}
	dptr = (uint32_t *)rx_va;
	for (uint32_t i = 0; i < DATA_SIZE_BYTES/4; i++) {
		if (*((uint32_t *)dptr) != (i + 1)) {
			fprintf(stderr, "ERROR: wrong data: [%d]: 0x%x.\n",
				i, *((volatile uint32_t *)dptr));
			ret = -EINVAL;
			goto error;
		}
		dptr++;
	}
	ret = 0;
	printf("Test Done.\n");
error:
	printf("Removing accel %s.\n", pkg_path);
	remove_accel(&bzip2_accel, 0);
	return ret;
}

