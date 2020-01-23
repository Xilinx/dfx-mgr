#include <acapd/accel.h>
#include <acapd/dma.h>
#include <acapd/shm.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DATA_SIZE_BYTES (4*1024)

void usage (const char *cmd)
{
	fprintf(stdout, "Usage %s -p <pkg_path>\n", cmd);
}

int main(int argc, char *argv[])
{
	int opt;
	char *pkg_path = NULL;
	acapd_accel_t bzip2_accel;
	acapd_shm_t tx_shm, rx_shm;
	acapd_chnl_t chnls[2];
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
	printf("Initializing accel with %s.\n", pkg_path);
	init_accel(&bzip2_accel, (acapd_accel_pkg_hd_t *)pkg_path);

	printf("Loading accel %s.\n", pkg_path);
	load_accel(&bzip2_accel, 0);

	/* TODO adding channels to acceleration */
	memset(chnls, 0, sizeof(chnls));
	chnls[0].dev_name = "a4000000.dma";
	chnls[0].ops = &axidma_vfio_dma_ops;
	chnls[0].dir = ACAPD_DMA_DEV_W;
	chnls[1].dev_name = "a4000000.dma";
	chnls[1].ops = &axidma_vfio_dma_ops;
	chnls[1].dir = ACAPD_DMA_DEV_R;
	bzip2_accel.chnls = chnls;
	bzip2_accel.num_chnls = 2;

	/* allocate memory */
	tx_va = acapd_accel_alloc_shm(&bzip2_accel, DATA_SIZE_BYTES, &tx_shm);
	if (tx_va == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate tx memory.\n");
		return -EINVAL;
	}
	dptr = (uint32_t *)tx_va;
	for (uint32_t i = 0; i < DATA_SIZE_BYTES/4; i++) {
		*((uint32_t *)dptr) = i + 1;
		dptr++;
	}
	rx_va = acapd_accel_alloc_shm(&bzip2_accel, DATA_SIZE_BYTES, &rx_shm);
	if (rx_va == NULL) {
		fprintf(stderr, "ERROR: allocate rx memory.\n");
		return -EINVAL;
	}

	/* Transfer data */
	ret = acapd_accel_write_data(&bzip2_accel, &tx_shm, tx_va, DATA_SIZE_BYTES, 0);
	if (ret < 0) {
		fprintf(stderr, "ERROR: Failed to write to accelerator.\n");
		return -EINVAL;
	}
	/* TODO: Execute acceleration (optional as load_accel can also start
	 * it from CDO */

	/* Wait for output data ready */
	/* For now, this function force to return 1 as no ready pin can poke */
	ret = acapd_accel_wait_for_data_ready(&bzip2_accel);
	if (ret < 0) {
		fprintf(stderr, "Failed to check if accelerator is ready.\n");
		return -EINVAL;
	}
	/* Read data */
#if 1
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
			return -1;
		}
		dptr++;
	}
#endif
	sleep(2);
	printf("Removing accel %s.\n", pkg_path);
	remove_accel(&bzip2_accel, 0);
	return 0;
}

