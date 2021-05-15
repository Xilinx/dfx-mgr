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

struct watch {
    int wd;
    char path[WATCH_PATH_LEN];
    char parent_path[WATCH_PATH_LEN];
};
sem_t mutex;

struct watch *active_watch = NULL;
struct basePLDesign *base_designs = NULL;
static int inotifyFd;
platform_info_t platform;

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
                    acapd_print("%s: Found base design %s\n",__func__,base_designs[i].base_path);
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
    for (i = 0; i < base->num_slots; i++){
        if (base->slots[i] == NULL)
            sprintf(line, "%d,%s",i,"GREY");
        else
            sprintf(line,"%d,%s",i, base->slots[i]->accel->pkg->name);
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
    acapd_accel_t *accel = (acapd_accel_t *)malloc(sizeof(acapd_accel_t));
    acapd_accel_pkg_hd_t *pkg = (acapd_accel_pkg_hd_t *) malloc(sizeof(acapd_accel_pkg_hd_t));
    struct basePLDesign *base = findBaseDesign(accel_name);
    slot_info_t *slot = malloc(sizeof(slot_info_t));
    accel_info_t *accel_info = NULL;

    if(base == NULL) {
        acapd_perror("No package found for %s\n",accel_name);
        goto out;
    } else
        sprintf(shell_path,"%s/shell.json",base->base_path);

    slot->accel = accel;

    if(base == NULL) {
        printf("No accel found for %s\n",accel_name);
		goto out;
    }

    /* Flat shell design are treated as with one slot */
    else if(base != NULL && !strcmp(base->type,"XRT_FLAT")) {
        if (base->slots == NULL) {
            base->slots = (slot_info_t **)malloc(sizeof(slot_info_t *) * base->num_slots);
            base->slots[0] = NULL;
		}

		if(platform.active_base != NULL && platform.active_base->active > 0) {
            printf("Remove previously loaded accelerator, no empty slot\n");
            return -1;
        }
        sprintf(pkg->name,"%s",accel_name);
        acapd_debug("%s:loading xrt flat shell design %s\n",__func__,pkg->name);
        pkg->path = base->base_path;
        pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
        init_accel(accel, pkg);
        accel->type = FLAT_SHELL;
        ret = load_accel(accel, shell_path, 0);
        if (ret < 0){
            acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
            base->active = 0;
            goto out;
        }
		accel->rm_slot = 0;
        base->fpga_cfg_id = accel->sys_info.fpga_cfg_id;
        base->active += 1;
        base->slots[0] = slot;
        platform.active_base = base;

        /* VART libary for SOM desings needs .xclbin path to be written to a file*/
        update_env(pkg->path);
        return 0;
    }
    for (i = 0; i < 10; i++){
        if(!strcmp(base->accel_list[i].name, accel_name)) {
            accel_info = &base->accel_list[i];
            acapd_debug("Found accel %s base %s parent %s\n",accel_name,base->base_path,accel_info->parent_path);
        }
    }
    /* For SIHA slotted architecture */
    if(base != NULL && !strcmp(base->type,"SLOTTED")) {
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
            acapd_print("Loading base shell design %s\n",base->name);
            pkg->path = base->base_path;
            pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
            init_accel(accel, pkg);
            accel->type = FLAT_SHELL;
            ret = load_accel(accel, shell_path, 0);
            if (ret < 0){
                acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
                base->active = 0;
                goto out;
            }
            base->fpga_cfg_id = accel->sys_info.fpga_cfg_id;
            base->slots = (slot_info_t **)malloc(sizeof(slot_info_t *) * base->num_slots);
            for (i = 0; i < base->num_slots; i++)
                base->slots[i] = NULL;
            platform.active_base = base;
            acapd_print("Loaded %s successfully.\n",base->name);
        }
        for (i = 0; i < base->num_slots; i++) {
            acapd_debug("Finding empty slot for %s i %d \n",accel_name,i);
            if (base->slots[i] == NULL){
                sprintf(path,"%s/%s_slot%d", accel_info->path, accel_info->name,i);
                if (access(path,F_OK) != 0){
                    printf("No accel found for %s slot %d\n",accel_info->name,i);
                    continue;
                }
                strcpy(pkg->name, accel_name);
                pkg->path = path;
                pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
                init_accel(accel, pkg);
                /* Set rm_slot before load_accel() so isolation for appropriate slot can be applied*/
                accel->rm_slot = i;
                accel->type = SIHA_SHELL;

                ret = load_accel(accel, shell_path, 0);
                if (ret < 0){
                    acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
                goto out;
                }
				platform.active_base->active += 1;
                base->slots[i] = slot;
                acapd_print("Loaded %s successfully to slot %d\n",pkg->name,i);
                return i;
            }
        }
        if (i >= base->num_slots)
            acapd_perror("Couldn't find empty slot for %s\n",accel_name);
    }
    else {
        acapd_perror("Not a valid accelerator package type.\n");
    }
out:
    free(slot);
    free(accel);
    free(pkg);
    return -1;
}
int remove_accelerator(int slot)
{
    struct basePLDesign *base = platform.active_base;
    acapd_accel_t *accel;
	int ret;

    if (base == NULL || base->slots[slot] == NULL){
        acapd_perror("%s No Accel in slot %d\n",__func__,slot);
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
    printf("%s: free buffer PA %lu\n",__func__,pa);
    freeBuffer(pa);
}

void getFDs(int slot)
{
    struct basePLDesign *base = platform.active_base;
    if(base == NULL){
        acapd_perror("No active design\n");
        return;
    }
    acapd_accel_t *accel = base->slots[slot]->accel;
    if (accel == NULL){
        acapd_perror("%s No Accel in slot %d\n",__func__,slot);
        return;
    }
   //get_fds(accel, slot, socket_d);
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
    int i,j, slot;
    char msg[256];
	char res[8*1024];

	memset(res,0, sizeof(res));
 
    sprintf(msg,"%32s%32s%15s%10s%20s\n\n","Accelerator","Base","Type","#slots","Active_slot");
	strcat(res,msg);
    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] != '\0' && base_designs[i].num_slots > 0) {
            for (j = 0; j < 10; j++) {
                if (base_designs[i].accel_list[j].path[0] != '\0') {
                    if (base_designs[i].active) {
                        char active_slots[20] = "";
                        char tmp[4];
                        for(slot = 0; slot < base_designs[i].num_slots; slot++) {
                            if( base_designs[i].slots[slot] != NULL && !strcmp(base_designs[i].slots[slot]->accel->pkg->name,
                                                                                base_designs[i].accel_list[j].name)){
                                    sprintf(tmp,"%d,",base_designs[i].slots[slot]->accel->rm_slot);
                                    strcat(active_slots,tmp);
                            }
                        }
                        if (strcmp(active_slots, ""))
                            sprintf(msg,"%32s%32s%15s%10d%20s\n",base_designs[i].accel_list[j].name, base_designs[i].name,
                                                                base_designs[i].type, base_designs[i].num_slots,active_slots);
                        else
                            sprintf(msg,"%32s%32s%15s%10d%20d\n",base_designs[i].accel_list[j].name,base_designs[i].name,
                                                    base_designs[i].type,base_designs[i].num_slots,-1);
						strcat(res,msg);
                    }
                    else {
                        sprintf(msg,"%32s%32s%15s%10d%20d\n",base_designs[i].accel_list[j].name,base_designs[i].name,
                                                base_designs[i].type,base_designs[i].num_slots,-1);
						strcat(res,msg);
                    }
                }
            }
        }
    }
	return strdup(res);
}

void add_to_watch(int wd, char *pathname)
{
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (active_watch[i].wd == -1) {
            //printf("adding watch to list %s\n",pathname);
            active_watch[i].wd = wd;
            strncpy(active_watch[i].path, pathname, WATCH_PATH_LEN -1);
            return;
        }
    }
    printf("no room to add more watch");
}

char * wd_to_pathname(int wd){
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (active_watch[i].wd == wd)
            return active_watch[i].path;
    }
    return NULL;
}

void remove_watch(char *path)
{
    int i;
    for (i = 0; i < MAX_WATCH; i++) {
        if (strcmp(active_watch[i].path, path) == 0){
		//printf("removing watch %s\n",path);
        inotify_rm_watch(inotifyFd, active_watch[i].wd);
        active_watch[i].wd = -1;
        active_watch[i].path[0] = '\0';
        }
    }
}
void parse_packages(struct basePLDesign *base, char *path)
{
    DIR *d;
    struct dirent *dir;
    char new_dir[512];
    int i,wd;

	/* For flat shell design there is no subfolder so assign the base path as the accel path */
	if (!strcmp(base->type,"XRT_FLAT")) {
		acapd_debug("This is flat shell design\n");
        strcpy(base->accel_list[0].name, base->name);
        strcpy(base->accel_list[0].path, base->base_path);
        strcpy(base->accel_list[0].parent_path, base->base_path);
		return;
	}

    d = opendir(path);
    if (d == NULL) {
        acapd_perror("Directory %s not found\n",path);
		return;
    }
    while((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (strlen(dir->d_name) > 64 || strcmp(dir->d_name,".") == 0 ||
                            strcmp(dir->d_name,"..") == 0) {
                continue;
            }
            sprintf(new_dir,"%s/%s", path, dir->d_name);
            wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
            if (wd == -1)
                acapd_perror("%s:inotify_add_watch failed on %s\n",__func__,new_dir);
            else {
                for (i=0; i < 10; i++) {
                    if (base->accel_list[i].path[0] == '\0') {
                        strcpy(base->accel_list[i].name, dir->d_name);
                        base->accel_list[i].name[sizeof(base->accel_list[i].name) - 1] = '\0';
                        strcpy(base->accel_list[i].path, new_dir);
                        base->accel_list[i].path[sizeof(base->accel_list[i].path) - 1] = '\0';
                        strcpy(base->accel_list[i].parent_path, path);
                        base->accel_list[i].parent_path[sizeof(base->accel_list[i].parent_path) - 1] = '\0';
                        base->accel_list[i].wd = wd;
                        break;
                    }
                }
            }
        }
    }
}
void add_accel_to_base(char *name, char *path, char *parent_path)
{
    int i, j, wd;

    for (i = 0; i < MAX_WATCH; i++) {
        if (!strcmp(base_designs[i].base_path,parent_path)) {
            wd = inotify_add_watch(inotifyFd, path, IN_ALL_EVENTS);
            if (wd == -1)
                acapd_perror("%s:inotify_add_watch failed on %s\n",__func__,path);
            else {
                for (j = 0; j < 10; j++) {
                    if (!strcmp(base_designs[i].accel_list[j].path, path)) {
                        return;
                    }
                    if (base_designs[i].accel_list[j].path[0] == '\0') {
                        strcpy(base_designs[i].accel_list[j].name, name);
                        base_designs[i].accel_list[j].name[sizeof(base_designs[i].accel_list[j].name) - 1] = '\0';
                        strcpy(base_designs[i].accel_list[j].path, path);
                        base_designs[i].accel_list[j].path[sizeof(base_designs[i].accel_list[j].path) - 1] = '\0';
                        strcpy(base_designs[i].accel_list[j].parent_path, parent_path);
                        base_designs[i].accel_list[j].parent_path[sizeof(base_designs[i].accel_list[j].parent_path) - 1] = '\0';
                        base_designs[i].accel_list[j].wd = wd;
                        break;
                    }
                }
            }
        }
    }
}

void add_base_design(char *name, char *path, char *parent, int wd)
{
    int i;

	/* Add accel to existing base design*/
    if (strcmp(parent,FIRMWARE_PATH)) {
        acapd_debug("Add accel %s to base %s\n",name,parent);
        add_accel_to_base(name, path, parent);
        return;
    }
    for (i = 0; i < MAX_WATCH; i++) {
        if (base_designs[i].base_path[0] == '\0') {
            acapd_debug("adding base design %s\n",path);
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
void remove_base_design(char *path,char *parent){
    int i, j;

    if (strcmp(parent,FIRMWARE_PATH)) {
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
            base_designs[i].num_slots = 0;
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
    int wd, i, j, ret;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p, *parent = NULL;
    char new_dir[512],fname[600];
    struct inotify_event *event;
    DIR *d;
    struct dirent *dir;
    struct basePLDesign *base;

    active_watch = (struct watch *)malloc(MAX_WATCH * sizeof(struct watch));
    base_designs = (struct basePLDesign *)malloc(MAX_WATCH * sizeof(struct basePLDesign));

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
    if (inotifyFd == -1)
        acapd_perror("inotify_init failed\n");

    /* For each command-line argument, add a watch for all events */

    wd = inotify_add_watch(inotifyFd, FIRMWARE_PATH, IN_ALL_EVENTS);
    if (wd == -1) {
        acapd_perror("%s:%s not found, can't add inotify watch\n",__func__,FIRMWARE_PATH);
        exit(EXIT_SUCCESS);
    }
    add_to_watch(wd,FIRMWARE_PATH);
    /* Add already existing packages to inotify */
    d = opendir(FIRMWARE_PATH);
    if (d == NULL) {
        acapd_perror("Directory %s not found\n",FIRMWARE_PATH);
    }
    while((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (strlen(dir->d_name) > 64 || strcmp(dir->d_name,".") == 0 ||
                            strcmp(dir->d_name,"..") == 0) {
                continue;
            }
            sprintf(new_dir,"%s/%s", FIRMWARE_PATH, dir->d_name);
            acapd_debug("Found dir %s\n",new_dir);
            wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
            if (wd == -1)
                acapd_perror("%s:inotify_add_watch failed on %s\n",__func__,new_dir);
            else {
                add_to_watch(wd, new_dir);
                add_base_design(dir->d_name, new_dir, FIRMWARE_PATH, wd);
				base = findBaseDesign(dir->d_name);
				sprintf(fname,"%s/%s",base->base_path,"shell.json");
				ret = initBaseDesign(base, fname);
				if (ret >= 0)
					parse_packages(base,base->base_path);
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
                    parent = wd_to_pathname(event->wd);
                    if (parent == NULL)
                        break;
                    sprintf(new_dir,"%s/%s",parent, event->name);
                    acapd_debug("%s: add inotify watch on %s\n",__func__,new_dir);
                    wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
                    if (wd == -1)
                        printf("inotify_add_watch failed on %s\n",new_dir);
                    add_to_watch(wd, new_dir);
                    add_base_design(event->name, new_dir, parent, wd);

                }
				else if(!strcmp(event->name,"shell.json")) {
					parent = wd_to_pathname(event->wd);
					base = findBaseDesign_path(parent);
					sprintf(fname,"%s/%s",parent,"shell.json");
					initBaseDesign(base, fname);
					parse_packages(base,parent);
				}
            }
            else if((event->mask & IN_DELETE) && (event->mask & IN_ISDIR)) {
                char * parent = wd_to_pathname(event->wd);
                if (parent == NULL)
                    break;
                sprintf(new_dir,"%s/%s",parent, event->name);
                acapd_debug("Removing watch on %s parent %s\n",event->name, parent);
                acapd_debug("%s:removing watch on  %s \n",__func__,new_dir);
                remove_watch(new_dir);
                remove_base_design(new_dir, parent);
            }
            else if((event->mask & IN_MOVED_FROM) && (event->mask & IN_ISDIR)) {
                char * parent = wd_to_pathname(event->wd);
                if (parent == NULL)
                    break;
                sprintf(new_dir,"%s/%s",parent, event->name);
                remove_base_design(new_dir, parent);
            }
            else if((event->mask & IN_MOVED_TO) && (event->mask & IN_ISDIR)) {
                char * parent = wd_to_pathname(event->wd);
                if (parent == NULL)
                    break;
                sprintf(new_dir,"%s/%s",parent, event->name);
                add_base_design(event->name, new_dir, parent, event->wd);
				base = findBaseDesign_path(new_dir);
				sprintf(fname,"%s/%s",new_dir,"shell.json");
				initBaseDesign(base, fname);
				parse_packages(base,new_dir);
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
	struct daemon_config config;
	//struct stat info;

	strcpy(platform.boardName,"Xilinx board");
	sem_init(&mutex, 0, 0);
	pthread_create(&t, NULL,threadFunc, NULL);
	sem_wait(&mutex);
	//if (stat("/configfs/device-tree/overlays",&info))
	//	ret = system("rmdir /configfs/device-tree/overlays/*");
	_unused(ret);
	parse_config(CONFIG_PATH, &config);
	if (config.defaul_accel_name != NULL && strcmp(config.defaul_accel_name, "") != 0)
		load_accelerator(config.defaul_accel_name);
	return 0;
}
