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

static acapd_accel_t bzip2_accel;

void usage (const char *cmd)
{
	fprintf(stdout, "Usage %s -p <pkg_path> -a <app_path>\n", cmd);
}

void sig_handler(int signo)
{
	(void)signo;
	printf("Removing accel.\n");
	remove_accel(&bzip2_accel, 0);
}

int main(int argc, char *argv[])
{
	int opt;
	char *pkg_path = NULL;
	char *app_path = NULL;
	int ret;

	while ((opt = getopt(argc, argv, "p:a:")) != -1) {
		switch (opt) {
		case 'p':
			pkg_path = optarg;
			break;
		case 'a':
			app_path = optarg;
			break;
		default:
			usage(argv[0]);
			return -EINVAL;
		}
	}

	if (pkg_path == NULL || app_path == NULL) {
		usage(argv[0]);
		return -EINVAL;
	}

	printf("Setting accel devices.\n");
	/* The Json Parsor is supposed to fill in this information */
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

	/* run user application */
error:
	printf("Removing accel %s.\n", pkg_path);
	remove_accel(&bzip2_accel, 0);
	return ret;
}

