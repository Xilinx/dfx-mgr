#include <acapd/accel.h>
#include <acapd/dma.h>
#include <acapd/shm.h>
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
	memset(&shell_dev, 0, sizeof(shell_dev));
	memset(&rm_dev, 0, sizeof(rm_dev));
	memset(&ip_dev, 0, sizeof(ip_dev));
	memset(&dma_dev, 0, sizeof(dma_dev));
	shell_dev.dev_name = "90000000.gpio";
	ip_dev[0].dev_name = "20100000000.bram";
	ip_dev[1].dev_name = "20120000000.axi_cdma";
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
	bzip2_accel.rm_dev = &rm_dev;
	bzip2_accel.chnls = chnls;
	bzip2_accel.num_chnls = 2;

	signal(SIGINT, sig_handler);
	printf("Loading accel %s.\n", pkg_path);
	ret = load_accel(&bzip2_accel, 0);
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
	ret = acapd_accel_write_data(&bzip2_accel, &tx_shm, tx_va, DATA_SIZE_BYTES, 0);
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
			ret = -EINVAL;
			goto error;
		}
		dptr++;
	}
	ret = 0;
	printf("Test Done.\n");
#endif
error:
	printf("Removing accel %s.\n", pkg_path);
	remove_accel(&bzip2_accel, 0);
	return ret;
}

