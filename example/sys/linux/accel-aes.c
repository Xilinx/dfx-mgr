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
static acapd_device_t shell_dev;
static acapd_device_t rm_dev;
static acapd_device_t ip_dev[2];
static acapd_device_t dma_dev;
static acapd_chnl_t chnls[2];

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
	void *va;
	uint32_t v;

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
	memset(&shell_dev, 0, sizeof(shell_dev));
	memset(&rm_dev, 0, sizeof(rm_dev));
	memset(&ip_dev, 0, sizeof(ip_dev));
	memset(&dma_dev, 0, sizeof(dma_dev));
	shell_dev.dev_name = "90000000.gpio";
	ip_dev[0].dev_name = "20100000000.ap_start";
	ip_dev[1].dev_name = "20100001000.key";
	dma_dev.dev_name = "a4000000.dma";
	dma_dev.driver = "vfio-platform";
	dma_dev.iommu_group = 0;

	/* TODO adding channels to acceleration */
	memset(chnls, 0, sizeof(chnls));
	chnls[0].dev = &dma_dev;
	chnls[0].ops = &axidma_vfio_dma_ops;
	chnls[0].dir = ACAPD_DMA_DEV_W;
	chnls[1].dev = &dma_dev;
	chnls[1].ops = &axidma_vfio_dma_ops;
	chnls[1].dir = ACAPD_DMA_DEV_R;
	/* allocate memory */
	printf("Initializing accel with %s.\n", pkg_path);
	init_accel(&bzip2_accel, (acapd_accel_pkg_hd_t *)pkg_path);

	bzip2_accel.num_ip_devs = 2;
	bzip2_accel.ip_dev = ip_dev;
	bzip2_accel.shell_dev = &shell_dev;
	bzip2_accel.chnls = chnls;
	bzip2_accel.num_chnls = 2;

	signal(SIGINT, sig_handler);
	printf("Loading accel %s.\n", pkg_path);
	ret = load_accel(&bzip2_accel,NULL, 0);
	if (ret != 0) {
		fprintf(stderr, "ERROR: failed to load accel.\n");
		goto error;
	}

	va = acapd_accel_get_reg_va(&bzip2_accel, ip_dev[1].dev_name);
	*((volatile uint32_t *)va) = 0xdeadbeef;
	v = *((volatile uint32_t *)va);
	if (v != 0xdeadbeef) {
		fprintf(stderr, "ERROR: failed to read from accel: 0x%x.\n",
			v);
		ret = -1;
		goto error;
	}

	ret = 0;
	printf("Test Done.\n");
error:
	printf("Removing accel %s.\n", pkg_path);
	remove_accel(&bzip2_accel, 0);
	return ret;
}

