/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (C) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/shell.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/json-config.h>
#include <dfx-mgr/daemon_helper.h>
#include <dfx-mgr/rpu.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>

struct daemon_config config;
struct watch *active_watch = NULL;
struct basePLDesign *base_designs = NULL;
static int inotifyFd;
platform_info_t platform;
sem_t mutex;
static void firmware_dir_walk(void);

struct watch {
    int wd;
	char name[64];
    char path[WATCH_PATH_LEN];
	char parent_name[64];
    char parent_path[WATCH_PATH_LEN];
};

/*
 * Filter: path a is over 64 char, "." and ".."
 */
static int
d_name_filter(char *a)
{
	return (strlen(a) > 64) ||
		(a[0] == '.' && (a[1] == 0 || (a[1] == '.' && a[2] == 0)));
}

static int
not_dir(char *path)
{
	struct stat sb;
	return stat(path, &sb) || !S_ISDIR(sb.st_mode);
}

struct basePLDesign *findBaseDesign(const char *name)
{
    int i,j;

    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] != '\0') {
            if(!strcmp(base_designs[i].name, name)) {
                    DFX_DBG("Found base design %s", base_designs[i].name);
                    return &base_designs[i];
	    }
            for (j = 0; j < RP_SLOTS_MAX; j++) {
                if(!strcmp(base_designs[i].accel_list[j].name, name)) {
                    DFX_DBG("accel %s in base %s", name, base_designs[i].name);
                    return &base_designs[i];
                }
            }
        }
    }
    DFX_ERR("No accel found for %s", name);
    return NULL;
}

struct basePLDesign *findBaseDesign_path(const char *path)
{
    int i,j;

    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] != '\0') {
            if(!strcmp(base_designs[i].base_path, path)) {
                    DFX_DBG("Found base design %s", base_designs[i].base_path);
                    return &base_designs[i];
	    }
            for (j = 0; j < RP_SLOTS_MAX; j++) {
                if(!strcmp(base_designs[i].accel_list[j].path, path)) {
                    DFX_DBG("accel %s in base %s", path, base_designs[i].base_path);
                    return &base_designs[i];
                }
            }
        }
    }
    DFX_ERR("No base design or accel found for %s", path);
    return NULL;
}

/*
 * getRMInfo is a function intended to be able to read the presently
 * loaded RMs. Rename to saveRMinfo()?
 */
void getRMInfo()
{
    FILE *fptr;
    int i;
    struct basePLDesign *base = platform.active_base;

    if(base == NULL || base->slots == NULL) {
        DFX_ERR("No design currently loaded");
        return;
    }

    fptr = fopen("/home/root/rm_info.txt","w");
    if (fptr == NULL) {
        DFX_ERR("Couldn't create /home/root/rm_info.txt");
        return;
    }

    for (i = 0; i < base->num_pl_slots; i++)
        fprintf(fptr, "%d %s\n", i, base->slots[i] ? "used" : "GREY");

    fclose(fptr);
}

/**
 * find_slot_from_handle() - returns the slot number
 * @*base - Pointer to a base design
 * @slot_handle - slot handle number
 *
 * This function returns the slot number for the given
 * slot handle of a base
 *
 * Return: slot number
 *         -1 for slot not found
 */
int find_slot_from_handle(struct basePLDesign *base, int slot_handle)
{
	int slot = -1;
	for(int i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++){
		if(base->slots[i]){
			if(slot_handle == base->slots[i]->slot_handle){
				slot = i;
				break;
			}
		}
	}
	DFX_DBG("slot %d slot_handle %d",slot,slot_handle);
	return slot;
}


/**
 * get_free_slot_handle() - get free slot handle
 *
 * This function returns a free slot handle from
 * available list
 *
 * Return: slot handle index on success
 *        -1 if no slots available
 */
int get_free_slot_handle()
{
	int slot_handle = -1;
	for (int index = 0; index < SLOT_HANDLE_MAX; index++){
		if(platform.available_slot_handle[index] == 0){
			platform.available_slot_handle[index] = 1;
			slot_handle = index;
			break;
		}
	}
	return slot_handle;
}


/*
 * Update Vitis AI Runtime (VART) env
 */
void update_env(char *path)
{
    DIR *FD;
    struct dirent *dir;
    int len, ret;
    char cmd[512];

    DFX_DBG("%s", path);
    FD = opendir(path);
    if (FD) {
        while ((dir = readdir(FD)) != NULL) {
            len = strlen(dir->d_name);
            if (len > 7) {
                if (!strcmp(dir->d_name + (len - 7), ".xclbin") ||
                        !strcmp(dir->d_name + (len - 7), ".XCLBIN")) {
		    snprintf(cmd,sizeof(cmd),"echo \"firmware: %s/%s\" > /etc/vart.conf",path,dir->d_name);
		    DFX_DBG("system %s", cmd);
                    ret = system(cmd);
                    if (ret)
			    DFX_ERR("%s", cmd);
                }
            }
        }
        closedir(FD);
    }
}

int load_accelerator(const char *accel_name)
{
    int i, ret;
    int rv = -DFX_MGR_LOAD_ERROR;
    char path[1024];
    char shell_path[600];
    acapd_accel_t *pl_accel = (acapd_accel_t *)calloc(1, sizeof(acapd_accel_t));
    acapd_accel_pkg_hd_t *pkg = (acapd_accel_pkg_hd_t *)calloc(1, sizeof(acapd_accel_pkg_hd_t));
    struct basePLDesign *base;
    slot_info_t *slot = (slot_info_t *)malloc(sizeof(slot_info_t));
    accel_info_t *accel_info = NULL;
    char *rpmsg_ctrl_dev_name = NULL;

    slot->accel = NULL;
    firmware_dir_walk();
    base = findBaseDesign(accel_name);
    if(base == NULL) {
        DFX_ERR("No package found for %s", accel_name);
        rv = -DFX_MGR_NO_PACKAGE_FOUND_ERROR;
        goto out;
    }
    sprintf(shell_path,"%s/shell.json",base->base_path);

    /* Flat shell design are treated as with one slot */
    if (!strcmp(base->type,"XRT_FLAT") || !strcmp(base->type,"PL_FLAT")) {
        if (base->slots == NULL)
            base->slots = calloc(base->num_pl_slots + base->num_aie_slots,
                                 sizeof(slot_info_t *));

        if(platform.active_base != NULL && platform.active_base->active > 0) {
            DFX_ERR("Remove previously loaded accelerator, no empty slot");
            rv = -DFX_MGR_NO_EMPTY_SLOT_ERROR;
            goto out;
        }
        sprintf(pkg->name,"%s",accel_name);
        pkg->path = base->base_path;
        pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
        init_accel(pl_accel, pkg);
        strncpy(pl_accel->sys_info.tmp_dir, pkg->path,
                sizeof(pl_accel->sys_info.tmp_dir) - 1);
        strcpy(pl_accel->type,"XRT_FLAT");
        DFX_PR("load flat shell from %s", pkg->path);
        ret = load_accel(pl_accel, shell_path, 0);
        if (ret < 0){
            DFX_ERR("load_accel %s", accel_name);
            base->active = 0;
            goto out;
        }
		pl_accel->rm_slot = 0;
        base->fpga_cfg_id = pl_accel->sys_info.fpga_cfg_id;
        base->active += 1;
		slot->accel = pl_accel;
		slot->is_aie = 0;
        base->slots[0] = slot;
        platform.active_base = base;

	/*
	 * For PL_FLAT only single slot is available
	 * assign free slot_handle to slot 0
	 */
	base->slots[0]->slot_handle = get_free_slot_handle();
	DFX_PR("Loaded %s successfully to slot 0 with slot_handle %d", accel_name,base->slots[0]->slot_handle);

        /* VART libary for SOM desings needs .xclbin path to be written to a file*/
		if(!strcmp(base->type,"XRT_FLAT"))
			update_env(pkg->path);
        return 0;
    }
    for (i = 0; i < RP_SLOTS_MAX; i++){
        if(!strcmp(base->accel_list[i].name, accel_name)) {
            accel_info = &base->accel_list[i];
            DFX_DBG("Found accel %s base %s parent %s", accel_info->path, base->base_path, accel_info->parent_path);
        }
    }
    /* For SIHA slotted architecture */
    if(!strcmp(base->type,"PL_DFX") || !strcmp(base->type,"RPU")) {

	    /* For base type PL_DFX active_base is used to store
	     * the current active PL base, below are checks for
	     * 1) Availability of slots
	     * 2) If the PL being loaded is of the active base
	     * 3) If base design needs to be loaded then its done
	     */
	    if(!strcmp(base->type,"PL_DFX")){
		    if (platform.active_base != NULL && !platform.active_base->active &&
				    strcmp(platform.active_base->base_path, accel_info->parent_path)) {
			    DFX_PR("All slots for base are empty, loading new base design");
			    remove_base(platform.active_base->fpga_cfg_id);
			    free(platform.active_base->slots);
			    platform.active_base->slots = NULL;
			    platform.active_base = NULL;
		    }
		    else if(platform.active_base != NULL && platform.active_base->active > 0 &&
				    strcmp(platform.active_base->base_path, accel_info->parent_path)) {
			    DFX_ERR("Active base design doesn't match this accel base");
			    goto out;
		    }
		    if (platform.active_base == NULL) {
			    sprintf(pkg->name,"%s",base->name);
			    pkg->path = base->base_path;
			    pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
			    if (base->load_base_design) {
				    init_accel(pl_accel, pkg);
				    strncpy(pl_accel->sys_info.tmp_dir, pkg->path,
						    sizeof(pl_accel->sys_info.tmp_dir) - 1);
				    strcpy(pl_accel->type,"PL_DFX");
				    DFX_PR("load from %s", pkg->path);
				    ret = load_accel(pl_accel, shell_path, 0);
				    if (ret < 0){
					    DFX_ERR("load_accel %s", accel_name);
					    base->active = 0;
					    goto out;
				    }
				    DFX_PR("Loaded %s successfully", base->name);
				    base->fpga_cfg_id = pl_accel->sys_info.fpga_cfg_id;
			    }
			    if (base->slots == NULL)
				    base->slots = calloc(base->num_pl_slots + base->num_aie_slots,
						    sizeof(slot_info_t *));
			    platform.active_base = base;
		    }
	    }
	    else if (!strcmp(base->type,"RPU")){

		    /* For base type RPU active_rpu_base is used to store
		     * the current active RPU base, below are checks for
		     * 1) Availability of slots
		     * 2) If the RPU being loaded is of the active RPU base
		     * base design loading is not supported in RPU case
		     */
		    if (platform.active_rpu_base != NULL && !platform.active_rpu_base->active &&
				    strcmp(platform.active_rpu_base->base_path, accel_info->parent_path)) {
			    DFX_PR("All slots for rpu base are empty, loading new base design");
			    remove_base(platform.active_rpu_base->fpga_cfg_id);
			    free(platform.active_rpu_base->slots);
			    platform.active_rpu_base->slots = NULL;
			    platform.active_rpu_base = NULL;
		    }
		    else if(platform.active_rpu_base != NULL && platform.active_rpu_base->active > 0 &&
				    strcmp(platform.active_rpu_base->base_path, accel_info->parent_path)) {
			    DFX_ERR("Active base design doesn't match this accel base");
			    goto out;
		    }
		    if (platform.active_rpu_base == NULL) {
			    if (base->slots == NULL)
				    base->slots = calloc(base->num_pl_slots + base->num_aie_slots,
						    sizeof(slot_info_t *));
			    platform.active_rpu_base = base;
		    }
	    }
	    /*
	     * Check if base UID matches the parent_uid in accel_info
	     */
	    DFX_DBG("base(%s).uid = %d does %s match accel(%s).pid = %d",
			    base->name, base->uid,
			    base->uid == accel_info->pid ? "" : "NOT",
			    accel_info->name, accel_info->pid);
	    for (i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++) {
		    DFX_DBG("Finding empty slot for %s i %d", accel_name, i);
		    if (base->slots[i] == NULL){
			    sprintf(path,"%s/%s_slot%d", accel_info->path, accel_info->name,i);
			    if (access(path,F_OK) != 0){
				    continue;
			    }

			    if (!strcmp(base->type,"RPU")) {

				    /*
				     * For basetype RPU
				     * Call load_rpu with path of firmware and rpu number(slot number)
				     * update slot with details
				     * increment active_rpu_base
				     */
				    ret = load_rpu(path,i);
				    if (ret < 0){
					    DFX_ERR("load_rpu %s failed", accel_name);
					    goto out;
				    }

				    strcpy(slot->name, accel_name);
                                    slot->is_aie = 0;
				    slot->is_rpu = 1;

				    /*
				     * wait for firmware to be up
				     * rpu firware uptime is set in
				     * json file : /etc/dfx-mgr/daemon.conf
				     * property  : rpu_fw_uptime_msec
				     * */
				    DFX_DBG("RPU firmware uptime %d msec\n",config.rpu_fw_uptime_msec);
				    usleep(config.rpu_fw_uptime_msec * 1000);

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
					    /* Store rpmsg control dev in slot */
					    memcpy(slot->rpu.rpmsg_ctrl_dev_name,rpmsg_ctrl_dev_name,strlen(rpmsg_ctrl_dev_name));
					    slot->rpu.rpmsg_ctrl_dev_name[strlen(rpmsg_ctrl_dev_name)] ='\0';

					    /* Get and store virtio number in slot */
					    slot->rpu.virtio_num = get_virtio_number(rpmsg_ctrl_dev_name);
					    DFX_PR("rpmsg_ctrl_dev %s virtio %d", slot->rpu.rpmsg_ctrl_dev_name ,slot->rpu.virtio_num);
				    } else {
					    DFX_PR("No rpmsg control device found after rpu fw load");
				    }

				    /* Initialize rpmsg dev list */
				    acapd_list_init(&slot->rpu.rpmsg_dev_list);

                                    slot->accel = pl_accel;
				    base->slots[i] = slot;
				    platform.active_rpu_base->active += 1;

				    /* For RPU assign a free slot_handle to slot */
				    base->slots[i]->slot_handle = get_free_slot_handle();
				    DFX_PR("Loaded %s successfully to slot %d with slot_handle %d", accel_name, i,slot->slot_handle);
                                    /* return slot_handle instead of slot number */
				    return slot->slot_handle;

                            }
			    else {
				    /*
				     * For basetype PL_DFX
				     */
				    strcpy(slot->name, accel_name);
				    if (!strcmp(accel_info->accel_type,"XRT_AIE_DFX")) {
					    DFX_ERR("%s: XRT_AIE_DFX unsupported", accel_name);
					    goto out;
				    }
				    slot->is_aie = 0;
				    strcpy(pkg->name, accel_name);
				    pkg->path = path;
				    pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
				    init_accel(pl_accel, pkg);
				    /* Set rm_slot before load_accel() so isolation for appropriate slot can be applied*/
				    pl_accel->rm_slot = i;
				    strcpy(pl_accel->type,accel_info->accel_type);
				    strncpy(pl_accel->sys_info.tmp_dir, pkg->path,
						    sizeof(pl_accel->sys_info.tmp_dir) - 1);
				    DFX_PR("load from %s", pkg->path);
				    ret = load_accel(pl_accel, shell_path, 0);
				    if (ret < 0){
					    DFX_ERR("load_accel %s", accel_name);
					    goto out;
				    }
				    platform.active_base->active += 1;
				    slot->accel = pl_accel;
				    base->slots[i] = slot;

				    /* For PL_DFX assign a free slot_handle to slot */
				    base->slots[i]->slot_handle = get_free_slot_handle();
				    DFX_PR("Loaded %s successfully to slot %d with slot_handle %d", pkg->name, i,slot->slot_handle);
                                    /* return slot_handle instead of slot number */
				    return slot->slot_handle;
			    }
		    }
	    }
	    if (i >= (base->num_pl_slots + base->num_aie_slots)) {
		    DFX_ERR("No empty slot for %s", accel_name);
		    rv = -DFX_MGR_NO_EMPTY_SLOT_ERROR;
	    }
    }
    else {
	    DFX_ERR("Check the supported type of base/accel");
    }
out:
    free(slot);
    free(pl_accel);
    free(pkg);
    return rv;
}

static int
remove_accel_base(void)
{
	struct basePLDesign *base = platform.active_base;
	int ret = -1;

	if (!base)
		DFX_PR("No base");
	else if (strcmp(base->type, "PL_DFX"))
		DFX_PR("Invalid base type: %s", base->type);
	else if (base->active)
		DFX_PR("Can't remove active base PL design: %s", base->name);
	else {
		DFX_PR("Unload base: %s", base->name);
		ret = remove_base(base->fpga_cfg_id);
		if (ret == 0) {
			free(base->slots);
			base->slots = NULL;
			platform.active_base = NULL;
		}
	}
	return ret;
}


/**
 * remove_accelerator() - remove accel or rpu from give slot handle
 * @slot_handle - slot handle number
 *
 * This function removes an accel/rpu firmware from provided slot_handle
 * the slot_handle is mapped to slots for each accel/rpu
 *
 * Return:  0 on success
 *         -1 on failure
 */
int remove_accelerator(int slot_handle)
{
	struct basePLDesign *base = platform.active_base;
        struct basePLDesign *rpu_base = platform.active_rpu_base; /* get active rpu base */
	acapd_accel_t *accel;
	int ret = -1;
	int slot = -1;

	/* slot -1 means remove base PL design */
	if (slot_handle == -1)
		return remove_accel_base();

	/* check if base for pl or rpu is active */
	if ((!base && !rpu_base) || slot_handle < 0 ) {
		DFX_ERR("No Accel or invalid slot %d", slot_handle);
		return -1;
	}

	/*
	 * if rpu base is active and slot_handle is found in active rpu base
	 * then remove the rpu firmware by getting the slot number mapped to
	 * slot_handle
	 */
	if(rpu_base){
		slot = find_slot_from_handle(rpu_base, slot_handle);
		if (slot != -1){
			DFX_DBG("Removing rpu %s from slot %d slot_handle %d", rpu_base->slots[slot]->name, slot,rpu_base->slots[slot]->slot_handle);
			ret = remove_rpu(slot);
			platform.available_slot_handle[slot_handle] = 0;
			/* delete rpmsg_dev_list from slot */
			delete_rpmsg_dev_list(&rpu_base->slots[slot]->rpu.rpmsg_dev_list);
			free(rpu_base->slots[slot]);
			rpu_base->slots[slot] = NULL;
			platform.active_rpu_base->active -= 1;
			return ret;

		}
	}

	/*
	 * if pl base is active and slot_handle is found in active pl base
	 * then remove_accel by getting the slot number mapped to slot_handle
	 */
	if (base) {
                slot = find_slot_from_handle(base, slot_handle);
                if (slot != -1){
                        if (base->slots[slot]->is_aie){
                                free(base->slots[slot]);
                                base->slots[slot] = NULL;
                                platform.active_base->active -= 1;
                                return 0;
                        }
                        accel = base->slots[slot]->accel;
                        DFX_PR("Removing accel %s from slot %d", accel->pkg->name, slot);

                        ret = remove_accel(accel, 0);
                        platform.available_slot_handle[slot_handle] = 0;
                        free(accel->pkg);
                        free(accel);
                        free(base->slots[slot]);
                        base->slots[slot] = NULL;
                        platform.active_base->active -= 1;
                }
        }

	return ret;
}

void sendBuff(uint64_t size)
{
    DFX_PR("buffer size %lu", size);
    //sendBuffer(size, socket_d);
}
void allocBuffer(uint64_t size)
{
    DFX_PR("buffer size %lu", size);
    allocateBuffer(size);
}
void freeBuff(uint64_t pa)
{
    DFX_DBG("free buffer PA %lu", pa);
    freeBuffer(pa);
}

int getFD(int slot_handle, char *name)
{
	struct basePLDesign *base = platform.active_base;
	int slot = -1;

	if(base == NULL || base->slots == NULL){
		DFX_ERR("No active design");
		return -1;
	}

	/* get the slot mapped to the slot_handle */
	slot = find_slot_from_handle(base, slot_handle);
	if (slot == -1){
		DFX_ERR("slot not found");
		return -1;
	}

	if ( slot >= base->num_pl_slots || !base->slots[slot])
		return -1;

	acapd_accel_t *accel = base->slots[slot]->accel;
	if (accel == NULL){
		DFX_ERR("No Accel in slot %d", slot);
		return -1;
	}
	for (int i = 0; i < accel->num_ip_devs; i++)
		if (strstr(accel->ip_dev[i].dev_name, name))
			return accel->ip_dev[i].id;

	// Check if the name matches one of the shell devices
	return dfx_shell_fd_by_name(name);
}

int dfx_getFDs(int slot, int *fd)
{
    struct basePLDesign *base = platform.active_base;
    if(base == NULL){
        DFX_ERR("No active design");
        return -1;
    }
    acapd_accel_t *accel = base->slots[slot]->accel;
    if (accel == NULL){
        DFX_ERR("No Accel in slot %d", slot);
        return -1;
    }
    fd[0] = accel->ip_dev[2*slot].id;
    fd[1] = accel->ip_dev[2*slot+1].id;
    DFX_PR("Daemon slot %d accel_config %d d_hls %d",
                    slot, fd[0], fd[1]);
    return 0;
}

void getShellFD()
{
    //get_shell_fd(socket_d);
}
void getClockFD()
{
    //get_shell_clock_fd(socket_d);
}

/*
 * list_accel_uio  list uio device name and its path
 *
 * Note: dev_name + path < 80 char, see: acapd_device_t
 * and device_name limits in json. Reduce buf sise by
 * one line (80 char) to prevent buffer overrun.
 */
void
list_accel_uio(int slot_handle, char *buf, size_t sz)
{
	struct basePLDesign *base = platform.active_base;
	struct basePLDesign *rpu_base = platform.active_rpu_base; /* get active rpu base */
	acapd_accel_t *accel;
	char *end = buf + sz - 80;
	char *p = buf;
	int i;
        int slot = -1;
	char *rpmsg_ctrl_dev;
        int virtio_num;
	acapd_list_t *rpmsg_dev_list;
	acapd_list_t *rpmsg_dev_node;

	/*Check if rpu is active in slot_handle*/
	if(rpu_base)
	{
		slot = find_slot_from_handle(rpu_base, slot_handle);
		if (slot != -1){
			/* get virtio number of slot */
			virtio_num = rpu_base->slots[slot]->rpu.virtio_num;

			/* get rpmgs ctrl device name */
			rpmsg_ctrl_dev = rpu_base->slots[slot]->rpu.rpmsg_ctrl_dev_name;

			/* get stored list */
			rpmsg_dev_list = &rpu_base->slots[slot]->rpu.rpmsg_dev_list;

			/* update rpmsg_dev_list
			 * This function updates rpmsg_dev_list by parsing through /sys/bus/rpmsg/devices
			 * directory and does the following:
			 * 1) remove deleted rpmsg dev from list
			 * 2) create new rpmsg_dev_t for new dev found
			 *    a) setups up rpmsg end point dev for new rpmsg virtio dev and ctrl
			 *    b) setup rpmsg_channel name
			 * 3) add new entry to rpmsg dev list
			 */
			update_rpmsg_dev_list(rpmsg_dev_list, rpmsg_ctrl_dev, virtio_num);

			/* traverse through list and return channel name and dev name*/
			acapd_list_for_each(rpmsg_dev_list, rpmsg_dev_node) {
				rpmsg_dev_t *rpmsg_dev;

				rpmsg_dev = (rpmsg_dev_t *)acapd_container_of(rpmsg_dev_node, rpmsg_dev_t,
						rpmsg_node);
				p += sprintf(p, "%-30s %-30s\n", rpmsg_dev->rpmsg_channel_name, rpmsg_dev->ept_rpmsg_dev_name);
			}

			return;
		}
	}

	/* if base is active get the slot from slot handle */
	if(base)
	{
		slot = find_slot_from_handle(base, slot_handle);
		if (slot == -1){
			DFX_ERR("slot not found");
			return;
		}
	}

	if(base == NULL || base->slots == NULL)
		p += sprintf(p, "No active design\n");
	else if ( slot >= base->num_pl_slots || !base->slots[slot])
		p += sprintf(p, "slot %d is not used or invalid\n", slot);
	else if ( (accel = base->slots[slot]->accel) == NULL)
		p += sprintf(p, "No Accel in slot %d\n", slot);
	if (p != buf)
		return;

	for (i = 0; i < accel->num_ip_devs && p < end; i++) {
		char *dname = accel->ip_dev[i].dev_name;
		if (dname)
			p += sprintf(p, "%-30s %s\n", dname,
					accel->ip_dev[i].path);
	}
	dfx_shell_uio_list(p, end - p);
}

char *
get_accel_uio_by_name(int slot_handle, const char *name)
{
	struct basePLDesign *base = platform.active_base;
	struct basePLDesign *rpu_base = platform.active_rpu_base; /* get active rpu base */
	acapd_accel_t *accel;
	int slot = -1;
	char *ept_dev = NULL;
	acapd_list_t *rpmsg_dev_list;
	acapd_list_t *rpmsg_dev_node;
	char *rpmsg_ctrl_dev;
	int virtio_num;

	if(rpu_base)
	{

		slot = find_slot_from_handle(rpu_base, slot_handle);
		if(slot != -1){
			/* get virtio number of slot */
			virtio_num = rpu_base->slots[slot]->rpu.virtio_num;

			/* get rpmgs ctrl device name */
			rpmsg_ctrl_dev = rpu_base->slots[slot]->rpu.rpmsg_ctrl_dev_name;

			/* get stored list */
			rpmsg_dev_list = &rpu_base->slots[slot]->rpu.rpmsg_dev_list;

			/* update rpmsg_dev_list
			 * This function updates rpmsg_dev_list by parsing through /sys/bus/rpmsg/devices
			 * directory and does the following:
			 * 1) remove deleted rpmsg dev from list
			 * 2) create new rpmsg_dev_t for new dev found
			 *    a) setups up rpmsg end point dev for new rpmsg virtio dev and ctrl
			 *    b) setup rpmsg_channel name
			 * 3) add new entry to rpmsg dev list
			 */
			update_rpmsg_dev_list(rpmsg_dev_list, rpmsg_ctrl_dev, virtio_num);

			/* traverse throught the list */
			acapd_list_for_each(rpmsg_dev_list, rpmsg_dev_node) {
				rpmsg_dev_t *rpmsg_dev;
				rpmsg_dev = (rpmsg_dev_t *)acapd_container_of(rpmsg_dev_node, rpmsg_dev_t,
						rpmsg_node);

				/* get end point dev for given rpmsg channel name */
				if((strlen(name) == strlen(rpmsg_dev->rpmsg_channel_name)) &&
						!strncmp(name, rpmsg_dev->rpmsg_channel_name, strlen(name))){
					ept_dev = rpmsg_dev->ept_rpmsg_dev_name;
					break;
				}
			}

			/* return endpoint dev name */
			return ept_dev;
		}
	}

        if(base)
        {
                slot = find_slot_from_handle(base, slot_handle);
                if (slot == -1){
                        DFX_ERR("slot not found");
                        return NULL;
                }
        }

	// NULL if no active design, slot is not used or invalid
	if(!base || !base->slots || slot >= base->num_pl_slots ||
	   !base->slots[slot])
		return NULL;

	accel = base->slots[slot]->accel;
	if (!accel)
		return NULL;

	for (int i = 0; i < accel->num_ip_devs; i++)
		if (strstr(accel->ip_dev[i].dev_name, name))
			return accel->ip_dev[i].path;

	// Check if the name matches one of the shell devices
	return dfx_shell_uio_by_name(name);
}

/*
 * the s2mm config offset is 0; mm2s - 0x10000
 * TBD: find a sutable header file for DM_*_OFFT and data_mover_cfg
 */
#define DM_S2MM_OFFT 0
#define DM_MM2S_OFFT 0x10000

struct data_mover_cfg {
	uint32_t dm_cr;      /**< 0: Control signals */
	uint32_t dm_gier;    /**< 4: Global Interrupt Enable Reg */
	uint32_t dm_iier;    /**< 8: IP Interrupt Enable Reg */
	uint32_t dm_iisr;    /**< c: IP Interrupt Status Reg */
	uint32_t dm_addr_lo; /**< 10: Data signal of mem_V[31:0] */
	uint32_t dm_addr_hi; /**< 14: Data signal of mem_V[63:31] */
	uint32_t dm_x18;     /**< 18: reserved */
	uint32_t dm_size_lo; /**< 1c: Data signal of size_V[31:0] */
	uint32_t dm_x20;     /**< 20: reserved */
	uint32_t dm_tid;     /**< 24: Data signal of tid_V[7:0] */
};

static int
dm_format(char *buf, struct data_mover_cfg *p_dm)
{
	struct basePLDesign *base = platform.active_base;
	uint64_t addr = p_dm->dm_addr_hi;
	int s, slot = '=';

	addr = addr << 32 | p_dm->dm_addr_lo;
	for (s = 0; s < base->num_pl_slots; s++)
		if (base->inter_rp_comm[s] == addr) {
			slot = s + '0';
			break;
		}

	return sprintf(buf, slot == '='
		       ? "  memory_addr%c%#12lx sz=%#8x\n"
		       : " DataMover[%c]=%#12lx sz=%#8x\n",
		       slot, addr, p_dm->dm_size_lo);
}

#define DMCFG(p, f) (*(uint32_t *)((p) + offsetof(struct data_mover_cfg, f)))
/**
 * siha_ir_buf_list - list DMs configuration
 * @sz:  the size of the buf
 * @buf: buffer to put the DMs configuration
 *
 * Some of the bits in data_mover_cfg are Clear-On-Read, e.g.:
 *	dm_cr:1 (bit 1)
 * Do not read all of the struct data_mover_cfg as in
 *	memcpy(&dm_cfg, reg + DM_MM2S_OFFT, sizeof(dm_cfg));
 * This causes user application to hang.
 *
 * Returns: 0 on error or the size of the buffer to send to the client
 */
int
siha_ir_buf_list(uint32_t sz, char *buf)
{
	struct basePLDesign *base = platform.active_base;
	struct data_mover_cfg dm_cfg;
	char *p = buf;
	uint8_t slot;

	if (sz > (1<<15) || !buf) {
		/* the sz should be < struct_message.data[32*1024] */
		DFX_ERR("sz,buf = %u,%p", sz, buf);
		return 0;
	}
	if (!base || !base->slots || strcmp(base->type, "PL_DFX")) {
		p += sprintf(p, "No base, slots, or not a PL_DFX base: %s",
				base ? base->name : "no_base");
		return (p > buf) ? p + 1 - buf : 0;
	}
	memset(&dm_cfg, 0, sizeof(dm_cfg));
	for (slot = 0;
		slot < base->num_pl_slots
		&& base->slots[slot]
		&& base->slots[slot]->accel
		&& (p < buf + sz);
	     slot++) {
		acapd_accel_t *accel = base->slots[slot]->accel;
		void *reg = acapd_accel_get_reg_va(accel, "rm_comm_box");

		p += sprintf(p, "DataMover[%hhu]: %p\n", slot, reg);
		/* dm_cr:1 (bit 1) is Clear-on-Read: do not read it */
		if (reg) {
			uint64_t cr = (uint64_t)reg + DM_MM2S_OFFT;

			dm_cfg.dm_addr_lo = DMCFG(cr, dm_addr_lo);
			dm_cfg.dm_addr_hi = DMCFG(cr, dm_addr_hi);
			dm_cfg.dm_size_lo = DMCFG(cr, dm_size_lo);
			p += sprintf(p, " DataToAccel[%hhu]  ", slot);
			p += dm_format(p, &dm_cfg);
			cr = (uint64_t)reg + DM_S2MM_OFFT;
			dm_cfg.dm_addr_lo = DMCFG(cr, dm_addr_lo);
			dm_cfg.dm_addr_hi = DMCFG(cr, dm_addr_hi);
			dm_cfg.dm_size_lo = DMCFG(cr, dm_size_lo);
			p += sprintf(p, " DataFromAccel[%hhu]", slot);
			p += dm_format(p, &dm_cfg);
		}
	}
	return (p > buf) ? p + 1 - buf : 0;
}

#ifndef BIT
#define BIT(N) (1U << (N))
#endif

/**
 * siha_ir_buf - src outputs to the slot dst IR-buffer
 * @src: src slot writes  to dst IR-buffer
 * @des: dst slot reads from dst IR-buffer
 * @clear: clear DM_MM2S_OFFT src if BIT(0) is set
 * 	   clear DM_S2MM_OFFT dst if BIT(1) is set
 *
 * Inter-RP buffer addrs are from shell.json. See rp_comms_interconnect
 *
 * Returns: 0 if connected successfully; non-0 otherwise
 */
static int
siha_ir_buf(acapd_accel_t *src, acapd_accel_t *dst, uint8_t clear)
{
	void *src_dm = acapd_accel_get_reg_va(src, "rm_comm_box");
	void *dst_dm = acapd_accel_get_reg_va(dst, "rm_comm_box");
	struct basePLDesign *base = platform.active_base;
	struct data_mover_cfg *p_src, *q_dst;
	uint32_t ir_buf_lo, ir_buf_hi;
	int dst_slot = dst->rm_slot;

	if (dst_slot < 0 || dst_slot > base->num_pl_slots - 1) {
		DFX_ERR("dst_slot=%d", dst_slot);
		return -1;
	}
	if (!src_dm || !dst_dm) {
		DFX_ERR("rm_comm_box src,dst = %p,%p", src_dm, dst_dm);
		return -1;
	}
	ir_buf_lo = base->inter_rp_comm[dst_slot] & 0xFFFFFFFF;
	ir_buf_hi = base->inter_rp_comm[dst_slot] >> 32;
	DFX_DBG("src,dst=%u,%u addr=%p", src->rm_slot, dst_slot,
		(void *)base->inter_rp_comm[dst_slot]);

	/* set s2mm in src_dm to write to dst IR-buf */
	/* set mm2s in dst_dm to read from its own IR-buf */
	p_src = (struct data_mover_cfg *)(src_dm + DM_S2MM_OFFT);
	q_dst = (struct data_mover_cfg *)(dst_dm + DM_MM2S_OFFT);

	p_src->dm_addr_lo = q_dst->dm_addr_lo = ir_buf_lo;
	p_src->dm_addr_hi = q_dst->dm_addr_hi = ir_buf_hi;

	if (clear & BIT(0)) {
		p_src = (struct data_mover_cfg *)(src_dm + DM_MM2S_OFFT);
		p_src->dm_addr_lo = p_src->dm_addr_hi = 0;
	}
	if (clear & BIT(1)) {
		q_dst = (struct data_mover_cfg *)(dst_dm + DM_S2MM_OFFT);
		q_dst->dm_addr_lo = q_dst->dm_addr_hi = 0;
	}

	/*
	 * The app should set the size and the ap_start bit:
	 * p_src->dm_size_lo = q_dst->dm_size_lo = input_size;
	 * q_dst->dm_cr = 1;
	 * p_src->dm_cr = 1;
	 */
	return 0;
}

/**
 * slot_seq_init copy only 0-9 into slot_seq (filter delimiters)
 */
static int
slot_seq_init(char *slot_seq, char *buf)
{
	int num_pl_slots = platform.active_base->num_pl_slots;
	struct basePLDesign *base = platform.active_base;
	uint32_t no_dup = 0;
	int i, sz;
	int slot=-1;

	for (sz = i = 0; i < RP_SLOTS_MAX; i++) {
		int c = buf[i] - '0';

		if (c >= 0 && c <= num_pl_slots) {
			if (no_dup & BIT(c)) {
				DFX_ERR("repeated slot %d in %s", c, buf);
				return -1;
			}
			no_dup |= BIT(c);

			/* get the slot number from slot handle */
			slot = find_slot_from_handle(base, c);
			slot_seq[sz++] = slot;
		}
	}
	return sz;
}

/**
 * siha_ir_buf_set - Set up Inter-RM buffers for I/O between slots
 * @slot_seq:  user input array of slot IDs to connect
 *
 * Check if the slot_seq is valid, i.e.:
 *  - 1 < sz < base->num_pl_slots
 *  - it's a permutation w/o repetitions. Same as in nPk - "n permute k".
 *  - the requested slots are loaded
 *  - clear the first slot's DM_MM2S and the last slot's DM_S2MM
 *
 * Returns: 0 if connected successfully; non-0 otherwise
 */
int
siha_ir_buf_set(char *user_slot_seq)
{
	struct basePLDesign *base = platform.active_base;
	// acapd_accel_t *accel_src, *accel_dst;
	uint8_t slot0, slot, clear = 1;
	char slot_seq[RP_SLOTS_MAX];
	int i, sz;

	// NULL if no active design, slot is not used or invalid
	if (!base || !base->slots || strcmp(base->type, "PL_DFX")) {
		DFX_ERR("No base, slots, or not a PL_DFX base: %s",
			base ? base->name : "no_base");
		return -1;
	}

	sz = slot_seq_init(slot_seq, user_slot_seq);
	if (sz < 2 || sz > base->num_pl_slots) {
		DFX_ERR("invalid sz: %d or sequence: %s", sz, user_slot_seq);
		return -1;
	}

	for (slot0 = 0xff, slot = i = 0; i < sz; i++) {
		slot = slot_seq[i];
		DFX_DBG("s,d=%d,%d", (int)slot0, (int)slot);
		if (slot > base->num_pl_slots || !base->slots[slot]) {
			DFX_ERR("No accel in slot %u", slot);
			return -1;
		}
		if (i == sz - 1)
			clear |= BIT(1);

		if (slot0 != 0xff && slot0 != slot) {
			int rc = siha_ir_buf(base->slots[slot0]->accel,
					     base->slots[slot]->accel,
					     clear);
			clear = 0;
			if (rc) {
				DFX_ERR("slot: %u,%u", slot0, slot);
				return -1;
			}
		}
		slot0 = slot;
	}
	return 0;
}

/*
 * pid_uid_check - compare PID with base UID
 * @base: base to get its uid
 * @accel_idx: index of the accel: its pid should match uid of the base
 *
 * Returns:
 *	"id_ok"  - when PID and UID are present and match, or *FLAT shells
 *	"id_err" - when PID and UID are present, but do not match
 *	"no_id"  - when either PID or UID are not present.
 */
static const char *
pid_uid_check(struct basePLDesign *base, int accel_idx)
{
	int base_uid = base->uid;
	int pid = base->accel_list[accel_idx].pid;
	static const char p_noid[] =	"no_id";
	static const char p_err[] =	"id_err";
	static const char p_ok[] =	"id_ok";
	const char *str = p_noid;

	if (!strcmp(base->type, "XRT_FLAT") || !strcmp(base->type, "PL_FLAT"))
		str = p_ok;
	else if (base_uid && pid)
		str = (base_uid == pid) ? p_ok : p_err;

	return str;
}

char *listAccelerators()
{
    int i,j;
	uint8_t slot;
    char msg[330];	/* compiler warning if 326 bytes or less */
	char res[8*1024];
    char show_slots[16];
    const char format[] = "%30s%12s%24s%7s%12s%20s%16s\n";

	memset(res,0, sizeof(res));
	firmware_dir_walk();
 
    sprintf(msg, format, "Accelerator", "Accel_type", "Base", "Pid",
	    "Base_type", "#slots(RPU+PL+AIE)", "slot->handle");
	strcat(res,msg);
    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] != '\0' && base_designs[i].num_pl_slots > 0) {
            for (j = 0; j < RP_SLOTS_MAX; j++) {
                if (base_designs[i].accel_list[j].path[0] != '\0') {
                    char active_slots[16] = "";

		    /*
		     * For RPU num_pl_slots is used for RPU slot number
		     * For PL  num_pl_slots is used for PL slot number
		     */
		    if (!strcmp(base_designs[i].type, "RPU") ){
			    sprintf(show_slots, "(%d+0+0)",
					    base_designs[i].num_pl_slots);
                    }else {
			    /*
			     * Internally flat shell is treated as one slot to make
			     * the code generic and save info of the active design
			     */
			    sprintf(show_slots, "(0+%d+%d)",
					    !strcmp(base_designs[i].type, "XRT_FLAT") ||
					    !strcmp(base_designs[i].type, "PL_FLAT")
					    ? 0 : base_designs[i].num_pl_slots,
					    base_designs[i].num_aie_slots);
		    }

                    if (base_designs[i].active) {
                        char tmp[10];
                        for(slot = 0; slot < (base_designs[i].num_pl_slots +  base_designs[i].num_aie_slots); slot++) {
                           if (base_designs[i].slots[slot] != NULL &&
					   (base_designs[i].slots[slot]->is_aie || base_designs[i].slots[slot]->is_rpu) &&
					   !strcmp(base_designs[i].slots[slot]->name, base_designs[i].accel_list[j].name)) {
				    sprintf(tmp,"%d->%d,",slot, base_designs[i].slots[slot]->slot_handle);
                                    strcat(active_slots,tmp);
                            }
                            else if (base_designs[i].slots[slot] != NULL && !base_designs[i].slots[slot]->is_aie &&
                                     !base_designs[i].slots[slot]->is_rpu &&
				     !strcmp(base_designs[i].slots[slot]->accel->pkg->name, base_designs[i].accel_list[j].name)) {
				    sprintf(tmp,"%d->%d,",slot, base_designs[i].slots[slot]->slot_handle);
                                    strcat(active_slots,tmp);
                            }
                        }
                    }
                    sprintf(msg, format,
                            base_designs[i].accel_list[j].name,
                            base_designs[i].accel_list[j].accel_type,
                            base_designs[i].name,
                            pid_uid_check(&base_designs[i], j),
                            base_designs[i].type,
                            show_slots,
                            active_slots[0] ? active_slots : "-1");
                    strcat(res, msg);
                }
            }
        }
    }

    return strdup(res);
}

void add_to_watch(int wd, char *name, char *path, char *parent_name, char *parent_path)
{
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (active_watch[i].wd == -1) {
            active_watch[i].wd = wd;
            strncpy(active_watch[i].name, name, 64 -1);
            strncpy(active_watch[i].path, path, WATCH_PATH_LEN -1);
            strncpy(active_watch[i].parent_name, parent_name, 64 -1);
            strncpy(active_watch[i].parent_path, parent_path, WATCH_PATH_LEN -1);
            return;
        }
    }
    DFX_ERR("no room to add more watch");
}

struct watch *get_watch(int wd)
{
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (active_watch[i].wd == wd)
            return &active_watch[i];
    }
    return NULL;
}
char * wd_to_pathname(int wd)
{
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (active_watch[i].wd == wd)
            return active_watch[i].path;
    }
    return NULL;
}
struct watch* path_to_watch(char *path)
{
	int i;
	for(i=0; i < MAX_WATCH; i++){
		if(!strcmp(path,active_watch[i].path))
			return &active_watch[i];
	}
	return NULL;
}
void remove_watch(char *path)
{
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (strcmp(active_watch[i].path, path) == 0){
			inotify_rm_watch(inotifyFd, active_watch[i].wd);
			active_watch[i].wd = -1;
			active_watch[i].path[0] = '\0';
        }
    }
}
accel_info_t *add_accel_to_base(struct basePLDesign *base, char *name, char *path, char *parent_path)
{
    int j;

    for (j = 0; j < RP_SLOTS_MAX; j++) {
        if (!strcmp(base->accel_list[j].path, path)) {
			DFX_DBG("%s already exists", path);
            break;
        }
        if (base->accel_list[j].path[0] == '\0') {
			DFX_DBG("adding %s to base %s", path, parent_path);
            strcpy(base->accel_list[j].name, name);
            base->accel_list[j].name[sizeof(base->accel_list[j].name) - 1] = '\0';
            strcpy(base->accel_list[j].path, path);
            base->accel_list[j].path[sizeof(base->accel_list[j].path) - 1] = '\0';
            strcpy(base->accel_list[j].parent_path, parent_path);
            base->accel_list[j].parent_path[sizeof(base->accel_list[j].parent_path) - 1] = '\0';
            //base->accel_list[j].wd = wd;
	    break;
        }
    }
	return &base->accel_list[j];
}


/*
 * The present limitation is only one level of hierarchy (dir1 and dir2).
 * In future when we face hierarchical designs that may have RPs within
 * RMs we will need this scalability.
 */
void parse_packages(struct basePLDesign *base,char *fname, char *path)
{
    DIR *dir1 = NULL, *dir2 = NULL;
    struct dirent *d1,*d2;
    char first_level[512],second_level[800],filename[811];
	struct stat stat_info;
	accel_info_t *accel;
    int wd;

	/* For flat shell design there is no subfolder so assign the base path as the accel path */
	if (!strcmp(base->type,"XRT_FLAT") || !strcmp(base->type,"PL_FLAT")) {
		DFX_DBG("%s : %s", base->name, base->type);
        strcpy(base->accel_list[0].name, base->name);
        strcpy(base->accel_list[0].path, base->base_path);
        strcpy(base->accel_list[0].parent_path, base->base_path);
        strcpy(base->accel_list[0].accel_type, base->type);
		return;
	}

    dir1 = opendir(path);
    if (dir1 == NULL) {
        DFX_ERR("Directory %s not found", path);
		return;
    }
    while((d1 = readdir(dir1)) != NULL) {
	    sprintf(first_level, "%s/%s", path, d1->d_name);
	    if (d_name_filter(d1->d_name) || not_dir(first_level))
		    continue;

			//add_accel_to_base(dir->d_name, new_dir, path);
            wd = inotify_add_watch(inotifyFd, first_level, IN_ALL_EVENTS);
            if (wd == -1){
                DFX_ERR("inotify_add_watch failed on %s", first_level);
				goto close_dir;
			}
            
			add_to_watch(wd, d1->d_name, first_level, fname, path);
			sprintf(filename,"%s/accel.json",first_level);
			/* For pl slots we need to traverse next level to find accel.json*/
			if (stat(filename,&stat_info)){

				dir2 = opendir(first_level);
				while((d2 = readdir(dir2)) != NULL) {
					sprintf(second_level,"%s/%s", first_level, d2->d_name);
					if (d_name_filter(d2->d_name) || not_dir(second_level))
						continue;

					wd = inotify_add_watch(inotifyFd, second_level, IN_ALL_EVENTS);
					if (wd == -1){
						DFX_ERR("inotify_add_watch failed on %s", second_level);
						goto close_dir;
					}
					add_to_watch(wd, d2->d_name, second_level, d1->d_name, first_level);
					sprintf(filename,"%s/accel.json",second_level);
					if (!stat(filename,&stat_info)){
					accel = add_accel_to_base(base,d1->d_name, first_level, path);
					initAccel(accel, second_level);
					}
				}
				/* accel.json file not there for RPU
				 * Check if base-type is "RPU"
				 */
				if (!strcmp(base->type,"RPU")) {
					accel = add_accel_to_base(base,d1->d_name, first_level, path);
					/* accel_type is set inside initAccel in the case of PL
					 * by reading the accel.json file, since for RPU we do not
					 * have accel.json file, we set the accel_type to RPU here
					 */
					strcpy(accel->accel_type, base->type);
				}
			}
				/* Found accel.json so add it*/
			else {
				accel = add_accel_to_base(base,d1->d_name, first_level, path);
				initAccel(accel, first_level);
            }
    }
close_dir:
    if (dir1)
	    closedir(dir1);
    if (dir2)
	    closedir(dir2);
}

void add_base_design(char *name, char *path, char *parent, int wd)
{
    int i;

    for (i = 0; i < MAX_WATCH; i++) {
	if (!strcmp(base_designs[i].base_path, path)){
		DFX_DBG("Base design %s already exists", path);
		return;
	}
    }

	// Now find the fist unsued base_designs[] element
    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] == '\0') {
            DFX_DBG("Adding base design %s", path);
            strncpy(base_designs[i].name, name, sizeof(base_designs[i].name)-1);
            base_designs[i].name[sizeof(base_designs[i].name) - 1] = '\0';
            strncpy(base_designs[i].base_path, path, sizeof(base_designs[i].base_path)-1);
            base_designs[i].base_path[sizeof(base_designs[i].base_path) - 1] = '\0';
            strncpy(base_designs[i].parent_path, parent, sizeof(base_designs[i].parent_path)-1);
            base_designs[i].parent_path[sizeof(base_designs[i].parent_path) - 1] = '\0';
            base_designs[i].active = 0;
            base_designs[i].wd = wd;
            return;
        }
    }
}
void remove_base_design(char *path,char *parent, int is_base)
{
    int i, j;

    if (!is_base) {
        DFX_DBG("Removing accel %s from base %s", path, parent);
        for (i = 0; i < MAX_WATCH; i++) {
            if (!strcmp(base_designs[i].base_path,parent)) {
		for (j = 0; j < RP_SLOTS_MAX; j++) {
		    if (!strcmp(base_designs[i].accel_list[j].path,path)) {
                        remove_watch(path);
                        base_designs[i].accel_list[j].name[0] = '\0';
                        base_designs[i].accel_list[j].path[0] = '\0';
                        base_designs[i].accel_list[j].parent_path[0] = '\0';
                        base_designs[i].accel_list[j].wd = -1;
                        break;
                    }
                }
            }
        }
        return;
    }
    for (i = 0; i < MAX_WATCH; i++) {
        if (strcmp(base_designs[i].base_path, path) == 0) {
            DFX_DBG("Removing base design %s", path);
	    for (j = 0; j < RP_SLOTS_MAX; j++) {
                if (base_designs[i].accel_list[j].path[0] != '\0') {
                    remove_watch(path);
                    base_designs[i].accel_list[j].name[0] = '\0';
                    base_designs[i].accel_list[j].path[0] = '\0';
                    base_designs[i].accel_list[j].parent_path[0] = '\0';
                    base_designs[i].accel_list[j].wd = -1;
                    break;
                }
            }
            base_designs[i].name[0] = '\0';
            base_designs[i].type[0] = '\0';
            base_designs[i].base_path[0] = '\0';
            base_designs[i].num_pl_slots = 0;
            //free(base_designs[i].slots);
            base_designs[i].active = 0;
            base_designs[i].wd = -1;
            return;
        }
    }
}

/*static void
displayInotifyEvent(struct inotify_event *i)
{
     printf(" event name = %s\n", i->name);
    //printf("    wd =%2d; ", i->wd);
   // if (i->cookie > 0)
      //  printf("cookie =%4d; ", i->cookie);

//    printf("mask = ");
    if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
    if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
    if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
    if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
    if (i->mask & IN_CREATE)        printf("IN_CREATE ");
    if (i->mask & IN_DELETE)        printf("IN_DELETE ");
    if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
    if (i->mask & IN_IGNORED)       printf("IN_IGNORED ");
    if (i->mask & IN_ISDIR)         printf("IN_ISDIR ");
    if (i->mask & IN_MODIFY)        printf("IN_MODIFY ");
    if (i->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
    if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
    if (i->mask & IN_MOVED_TO)      printf("IN_MOVED_TO ");
    if (i->mask & IN_OPEN)          printf("IN_OPEN ");
    if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
    if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
    printf("\n");


//   if (i->len > 0){}
//        printf("        name = %s\n", i->name);
}*/

static void
acell_dir_add(char *cpath, struct dirent *dirent)
{
	char new_dir[512], fname[600];
	struct basePLDesign *base;
	char *d_name;
	int wd;

	if (!cpath || !dirent || d_name_filter(dirent->d_name))
		return;

	d_name = dirent->d_name;
	sprintf(new_dir, "%s/%s", cpath, d_name);
	if (not_dir(new_dir))
		return;

	DFX_DBG("Found dir %s", new_dir);
	wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
	if (wd == -1){
		DFX_ERR("inotify_add_watch %s", new_dir);
		return;
	}
	add_to_watch(wd, d_name, new_dir, "", cpath);
	add_base_design(d_name, new_dir, cpath, wd);
	base = findBaseDesign(d_name);
	if (base == NULL)
		return;

	sprintf(fname, "%s/shell.json", base->base_path);
	if (initBaseDesign(base, fname) == 0)
		parse_packages(base, base->name, base->base_path);
}

/*
 * firmware_dir_walk()
 * For each config.firmware_locations, add a watch for all events
 */
static void
firmware_dir_walk(void)
{
	int k, wd;
	struct dirent *dirent;

	for(k = 0; k < config.number_locations; k++) {
		char *fwdir = config.firmware_locations[k];
		DIR *d = opendir(fwdir);

		if (d == NULL) {
			DFX_ERR("opendir(%s)", fwdir);
			continue;
		}
		if (!path_to_watch(fwdir)) {
			wd = inotify_add_watch(inotifyFd, fwdir, IN_ALL_EVENTS);
			if (wd == -1) {
				DFX_ERR("inotify_add_watch(%s)", fwdir);
				closedir(d);
				continue;
			}
			add_to_watch(wd, "", fwdir, "", "");
		}
		/* Add packages to inotify */
		while ((dirent = readdir(d)) != NULL)
			acell_dir_add(fwdir, dirent);
		closedir(d);
	}
}

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
void *threadFunc(void *)
{
    int wd, j, ret;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p;
    char new_dir[512],fname[600];
    struct inotify_event *event;
    struct basePLDesign *base;

    active_watch = (struct watch *)calloc(MAX_WATCH, sizeof(struct watch));
    base_designs = (struct basePLDesign *)calloc(MAX_WATCH, sizeof(struct basePLDesign));

    for(j = 0; j < MAX_WATCH; j++) {
        active_watch[j].wd = -1;
    }

    inotifyFd = inotify_init();                 /* Create inotify instance */
    if (inotifyFd == -1)
        DFX_ERR("inotify_init");

    firmware_dir_walk();
	/* Done parsing on target accelerators, now load a default one if present in config file */
	sem_post(&mutex);

    /* Listen for new updates in firmware path*/
    for (;;) {
        numRead = read(inotifyFd, buf, BUF_LEN);
        if (numRead <= 0 && errno != EAGAIN)
            DFX_ERR("read() from inotify failed");

        /* Process all of the events in buffer returned by read() */

        for (p = buf; p < buf + numRead; ) {
            event = (struct inotify_event *) p;
           // displayInotifyEvent(event);
            if(event->mask & IN_CREATE || event->mask & IN_CLOSE_WRITE){
                if (event->mask & IN_ISDIR) {
                    struct watch *w = get_watch(event->wd);
                    if (w == NULL || strstr(event->name, ".dpkg-new"))
                        break;
                    sprintf(new_dir,"%s/%s",w->path, event->name);
                    DFX_DBG("add inotify watch on %s w->name %s parent %s", new_dir, w->name, w->parent_path);
                    wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
                    if (wd == -1)
                        DFX_PR("inotify_add_watch failed on %s", new_dir);
					add_to_watch(wd, event->name, new_dir, w->name,w->path);
					if (!strcmp(w->parent_path,""))
						add_base_design(event->name, new_dir, w->path, wd);
                }
				else if(!strcmp(event->name,"shell.json")) {
					struct watch *w = get_watch(event->wd);
					if (w == NULL)
						break;
					base = findBaseDesign_path(w->path);
					if (base == NULL)
						break;
					sprintf(fname,"%s/%s",w->path,"shell.json");
					ret = initBaseDesign(base, fname);
					if (ret == 0)
						parse_packages(base,w->name, w->path);
				}
				else if(!strcmp(event->name,"accel.json")) {
					struct watch *w = get_watch(event->wd);
					struct watch *parent_watch = path_to_watch(w->parent_path);
					accel_info_t *accel;

					base = findBaseDesign_path(parent_watch->parent_path);
					DFX_DBG("Add accel %s to base %s", w->path, base->base_path);
					accel = add_accel_to_base(base, w->parent_name, w->parent_path, base->base_path);
					initAccel(accel, w->path);
				}
            }
            else if((event->mask & IN_DELETE) && (event->mask & IN_ISDIR)) {
				struct watch *w = get_watch(event->wd);
                if (w == NULL)
                    break;
                sprintf(new_dir,"%s/%s",w->path, event->name);
                DFX_DBG("Removing watch on %s parent_path %s", new_dir, w->parent_path);
				if (!strcmp(w->parent_path,""))
					remove_base_design(new_dir, w->path, 1);
				else
					remove_base_design(new_dir, w->path, 0);
                remove_watch(new_dir);
            }
            else if((event->mask & IN_MOVED_FROM) && (event->mask & IN_ISDIR)) {
				struct watch *w = get_watch(event->wd);
                if (w == NULL)
                    break;
                sprintf(new_dir,"%s/%s",w->path, event->name);
				if (!strcmp(w->parent_name,""))
					remove_base_design(new_dir, w->path, 1);
				else
					remove_base_design(new_dir, w->path, 0);
            }
            else if(event->mask & IN_MOVED_TO){
				/*
				 * 'dnf install' creates tmp filenames and then does 'mv' to desired filenames.
				 * Hence IN_CREATE notif will be on tmp filenames, add logic in MOVED_TO notification for
				 * shell.json
				 */
				if(event->mask & IN_ISDIR || !strcmp(event->name,"shell.json")){
					struct watch *w = get_watch(event->wd);
                    if (w == NULL || strstr(event->name, ".dpkg-new"))
						break;
					if (event->mask & IN_ISDIR){
						sprintf(new_dir,"%s/%s",w->path, event->name);
						add_base_design(event->name, new_dir, w->path, event->wd);
					} else {
						sprintf(new_dir,"%s",w->path);
					}
					base = findBaseDesign_path(new_dir);
					if (base == NULL)
						break;
					sprintf(fname,"%s/%s",new_dir,"shell.json");
					ret = initBaseDesign(base, fname);
					if (ret == 0)
						parse_packages(base,w->name, new_dir);
				}
            }
            p += sizeof(struct inotify_event) + event->len;
        }
    }
    //exit(EXIT_SUCCESS);
}

int dfx_init()
{
	pthread_t t;

	strcpy(config.defaul_accel_name, "");
	strcpy(platform.boardName,"Xilinx board");
	sem_init(&mutex, 0, 0);

	parse_config(CONFIG_PATH, &config);
	pthread_create(&t, NULL,threadFunc, NULL);
	sem_wait(&mutex);
	//TODO Save active design on filesytem and on reboot read that
	//if (stat("/configfs/device-tree/overlays",&info))
	//	ret = system("rmdir /configfs/device-tree/overlays/*");

	return 0;
}
