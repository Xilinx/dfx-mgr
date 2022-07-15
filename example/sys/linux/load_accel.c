#include <dfx-mgr/dfxmgr_client.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char uio_path[64];

int main(int argc, char *argv[])
{
	int ret;
	
	if (argc < 2) {
		printf("Enter the package to load\n");
		return 0;
	}
	ret = dfxmgr_load(argv[1]);
	printf("dfxmgr_load returned %d\n", ret);

	dfxmgr_uio_by_name(uio_path, ret, "SIHA");
	printf("%s SIHA\n", uio_path);

	dfxmgr_uio_by_name(uio_path, ret, "AccelConfig");
	printf("%s AccelConfig\n", uio_path);

	dfxmgr_uio_by_name(uio_path, ret, "rm_comm_box");
	printf("%s rm_comm_box\n", uio_path);

	return dfxmgr_remove(ret);
}
