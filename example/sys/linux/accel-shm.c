#include <acapd/accel.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


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
	int ret;

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

	/* allocate memory */
	ret = acapd_accel_alloc_shm(&bzip2_acccel, 1024*1024, &tx_shm);
	ret = acapd_accel_alloc_shm(&bzip2_acccel, 1024*1024, &rx_shm);

	/* Transfer data */
	ret = acapd_accel_transfer_data(&bzip2_accel, &tx_shm, NULL);
	/* TODO: Execute acceleration (optional as load_accel can also start
	 * it from CDO */

	/* Wait for output data ready */
	ret = acapd_accel_wait_for_data_ready(&bzip2_accel);
	/* Read data */
	ret = acapd_accel_transfer_data(&bzip2_accel, NULL, &rx_shm);

	sleep(2);
	printf("Removing accel %s.\n", pkg_path);
	remove_accel(&bzip2_accel, 0);
	return 0;
}

