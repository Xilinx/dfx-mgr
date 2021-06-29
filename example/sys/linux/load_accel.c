#include <dfx-mgr/accel.h>
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
	ret = loadapp(argv[0]);
	printf("loadapp returned %d\n",ret);
}

