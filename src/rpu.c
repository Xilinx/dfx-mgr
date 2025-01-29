/*
 * Copyright (C) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/shell.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/rpu_helper.h>


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


/**
 * remove_rpu() - remove/stop a loaded RPU firmware
 * @rpu_num - remoteproc number
 *
 * This function takes remoteproc number as input and
 * stops/removes the firmware running on it
 *
 * Return: 0 on success
 * 	  -1 on error
 */
int remove_rpu(int rpu_num)
{
	char cmd[1024];
	int ret;

	DFX_DBG("Remove firmware from RPU number %d", rpu_num);

	sprintf(cmd,"echo stop > /sys/class/remoteproc/remoteproc%d/state", rpu_num);
	ret = system(cmd);
	if(ret != 0 ){
		printf("Command not successful %s\n",cmd);
		return -1;
	}
	return 0;
}

/**
 * get_virtio_number() - return virtio number
 * @rpmsg_dev_name - rpmsg device or control name
 *
 * This function taken input rpmsg dev or control name
 * and returns the virtio number
 *
 * Return: virtio number on success
 *         0 if not found
 */

int get_virtio_number(char* rpmsg_dev_name)
{
	char *rpmsg_dev = strdup(rpmsg_dev_name); /* copy of dev name */
	char *virtio_str;
	int virtio_num = 0;

	virtio_str = strtok(rpmsg_dev,".");
	if (virtio_str != NULL) {
		while (*virtio_str) {
			if (isdigit(*virtio_str)) {
				/* get the virtio number */
				virtio_num = strtol(virtio_str, &virtio_str, 10);
			} else {
				/* Otherwise, move on to the next character */
				virtio_str++;
			}
		}
	}
	free(rpmsg_dev);
	return virtio_num;
}

/**
 * get_new_rpmsg_ctrl_dev() - return new rpmsg control dev found
 * @struct basePLDesign *base - PL base design
 *
 * This function checks if new rpmsg control dev is created by
 * RPU firmware and return the same, it does this by parsing through
 * /sys/device/rpmsg/devices directory takes each entries and comparing
 * with the previously stored data in base design structure
 *
 * Return: rpmsg_ctrl_dev_name on success
 * 	   NULL on failure
 *
 */
char* get_new_rpmsg_ctrl_dev(struct basePLDesign *base)
{
	char dpath[] = "/sys/bus/rpmsg/devices";
	DIR *dir = opendir(dpath);
	int rpmsg_ctrl_found = 0;
	struct dirent *ent;

	if (!dir) {
		fprintf(stderr, "opendir %s, %s\n", dpath, strerror(errno));
		return NULL;
	}
	while ((ent = readdir(dir)) != NULL) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") || strstr(ent->d_name, "rpmsg_ns"))
			continue;    /* skip self and parent */

		/* Find rpmsg ctrl dev file created */
		if (strstr(ent->d_name, "rpmsg_ctrl")) {
			/* Check if ctrl is already recorded */
			for(int i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++) {
				/* Check if slot exists */
				if (base->slots[i]) {
					/* Compare with existing records */
					if (!strncmp(base->slots[i]->rpu.rpmsg_ctrl_dev_name,ent->d_name,strlen(ent->d_name))) {
						DFX_DBG("rpmsg ctrl dev already recorded\n");
						rpmsg_ctrl_found = 1;
						break;
					}
				}
			}
			/* Entry already recorded */
			if (rpmsg_ctrl_found == 1) {
				rpmsg_ctrl_found = 0;
				continue;
			}

			closedir(dir);
			/* Return new ctrl dev found */
			return ent->d_name;
		}
	}

	closedir(dir);
	/* Return NULL if no new ctrl dev is found */
	return NULL;
}

/**
 * delete_rpmsg_dev_list - delete completed rpmsg_dev_list
 * @acapd_list_t *rpmsg_dev_list -- pointer to rpmsg_dev_list
 *
 * This function delete complete list
 * Called when RPU is removed
 *
 */
void delete_rpmsg_dev_list(acapd_list_t *rpmsg_dev_list)
{

	if (rpmsg_dev_list == NULL) {
		DFX_DBG("List is empty\n");
		return;
	}

	acapd_list_t *rpmsg_dev_node;

	/* traverse through the list */
	acapd_list_for_each(rpmsg_dev_list, rpmsg_dev_node) {
		rpmsg_dev_t *rpmsg_dev;

		rpmsg_dev = (rpmsg_dev_t *)acapd_container_of(rpmsg_dev_node, rpmsg_dev_t,
				rpmsg_node);
		/* delete node */
		acapd_list_del(&rpmsg_dev->rpmsg_node);
		free(rpmsg_dev);
	}
}

/**
 * update_rpmsg_dev_list() - updates rpmsg device list and setup
 * end point
 * @acapd_list_t *rpmsg_dev_list -- pointer to rpmsg_dev_list
 * @rpmsg_ctrl_dev -- rpmsg control dev
 * @virtio_num -- virtio number
 *
 * This function updates rpmsg_dev_list by parsing through /sys/bus/rpmsg/devices
 * directory and does the following:
 * 1) remove deleted rpmsg dev from list
 * 2) create new rpmsg_dev_t for new dev found
 *    a) setups up rpmsg end point dev for new rpmsg virtio dev and ctrl
 *    b) setup rpmsg_channel name
 * 3) add new entry to rpmsg dev list
 *
 * Return:  void
 *
 */
void update_rpmsg_dev_list(acapd_list_t *rpmsg_dev_list, char* rpmsg_ctrl_dev, int virtio_num)
{
	char dpath[] = "/sys/bus/rpmsg/devices";
	DIR *dir = opendir(dpath);
	struct dirent *ent;
	int rpmsg_dev_found = 0;
	int v_num;
	rpmsg_dev_t *rpmsg_dev;

	if (!dir) {
		DFX_DBG("opendir %s, %s\n", dpath, strerror(errno));
		return;
	}

	/*
	 * reset active flag for all in list
	 * this is needed as endpoints can be created and deleted at runtime
	 * - active flag will be set if dev is found
	 * - any device without active flag set will be deleted
	 * */
	reset_active(rpmsg_dev_list);

	while ((ent = readdir(dir)) != NULL) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") || strstr(ent->d_name, "rpmsg_ns")
				|| strstr(ent->d_name, "rpmsg_ctrl"))
			continue;    /* skip self, parent and ctrl dev */

		/* check if virtio num matches */
		v_num = get_virtio_number(ent->d_name);
		if (v_num != virtio_num)
			continue;

		/* check if dev already recorded and set active flag*/
		rpmsg_dev_found = check_rpmsg_dev_active(rpmsg_dev_list, ent->d_name, virtio_num);

		/* if device is recorded go to next file */
		if (rpmsg_dev_found == 1) {
			rpmsg_dev_found = 0;
			continue;
		}

		/* new virtio rpmsg dev found */

		/*
		 * create new rpmsg dev
		 *  - setup endpoint dev and rpmsg channel name
		 * */
		rpmsg_dev = create_rpmsg_dev(ent->d_name, rpmsg_ctrl_dev);

		/* new rpmsg device node created, insert to end of list */
		if ( rpmsg_dev != NULL)
			acapd_list_add_tail(rpmsg_dev_list, &rpmsg_dev->rpmsg_node);

	}

	/* delete all inactive node by checking active flag */
	delete_inactive_rpmsgdev(rpmsg_dev_list);

	return;
}
