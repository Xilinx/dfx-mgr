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
	load_accel(&bzip2_accel, NULL, 0);

	sleep(2);
	printf("Removing accel %s.\n", pkg_path);
	remove_accel(&bzip2_accel, 0);
	return 0;
}

