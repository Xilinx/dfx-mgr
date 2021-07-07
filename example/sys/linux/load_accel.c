#include <dfx-mgr/client_helper.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int ret;
	
	if (argc < 2) {
		printf("Enter the package to load\n");
		return 0;
	}
	ret = dfxmgr_load(argv[1]);
	printf("dfxmgr_load returned %d\n",ret);
	sleep(5);
	dfxmgr_remove(ret);
}

