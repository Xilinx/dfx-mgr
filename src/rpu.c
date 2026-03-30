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
#include <dfx-mgr/rpu.h>
#include <dfx-mgr/daemon_helper.h>
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
 * start_rpu_firmware() - Set and start firmware on an RPU slot
 * @fw_name: Firmware filename
 * @rpu_slot: RPU slot number (remoteproc index)
 *
 * Return: 0 on success, -1 on error
 */
static int start_rpu_firmware(const char *fw_name, int rpu_slot)
{
	char cmd[1024];
	int ret;

	snprintf(cmd, sizeof(cmd),
		 "echo %s > /sys/class/remoteproc/remoteproc%d/firmware",
		 fw_name, rpu_slot);
	ret = system(cmd);
	if (ret != 0) {
		DFX_ERR("Command not successful %s\n", cmd);
		return -1;
	}

	DFX_DBG("Starting RPU firmware %s in slot %d\n", fw_name, rpu_slot);
	snprintf(cmd, sizeof(cmd),
		 "echo start > /sys/class/remoteproc/remoteproc%d/state",
		 rpu_slot);
	ret = system(cmd);
	if (ret != 0) {
		DFX_ERR("Command not successful %s\n", cmd);
		return -1;
	}

	return 0;
}

/**
 * load_rpu() - load and start RPU firmware
 * @*rpu_path - location of the firmware
 * @rpu_slot - RPU number to load the firmware
 * @*firmware_name - optional specific firmware filename (for new structure)
 *                   NULL for old structure (loads all .elf files)
 *
 * This function does the following
 * 1 - Set the firmware location path
 * 2 - Load the firmware (specific file or all files based on firmware_name)
 * 3 - start the firmware
 *
 * Return: slot_number on success
 * 	   -1 on error
 */
int load_rpu(char *rpu_path, int rpu_slot, char *firmware_name)
{
	int ret;
	char cmd[1024];
	DIR *dir1 = NULL;
	int found_firmware = 0;
	struct dirent *d1;

	DFX_DBG("rpu_path %s rpu_slot %d firmware_name %s\n",
			rpu_path, rpu_slot, firmware_name ? firmware_name : "NULL");

	DFX_DBG("Setting firmware location to  base_path %s\n",rpu_path);
	snprintf(cmd, sizeof(cmd),
		 "echo -n %s > /sys/module/firmware_class/parameters/path", rpu_path);
	ret = system(cmd);
	if(ret != 0 ){
		DFX_ERR("Command not successful for rpu_path %s slot %d: %s",
			rpu_path, rpu_slot, cmd);
		return -1;
	}

	/* NEW structure: Load specific firmware file */
	if (firmware_name) {
		ret = start_rpu_firmware(firmware_name, rpu_slot);
		return (ret == 0) ? rpu_slot : -1;
	}

	/* OLD structure: Load all .elf files in directory */
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

		ret = start_rpu_firmware(d1->d_name, rpu_slot);
		if (ret != 0) {
			closedir(dir1);
			return -1;
		}
	}

	closedir(dir1);

	if (found_firmware == 1)
		return rpu_slot;

	DFX_ERR("No firmware found in %s", rpu_path);
	return -1;
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
		DFX_ERR("Command not successful for rpu %d: %s", rpu_num, cmd);
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


/**
 * validate_rpu_slot_availability() - Check if an RPU slot is valid and available
 * @base: Base design containing the RPU slots
 * @slot_num: RPU slot number to validate
 *
 * Return: 0 on success (slot is valid and available)
 *        -DFX_MGR_NO_EMPTY_SLOT_ERROR if slot is out of bounds or occupied
 */
int validate_rpu_slot_availability(struct basePLDesign *base, int slot_num)
{
	if (slot_num >= base->num_pl_slots) {
		DFX_ERR("Slot %d exceeds available slots", slot_num);
		return -DFX_MGR_NO_EMPTY_SLOT_ERROR;
	}

	if (base->slots[slot_num] != NULL) {
		DFX_ERR("Slot %d already occupied", slot_num);
		return -DFX_MGR_NO_EMPTY_SLOT_ERROR;
	}

	return 0;
}

/**
 * finalize_rpu_slot_setup() - Complete RPU slot setup after firmware load
 * @base: Base design containing the RPU
 * @slot: Slot structure to initialize
 * @pl_accel: Accelerator structure (can be NULL for RPU)
 * @accel_name: Name of the accelerator
 * @slot_num: Slot number where RPU was loaded
 * @rpu_fw_uptime_msec: Time in ms to wait for firmware initialization
 *
 * This handles all common post-load operations for both NEW and OLD RPU structures:
 * - Setting slot metadata
 * - Waiting for firmware initialization
 * - Discovering and storing rpmsg control device
 * - Initializing rpmsg device list
 * - Assigning slot to base design
 * - Allocating and assigning slot handle
 *
 * Return: slot_handle on success, -1 on error
 */
int finalize_rpu_slot_setup(struct basePLDesign *base,
			    slot_info_t *slot,
			    acapd_accel_t *pl_accel,
			    const char *accel_name,
			    int slot_num,
			    unsigned int rpu_fw_uptime_msec)
{
	char *rpmsg_ctrl_dev_name;

	/* Set slot metadata */
	snprintf(slot->name, sizeof(slot->name), "%s", accel_name);
	slot->is_aie = 0;
	slot->is_rpu = 1;

	/* Wait for firmware to be ready */
	DFX_DBG("RPU firmware uptime %d msec\n", rpu_fw_uptime_msec);
	usleep(rpu_fw_uptime_msec * 1000);

	/* Get new rpmsg ctrl device created by firmware */
	rpmsg_ctrl_dev_name = get_new_rpmsg_ctrl_dev(base);

	/* check if new ctrl dev is found
		* Assumption is that ctrl dev will be created
		* during load, dev can be dynamic.
		* if found then record the virtio number
		* if not found then 2 of the following are the cases
		* 1- no dev is created by firmware
		*    Here we just proceed, nothing to address
		* 2- Firmware is taking time to create the channel
		*    Increase rpu_fw_uptime_msec from config file
		* */
	if (rpmsg_ctrl_dev_name != NULL) {
		snprintf(slot->rpu.rpmsg_ctrl_dev_name,
			sizeof(slot->rpu.rpmsg_ctrl_dev_name),
			"%s", rpmsg_ctrl_dev_name);
		slot->rpu.virtio_num = get_virtio_number(rpmsg_ctrl_dev_name);
		DFX_PR("rpmsg_ctrl_dev %s virtio %d",
				slot->rpu.rpmsg_ctrl_dev_name, slot->rpu.virtio_num);
	} else {
		DFX_PR("No rpmsg control device found after rpu fw load");
	}

	/* Initialize rpmsg device list */
	acapd_list_init(&slot->rpu.rpmsg_dev_list);

	/* Assign slot to base design */
	slot->accel = pl_accel;
	base->slots[slot_num] = slot;

	/* Assign a free slot_handle to slot */
	base->slots[slot_num]->slot_handle = get_free_slot_handle();
	DFX_PR("Loaded %s successfully to slot %d with slot_handle %d",
			accel_name, slot_num, slot->slot_handle);

	return slot->slot_handle;
}

/*
 * parse_rpu_slot_dir() - Parse a single RPU slot directory for .elf files
 * @base: Pointer to base design structure
 * @slot_path: Path to slot directory (e.g., /lib/firmware/xilinx/<board>/rpu/0)
 * @slot_num: Slot number
 * @rpu_path: Parent RPU path
 *
 * Scans slot directory for .elf files and registers each as an accelerator
 */
void parse_rpu_slot_dir(struct basePLDesign *base, char *slot_path,
			int slot_num, char *rpu_path)
{
	DIR *elf_dir;
	struct dirent *elf_entry;
	char elf_name[NAME_MAX];
	accel_info_t *accel;

	elf_dir = opendir(slot_path);
	if (elf_dir == NULL) {
		DFX_ERR("Cannot open slot directory %s", slot_path);
		return;
	}

	while ((elf_entry = readdir(elf_dir)) != NULL) {
		const char *ext;

		if (!strcmp(elf_entry->d_name, ".") || !strcmp(elf_entry->d_name, ".."))
			continue;

		ext = strrchr(elf_entry->d_name, '.');
		if (!ext || strcmp(ext, RPU_FIRMWARE_EXT))
			continue;

		/* Copy name without .elf extension */
		snprintf(elf_name, sizeof(elf_name), "%.*s",
			 (int)(ext - elf_entry->d_name), elf_entry->d_name);

		accel = add_accel_to_base(base, elf_name, slot_path, rpu_path);
		if (accel) {
			strcpy(accel->accel_type, RPU_TYPE_STR);
			accel->rpu.slot_num = slot_num;
		}
	}
	closedir(elf_dir);
}
