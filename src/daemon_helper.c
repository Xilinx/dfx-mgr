/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
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
void firmware_dir_walk(void);

struct watch {
    int wd;
	char name[64];
    char path[WATCH_PATH_LEN];
	char parent_name[64];
    char parent_path[WATCH_PATH_LEN];
};

struct basePLDesign *findBaseDesign(const char *name)
{
    int i,j;

    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] != '\0') {
            if(!strcmp(base_designs[i].name, name)) {
                    DFX_DBG("Found base design %s", base_designs[i].name);
                    return &base_designs[i];
			}
            for (j=0;j <10; j++){
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
            for (j=0;j <10; j++){
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

/*
 * Update Vitis AI Runtime (VART) env
 */
void update_env(char *path)
{
    DIR *FD;
    struct dirent *dir;
    int len, ret;
    char *str;
    char cmd[128];

    DFX_DBG("%s", path);
    FD = opendir(path);
    if (FD) {
        while ((dir = readdir(FD)) != NULL) {
            len = strlen(dir->d_name);
            if (len > 7) {
                if (!strcmp(dir->d_name + (len - 7), ".xclbin") ||
                        !strcmp(dir->d_name + (len - 7), ".XCLBIN")) {
                    str = (char *) calloc((len + strlen(path) + 1),
                                                    sizeof(char));
                    sprintf(str, "%s/%s",path,dir->d_name);
                    sprintf(cmd,"echo \"firmware: %s\" > /etc/vart.conf",str);
                    free(str);
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
    char path[1024];
    char shell_path[600];
    acapd_accel_t *pl_accel = (acapd_accel_t *)calloc(sizeof(acapd_accel_t), 1);
    acapd_accel_pkg_hd_t *pkg = (acapd_accel_pkg_hd_t *)calloc(sizeof(acapd_accel_pkg_hd_t),1);
    struct basePLDesign *base = findBaseDesign(accel_name);
    slot_info_t *slot = (slot_info_t *)malloc(sizeof(slot_info_t));
    accel_info_t *accel_info = NULL;

	slot->accel = NULL;

    if(base == NULL) {
        DFX_ERR("No package found for %s", accel_name);
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

        /* VART libary for SOM desings needs .xclbin path to be written to a file*/
		if(!strcmp(base->type,"XRT_FLAT"))
			update_env(pkg->path);
        return 0;
    }
    for (i = 0; i < 10; i++){
        if(!strcmp(base->accel_list[i].name, accel_name)) {
            accel_info = &base->accel_list[i];
            DFX_DBG("Found accel %s base %s parent %s", accel_info->path, base->base_path, accel_info->parent_path);
        }
    }
    /* For SIHA slotted architecture */
    if(!strcmp(base->type,"PL_DFX")) {
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
        for (i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++) {
            DFX_DBG("Finding empty slot for %s i %d", accel_name, i);
            if (base->slots[i] == NULL){
                sprintf(path,"%s/%s_slot%d", accel_info->path, accel_info->name,i);
                if (access(path,F_OK) != 0){
                    continue;
                }
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
                DFX_PR("Loaded %s successfully to slot %d", pkg->name, i);
                return i;
            }
        }
        if (i >= (base->num_pl_slots + base->num_aie_slots))
            DFX_ERR("No empty slot for %s", accel_name);
    }
    else {
        DFX_ERR("Check the supported type of base/accel");
    }
out:
    free(slot);
    free(pl_accel);
    free(pkg);
    return -1;
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

int remove_accelerator(int slot)
{
	struct basePLDesign *base = platform.active_base;
	acapd_accel_t *accel;
	int ret;

	/* slot -1 means remove base PL design */
	if (slot == -1)
		return remove_accel_base();

	if (!base || slot < 0 || base->active < slot || !base->slots[slot]) {
		DFX_ERR("No Accel or invalid slot %d", slot);
		return -1;
	}
	if (base->slots[slot]->is_aie){
		free(base->slots[slot]);
		base->slots[slot] = NULL;
		platform.active_base->active -= 1;
		return 0;
	}
	accel = base->slots[slot]->accel;
	DFX_PR("Removing accel %s from slot %d", accel->pkg->name, slot);

	ret = remove_accel(accel, 0);
	free(accel->pkg);
	free(accel);
	free(base->slots[slot]);
	base->slots[slot] = NULL;
	platform.active_base->active -= 1;
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

int getFD(int slot, char *name)
{
	struct basePLDesign *base = platform.active_base;
	if(base == NULL || base->slots == NULL){
		DFX_ERR("No active design");
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
list_accel_uio(int slot, char *buf, size_t sz)
{
	struct basePLDesign *base = platform.active_base;
	acapd_accel_t *accel;
	char *end = buf + sz - 80;
	char *p = buf;
	int i;

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
get_accel_uio_by_name(int slot, const char *name)
{
	struct basePLDesign *base = platform.active_base;
	acapd_accel_t *accel;

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

char *listAccelerators()
{
    int i,j;
	uint8_t slot;
    char msg[330];	/* compiler warning if 326 bytes or less */
	char res[8*1024];

	memset(res,0, sizeof(res));
	firmware_dir_walk();
 
    sprintf(msg,"%32s%20s%32s%20s%20s%20s\n\n","Accelerator","Accel_type","Base","Base_type","#slots(PL+AIE)","Active_slot");
	strcat(res,msg);
    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] != '\0' && base_designs[i].num_pl_slots > 0) {
            for (j = 0; j < 10; j++) {
                if (base_designs[i].accel_list[j].path[0] != '\0') {
					char s[20];

					/* Internally flat shell is treated as one slot to make the code generic and save info of the active design */
					if (!strcmp(base_designs[i].type,"XRT_FLAT") || !strcmp(base_designs[i].type,"PL_FLAT"))
						sprintf(s,"(%d+%d)", 0, base_designs[i].num_aie_slots);
					else
						sprintf(s,"(%d+%d)", base_designs[i].num_pl_slots,base_designs[i].num_aie_slots);

                    if (base_designs[i].active) {
                        char active_slots[20] = "";
                        char tmp[5];
                        for(slot = 0; slot < (base_designs[i].num_pl_slots +  base_designs[i].num_aie_slots); slot++) {
							if (base_designs[i].slots[slot] != NULL && base_designs[i].slots[slot]->is_aie &&
									!strcmp(base_designs[i].slots[slot]->name,base_designs[i].accel_list[j].name)){
                                    sprintf(tmp,"%d,",slot);
                                    strcat(active_slots,tmp);
							}
                            else if (base_designs[i].slots[slot] != NULL && !base_designs[i].slots[slot]->is_aie &&
										!strcmp(base_designs[i].slots[slot]->accel->pkg->name,base_designs[i].accel_list[j].name)){
                                    sprintf(tmp,"%d,",slot);
                                    strcat(active_slots,tmp);
                            }
                        }
                        if (strcmp(active_slots, ""))
                            sprintf(msg,"%32s%20s%32s%20s%20s%20s\n",base_designs[i].accel_list[j].name,
										base_designs[i].accel_list[j].accel_type, base_designs[i].name,
										base_designs[i].type, s,active_slots);
                        else
                            sprintf(msg,"%32s%20s%32s%20s%20s%20d\n",base_designs[i].accel_list[j].name,
										base_designs[i].accel_list[j].accel_type,base_designs[i].name,
                                                    base_designs[i].type, s,-1);
						strcat(res,msg);
                    }
                    else {
                        sprintf(msg,"%32s%20s%32s%20s%20s%20d\n",base_designs[i].accel_list[j].name,
							base_designs[i].accel_list[j].accel_type,base_designs[i].name,
																base_designs[i].type, s,-1);
						strcat(res,msg);
                    }
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

	for (j = 0; j < 10; j++) {
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
        if (d1->d_type == DT_DIR) {
            if (strlen(d1->d_name) > 64 || strcmp(d1->d_name,".") == 0 ||
                            strcmp(d1->d_name,"..") == 0) {
                continue;
            }
            sprintf(first_level,"%s/%s", path, d1->d_name);
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
					if (d2->d_type == DT_DIR) {
						if (strlen(d2->d_name) > 64 || strcmp(d2->d_name,".") == 0 ||
							strcmp(d2->d_name,"..") == 0) {
						continue;
						}
					}
					sprintf(second_level,"%s/%s", first_level, d2->d_name);
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
			}
				/* Found accel.json so add it*/
			else {
				accel = add_accel_to_base(base,d1->d_name, first_level, path);
				initAccel(accel, first_level);
            }
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
                for(j=0; j < 10; j++) {
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
            for (j=0; j < 10; j++) {
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

/*
 * Filter: path a is over 64 char, "." and ".."
 */
static int
d_name_filter(char *a)
{
	return (strlen(a) > 64) ||
		(a[0] == '.' && (a[1] == 0 || (a[1] == '.' && a[2] == 0)));
}

void
acell_dir_add(char *cpath, struct dirent *dirent)
{
	char new_dir[512], fname[600];
	char *d_name = dirent->d_name;
	struct basePLDesign *base;
	int wd;

	if (dirent->d_type != DT_DIR || d_name_filter(d_name))
		return;

	sprintf(new_dir, "%s/%s", cpath, d_name);
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
void
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

    active_watch = (struct watch *)calloc(sizeof(struct watch), MAX_WATCH);
    base_designs = (struct basePLDesign *)calloc(sizeof(struct basePLDesign), MAX_WATCH);

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
	int ret;
	//struct stat info;
	strcpy(config.defaul_accel_name, "");

	strcpy(platform.boardName,"Xilinx board");
	sem_init(&mutex, 0, 0);

	parse_config(CONFIG_PATH, &config);
	pthread_create(&t, NULL,threadFunc, NULL);
	sem_wait(&mutex);
	//TODO Save active design on filesytem and on reboot read that
	//if (stat("/configfs/device-tree/overlays",&info))
	//	ret = system("rmdir /configfs/device-tree/overlays/*");
	_unused(ret);
	if (config.defaul_accel_name != NULL && strcmp(config.defaul_accel_name, "") != 0)
		load_accelerator(config.defaul_accel_name);
	return 0;
}
