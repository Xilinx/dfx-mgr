/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
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

struct watch {
    int wd;
	char name[64];
    char path[WATCH_PATH_LEN];
	char parent_name[64];
    char parent_path[WATCH_PATH_LEN];
};

struct basePLDesign *findBaseDesign(const char *name){
    int i,j;

    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] != '\0') {
            if(!strcmp(base_designs[i].name, name)) {
                    acapd_debug("%s: Found base design %s\n",__func__,base_designs[i].name);
                    return &base_designs[i];
			}
            for (j=0;j <10; j++){
                if(!strcmp(base_designs[i].accel_list[j].name, name)) {
                    acapd_debug("%s: Found accel %s in base  %s\n",__func__,name,base_designs[i].name);
                    return &base_designs[i];
                }
            }
        }
    }
    acapd_perror("No accel found for %s\n",name);
    return NULL;
}

struct basePLDesign *findBaseDesign_path(const char *path){
    int i,j;

    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] != '\0') {
            if(!strcmp(base_designs[i].base_path, path)) {
                    acapd_debug("%s: Found base design %s\n",__func__,base_designs[i].base_path);
                    return &base_designs[i];
			}
            for (j=0;j <10; j++){
                if(!strcmp(base_designs[i].accel_list[j].path, path)) {
                    acapd_debug("%s: Found accel %s in base  %s\n",__func__,path,base_designs[i].base_path);
                    return &base_designs[i];
                }
            }
        }
    }
    acapd_perror("No base design or accel found for %s\n",path);
    return NULL;
}

void getRMInfo()
{
    FILE *fptr;
    int i;
    char line[512];
    struct basePLDesign *base = platform.active_base;

    fptr = fopen("/home/root/rm_info.txt","w");
    if (fptr == NULL){
        acapd_perror("Couldn't create /home/root/rm_info.txt");
        goto out;
    }

    if(base == NULL) {
        acapd_perror("No design currently loaded");
        return ;
    }
    for (i = 0; i < base->num_pl_slots; i++){
        if (base->slots[i] == NULL)
            sprintf(line, "%d,%s",i,"GREY");
        else
            //sprintf(line,"%d,%s",i, base->slots[i]->accel->pkg->name);
        fprintf(fptr,"\n%s",line);
    }
out:
    fclose(fptr);
}

void update_env(char *path)
{
    DIR *FD;
    struct dirent *dir;
    int len, ret;
    char *str;
    char cmd[128];

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
                    ret = system(cmd);
                    if (ret)
                        acapd_perror("Write to /etc/vart.conf failed\n");
                }
            }
        }
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
        acapd_perror("No package found for %s\n",accel_name);
        goto out;
    } else
        sprintf(shell_path,"%s/shell.json",base->base_path);

    if(base == NULL) {
        acapd_perror("No accel with name %s found\n",accel_name);
		goto out;
    }

    /* Flat shell design are treated as with one slot */
    else if(base != NULL && (!strcmp(base->type,"XRT_FLAT")|| !strcmp(base->type,"PL_FLAT"))) {
        if (base->slots == NULL) {
            base->slots = (slot_info_t **)malloc(sizeof(slot_info_t *) * (base->num_pl_slots+base->num_aie_slots));
            base->slots[0] = NULL;
		}

		if(platform.active_base != NULL && platform.active_base->active > 0) {
            acapd_perror("Remove previously loaded accelerator, no empty slot\n");
            return -1;
        }
        sprintf(pkg->name,"%s",accel_name);
        acapd_debug("%s:loading xrt flat shell design %s\n",__func__,pkg->name);
        pkg->path = base->base_path;
        pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
        init_accel(pl_accel, pkg);
        strncpy(pl_accel->sys_info.tmp_dir, pkg->path,
                sizeof(pl_accel->sys_info.tmp_dir) - 1);
        strcpy(pl_accel->type,"XRT_FLAT");
        ret = load_accel(pl_accel, shell_path, 0);
        if (ret < 0){
            acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
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
            acapd_debug("Found accel %s base %s parent %s\n",accel_info->path,base->base_path,accel_info->parent_path);
        }
    }
    /* For SIHA slotted architecture */
    if(base != NULL && !strcmp(base->type,"PL_DFX")) {
        if (platform.active_base != NULL && !platform.active_base->active && 
				strcmp(platform.active_base->base_path, accel_info->parent_path)) {
			acapd_print("All slots for base are empty, loading new base design\n");
			remove_base(platform.active_base->fpga_cfg_id);
			free(platform.active_base->slots);
			platform.active_base = NULL;
		}
		else if(platform.active_base != NULL && platform.active_base->active > 0 && 
				strcmp(platform.active_base->base_path, accel_info->parent_path)) {
            acapd_perror("Active base design doesn't match this accel base\n");
            goto out;
        }
        if (platform.active_base == NULL) {
            sprintf(pkg->name,"%s",base->name);
            pkg->path = base->base_path;
            pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
			if (base->load_base_design) {
				init_accel(pl_accel, pkg);
				strcpy(pl_accel->type,"PL_DFX");
				acapd_print("Loading base shell design %s\n",base->name);
				ret = load_accel(pl_accel, shell_path, 0);
				if (ret < 0){
					acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
					base->active = 0;
					goto out;
				}
				acapd_print("Loaded %s successfully.\n",base->name);
				base->fpga_cfg_id = pl_accel->sys_info.fpga_cfg_id;
			}
            base->slots = (slot_info_t **)malloc(sizeof(slot_info_t *) * (base->num_pl_slots +
																		base->num_aie_slots));
            for (i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++)
                base->slots[i] = NULL;
            platform.active_base = base;	
		}
        for (i = 0; i < (base->num_pl_slots + base->num_aie_slots); i++) {
            acapd_debug("Finding empty slot for %s i %d \n",accel_name,i);
            if (base->slots[i] == NULL){
                sprintf(path,"%s/%s_slot%d", accel_info->path, accel_info->name,i);
                if (access(path,F_OK) != 0){
                    continue;
                }
				strcpy(slot->name, accel_name);
				if (!strcmp(accel_info->accel_type,"XRT_AIE_DFX")) {
					acapd_perror("%s: XRT_AIE_DFX unsupported", accel_name);
					return -1;
				}
				slot->is_aie = 0;
                strcpy(pkg->name, accel_name);
                pkg->path = path;
                pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
                init_accel(pl_accel, pkg);
                /* Set rm_slot before load_accel() so isolation for appropriate slot can be applied*/
                pl_accel->rm_slot = i;
                strcpy(pl_accel->type,accel_info->accel_type);

                ret = load_accel(pl_accel, shell_path, 0);
                if (ret < 0){
                    acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
					goto out;
                }
				platform.active_base->active += 1;
				slot->accel = pl_accel;
                base->slots[i] = slot;
                acapd_print("Loaded %s successfully to slot %d\n",pkg->name,i);
                return i;
            }
        }
        if (i >= (base->num_pl_slots + base->num_aie_slots))
            acapd_perror("Couldn't find empty slot for %s\n",accel_name);
    }
    else {
        acapd_perror("Check the supported type of base/accel\n");
    }
out:
    free(slot);
    free(pl_accel);
    free(pkg);
    return -1;
}
int remove_accelerator(int slot)
{
    struct basePLDesign *base = platform.active_base;
    acapd_accel_t *accel;
	int ret;
	if (slot < 0) {
		acapd_perror("%s invalid slot %d\n",__func__, slot);
		return 0;
	}
    if (base == NULL || base->slots[slot] == NULL){
        acapd_perror("%s No Accel in slot %d\n",__func__,slot);
        return 0;
    }
	if (base->slots[slot]->is_aie){
		base->slots[slot] = NULL;
		platform.active_base->active -= 1;
		return 0;
	}
    accel = base->slots[slot]->accel;
    acapd_print("Removing accel %s from slot %d\n",accel->pkg->name,slot);

    ret = remove_accel(accel, 0);
    free(accel);
    base->slots[slot] = NULL;
	platform.active_base->active -= 1;
	return ret;
}
void sendBuff(uint64_t size)
{
    acapd_print("%s: allocating buffer of size %lu\n",__func__,size);
    //sendBuffer(size, socket_d);
}
void allocBuffer(uint64_t size)
{
    acapd_print("%s: allocating buffer of size %lu\n",__func__,size);
    allocateBuffer(size);
}
void freeBuff(uint64_t pa)
{
    acapd_debug("%s: free buffer PA %lu\n",__func__,pa);
    freeBuffer(pa);
}

int getFD(int slot, char *name)
{
    struct basePLDesign *base = platform.active_base;
    if(base == NULL){
        acapd_perror("No active design\n");
        return -1;
    }
    acapd_accel_t *accel = base->slots[slot]->accel;
    if (accel == NULL){
        acapd_perror("%s No Accel in slot %d\n",__func__,slot);
        return -1;
    }
	for (int i = 0; i < accel->num_ip_devs; i++) {
		char *dev_name = accel->ip_dev[i].dev_name;
		char substr[32];
		int j = 0;
		int n = strlen(dev_name);
		while (j < n){
			if (dev_name[j] == '.') {
				strncpy(substr, (dev_name + j +1),(n - j));
				break;
			}
			j += 1;
		}
		if(!strcmp(substr, name)){
			acapd_debug("Found ip_dev for %s fd %d\n",name,accel->ip_dev[i].id);
			return accel->ip_dev[i].id;
		}
	}
	acapd_perror("No ip_dev found for %s slot %d\n",name,slot);
	return -1;
}

int dfx_getFDs(int slot, int *fd)
{
    struct basePLDesign *base = platform.active_base;
    if(base == NULL){
        acapd_perror("No active design\n");
        return -1;
    }
    acapd_accel_t *accel = base->slots[slot]->accel;
    if (accel == NULL){
        acapd_perror("%s No Accel in slot %d\n",__func__,slot);
        return -1;
    }
    fd[0] = accel->ip_dev[2*slot].id;
    fd[1] = accel->ip_dev[2*slot+1].id;
    acapd_perror("%s Daemon slot %d accel_config %d d_hls %d\n",
                    __func__,slot,fd[0],fd[1]);
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
char *listAccelerators()
{
    int i,j;
	uint8_t slot;
    char msg[300];
	char res[8*1024];

	memset(res,0, sizeof(res));
 
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
    acapd_perror("no room to add more watch");
}

struct watch *get_watch(int wd){
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (active_watch[i].wd == wd)
            return &active_watch[i];
    }
    return NULL;
}
char * wd_to_pathname(int wd){
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (active_watch[i].wd == wd)
            return active_watch[i].path;
    }
    return NULL;
}
struct watch* path_to_watch(char *path){
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
			acapd_debug("%s already exists\n",path);
            break;
        }
        if (base->accel_list[j].path[0] == '\0') {
			acapd_debug("adding %s to base %s\n",path,parent_path);
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

void parse_packages(struct basePLDesign *base,char *fname, char *path)
{
    DIR *dir1,*dir2;
    struct dirent *d1,*d2;
    char first_level[512],second_level[800],filename[811];
	struct stat stat_info;
	accel_info_t *accel;
    int wd;

	/* For flat shell design there is no subfolder so assign the base path as the accel path */
	if (!strcmp(base->type,"XRT_FLAT") || !strcmp(base->type,"PL_FLAT")) {
		acapd_debug("This is flat shell design\n");
        strcpy(base->accel_list[0].name, base->name);
        strcpy(base->accel_list[0].path, base->base_path);
        strcpy(base->accel_list[0].parent_path, base->base_path);
        strcpy(base->accel_list[0].accel_type, base->type);
		return;
	}

    dir1 = opendir(path);
    if (dir1 == NULL) {
        acapd_perror("Directory %s not found\n",path);
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
                acapd_perror("%s:inotify_add_watch failed on %s\n",__func__,first_level);
				return;
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
					acapd_perror("%s:inotify_add_watch failed on %s\n",__func__,second_level);
					return;
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
}

void add_base_design(char *name, char *path, char *parent, int wd)
{
    int i;

    for (i = 0; i < MAX_WATCH; i++) {
		if (!strcmp(base_designs[i].base_path,path)){
			acapd_debug("Base design %s already exists\n",path);
			return;
		}
        if (base_designs[i].base_path[0] == '\0') {
            acapd_debug("Adding base design %s\n",path);
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
void remove_base_design(char *path,char *parent, int is_base){
    int i, j;

    if (!is_base) {
        acapd_debug("Removing accel %s from base %s\n",path,parent);
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
            acapd_debug("Removing base design %s \n",path);
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

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
void *threadFunc()
{
    int wd, i, j, k, ret;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p;
    char new_dir[512],fname[600];
    struct inotify_event *event;
    DIR *d;
    struct dirent *dir;
    struct basePLDesign *base;

    active_watch = (struct watch *)calloc(sizeof(struct watch), MAX_WATCH);
    base_designs = (struct basePLDesign *)calloc(sizeof(struct basePLDesign), MAX_WATCH);

    for(j = 0; j < MAX_WATCH; j++) {
        active_watch[j].wd = -1;
        active_watch[j].path[0] = '\0';
    }

    for(i = 0; i < MAX_WATCH; i++) {
        base_designs[i].base_path[0] = '\0';
        for( j = 0; j < 10; j++)
            base_designs[i].accel_list[j].path[0] = '\0';
    }
    inotifyFd = inotify_init();                 /* Create inotify instance */
    if (inotifyFd == -1){
        acapd_perror("inotify_init failed\n");
	}

    /* For each command-line argument, add a watch for all events */
	for(k = 0; k < config.number_locations; k++){
	    wd = inotify_add_watch(inotifyFd, config.firmware_locations[k], IN_ALL_EVENTS);
		if (wd == -1) {
			acapd_perror("%s:%s not found, can't add inotify watch\n",__func__,config.firmware_locations[k]);
			exit(EXIT_SUCCESS);
		}
	    add_to_watch(wd,"",config.firmware_locations[k],"","");
		/* Add already existing packages to inotify */
		d = opendir(config.firmware_locations[k]);
		if (d == NULL) {
			acapd_perror("Directory %s not found\n",config.firmware_locations[k]);
		}
		while((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (strlen(dir->d_name) > 64 || strcmp(dir->d_name,".") == 0 ||
                            strcmp(dir->d_name,"..") == 0) {
                continue;
            }
            sprintf(new_dir,"%s/%s", config.firmware_locations[k], dir->d_name);
            acapd_debug("Found dir %s\n",new_dir);
            wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
            if (wd == -1)
                acapd_perror("%s:inotify_add_watch failed on %s\n",__func__,new_dir);
            else {
                add_to_watch(wd, dir->d_name, new_dir,"",config.firmware_locations[k]);
                add_base_design(dir->d_name, new_dir, config.firmware_locations[k], wd);
				base = findBaseDesign(dir->d_name);
				sprintf(fname,"%s/%s",base->base_path,"shell.json");
				ret = initBaseDesign(base, fname);
				if (ret >= 0)
					parse_packages(base,base->name,base->base_path);
            }
        }
    }
	}
	/* Done parsing on target accelerators, now load a default one if present in config file */
	sem_post(&mutex);

    /* Listen for new updates in firmware path*/
    for (;;) {
        numRead = read(inotifyFd, buf, BUF_LEN);
        if (numRead <= 0 && errno != EAGAIN)
            acapd_perror("read() from inotify failed\n");

        /* Process all of the events in buffer returned by read() */

        for (p = buf; p < buf + numRead; ) {
            event = (struct inotify_event *) p;
           // displayInotifyEvent(event);
            if(event->mask & IN_CREATE || event->mask & IN_CLOSE_WRITE){
                if (event->mask & IN_ISDIR) {
                    struct watch *w = get_watch(event->wd);
                    if (w == NULL)
                        break;
                    sprintf(new_dir,"%s/%s",w->path, event->name);
                    acapd_debug("%s: add inotify watch on %s w->name %s parent %s \n",__func__,new_dir,w->name,w->parent_path);
                    wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
                    if (wd == -1)
                        acapd_print("inotify_add_watch failed on %s\n",new_dir);
					add_to_watch(wd, event->name, new_dir, w->name,w->path);
					if (!strcmp(w->parent_path,""))
						add_base_design(event->name, new_dir, w->path, wd);
                }
				else if(!strcmp(event->name,"shell.json")) {
					struct watch *w = get_watch(event->wd);
					base = findBaseDesign_path(w->path);
					sprintf(fname,"%s/%s",w->path,"shell.json");
					initBaseDesign(base, fname);
					parse_packages(base,w->name, w->path);
				}
				else if(!strcmp(event->name,"accel.json")) {
					struct watch *w = get_watch(event->wd);
					struct watch *parent_watch = path_to_watch(w->parent_path);
					accel_info_t *accel;

					base = findBaseDesign_path(parent_watch->parent_path);
					acapd_debug("Add accel %s to base %s\n",w->path, base->base_path);
					accel = add_accel_to_base(base, w->parent_name, w->parent_path, base->base_path);
					initAccel(accel, w->path);
				}
            }
            else if((event->mask & IN_DELETE) && (event->mask & IN_ISDIR)) {
				struct watch *w = get_watch(event->wd);
                if (w == NULL)
                    break;
                sprintf(new_dir,"%s/%s",w->path, event->name);
                acapd_debug("Removing watch on %s parent_path %s \n",new_dir,w->parent_path);
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
					if (w == NULL)
						break;
					if (event->mask & IN_ISDIR){
						sprintf(new_dir,"%s/%s",w->path, event->name);
						add_base_design(event->name, new_dir, w->path, event->wd);
					} else {
						sprintf(new_dir,"%s",w->path);
					}
					base = findBaseDesign_path(new_dir);
					sprintf(fname,"%s/%s",new_dir,"shell.json");
					initBaseDesign(base, fname);
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
