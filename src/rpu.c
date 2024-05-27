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
