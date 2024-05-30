/*
 * Copyright (C) 2022 - 2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <dfx-mgr/print.h>


/**
 * get_number_of_rpu() - Get the number of remoteproc's created
 *
 * This function looks into /sys/class/remoteproc directory and
 * identifies the number of remoteproc entries available, this
 * will be the number of rpu's
 *
 * Return: Number of rpu
 */
int get_number_of_rpu(void)
{
        int no_of_rpu=0;
        char *rpu_path = "/sys/class/remoteproc/";
        DIR *dir_rpu = NULL;
        struct dirent *drpu;

        dir_rpu = opendir(rpu_path);
        if (dir_rpu == NULL) {
                DFX_ERR("Directory %s not found", rpu_path);
                return 0;
        }

        while((drpu = readdir(dir_rpu)) != NULL) {
                if (!strcmp(drpu->d_name, ".") || !strcmp(drpu->d_name, ".."))
                        continue;    /* skip self and parent */
                if (strstr(drpu->d_name, "remoteproc"))
                        no_of_rpu++;
        }

        return no_of_rpu;
}


/**
 * load_rpu() - load and start RPU firmware
 * @*rpu_path - location of the firmware
 * @rpu_slot - RPU number to load the firmware
 *
 * This function does the following
 * 1 - Set the firmware location path
 * 2 - Load the firmware
 * 3 - start the firmware
 *
 * Return: slot_number on success
 * 	   -1 on error
 */
int load_rpu( char *rpu_path, int rpu_slot)
{
	int ret;
	char cmd[1024];
	DIR *dir1 = NULL;
	int found_firmware = 0;
	struct dirent *d1;

	DFX_DBG("rpu_path %s rpu_slot %d\n",rpu_path,rpu_slot);

	DFX_DBG("Setting firmware location to  base_path %s\n",rpu_path);
	sprintf(cmd,"echo -n %s > /sys/module/firmware_class/parameters/path", rpu_path);
	ret = system(cmd);
	if(ret != 0 ){
		printf("Command not successful %s\n",cmd);
		return -1;
	}

	dir1 = opendir(rpu_path);
	if (dir1 == NULL) {
		DFX_ERR("Directory %s not found", rpu_path);
		return -1;
	}

	while((d1 = readdir(dir1)) != NULL) {
		if (!strcmp(d1->d_name, ".") || !strcmp(d1->d_name, ".."))
			continue;    /* skip self and parent */
		found_firmware = 1;
		DFX_DBG("Loading RPU firmware %s in slot %d\n",d1->d_name, rpu_slot);

		sprintf(cmd,"echo %s > /sys/class/remoteproc/remoteproc%d/firmware", d1->d_name, rpu_slot);
		ret = system(cmd);
		if(ret != 0 ){
			DFX_ERR("Command not successful %s\n",cmd);
			return -1;
		}

		DFX_DBG("Starting RPU firmware %s in slot %d\n",d1->d_name, rpu_slot);
		sprintf(cmd,"echo start > /sys/class/remoteproc/remoteproc%d/state", rpu_slot);
		ret = system(cmd);
		if(ret != 0 ){
			DFX_ERR("Command not successful %s\n",cmd);
			return -1;
		}
	}
	if (found_firmware == 1)
		return rpu_slot;
	else{
		DFX_ERR("No firmware found in folder\n");
		return -1;
	}
}
