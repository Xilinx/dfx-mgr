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
#include <dfx-mgr/shell.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/json-config.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#define WATCH_PATH_LEN 256
#define MAX_WATCH 50
#define SERVER_PATH     "/tmp/dfx-mgrd_socket"

static int interrupted, socket_d;
static acapd_accel_t *active_slots[3];
char *firmware_path = "/lib/firmware/xilinx";

struct watch {
    int wd;
    char path[WATCH_PATH_LEN];
    char parent_path[WATCH_PATH_LEN];
};

struct watch *active_watch = NULL;
struct basePLDesign *base_designs = NULL;
static int inotifyFd;
platform_info_t platform;

struct pss {
	struct lws_spa *spa;
};

/*static const char * const param_names[] = {
	"text1",
};

enum enum_param_names {
    EPN_TEXT1,
};*/
struct basePLDesign *findBaseDesign(const char *name){
	int i,j;

    for (i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].base_path[0] != '\0') {
			for (j=0;j <10; j++){ 
				if(!strcmp(base_designs[i].accel_list[j].name, name)) {
					acapd_debug("%s: Found existing base desing for %s\n",__func__,name);
					return &base_designs[i];
				}
			}
		}
	}
	acapd_perror("%s:No base design found for %s\n",__func__,name);
	return NULL;
}

char *get_accel_path(const char *name, int slot)
{
	char *accel_path = malloc(sizeof(char)*1024);
	DIR *d;
	struct dirent *dir;
	struct stat info;

    d = opendir(firmware_path);
    if (d == NULL) {
		acapd_perror("Directory %s not found\n",firmware_path);
	}
	while((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_DIR) {
			if (strlen(dir->d_name) > 64) {
				continue;
			}
			if(strcmp(dir->d_name, name) == 0){
				acapd_debug("Found dir %s in %s\n",name,firmware_path);
				sprintf(accel_path,"%s/%s/%s_slot%d", firmware_path, dir->d_name, dir->d_name,slot);
				if (stat(accel_path,&info) != 0)
					return NULL;
				if (info.st_mode & S_IFDIR){
					acapd_debug("Found accelerator path %s\n",accel_path);
					return accel_path;
				}
				else
					acapd_perror("No %s accel for slot %d found\n",name,slot);
			}
			else {
				acapd_debug("Found %s in %s\n",dir->d_name,firmware_path);
			}
		}
	}
	return NULL;
}
void getRMInfo()
{
	FILE *fptr;
	int i;
	char line[512];
	acapd_accel_t *accel;

	fptr = fopen("/home/root/rm_info.txt","w");
	if (fptr == NULL){
		acapd_perror("Couldn't create /home/root/rm_info.txt");
		goto out;
	}

	for (i = 0; i < 3; i++) {
		if(active_slots[i] == NULL)
			sprintf(line, "%d,%s",i,"GREY");
		else {
			accel = active_slots[i];
			sprintf(line,"%d,%s",i,accel->pkg->path);
		}
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

	for (i = 0; i < 10; i++){ 
		if(!strcmp(base->accel_list[i].name, accel_name)) {
			accel_info = &base->accel_list[i];
			printf("Found accel %s base %s parent %s\n",accel_name,base->base_path,accel_info->parent_path);
		}
	}

	/* Flat shell designs which don't have any slot */
	if(base != NULL && !strcmp(base->type,"XRT_FLAT") && accel_info != NULL) {
		if (base->slots[0] != NULL) {
			printf("Remove previously loaded accelerator, no empty slot\n");
			return -1;
		}
		sprintf(pkg->name,"%s",accel_name);
		acapd_debug("%s:loading xrt flat shell design %s\n",__func__,pkg->name);
		pkg->path = accel_info->path;
		pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
		init_accel(accel, pkg);
		accel->type = FLAT_SHELL;
		ret = load_accel(accel, shell_path, 0);
		if (ret < 0){
			acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
			base->active = 0;
			goto out;
		}
		base->active = 1;
		base->slots[0] = slot;

		/* VART libary for SOM desings needs .xclbin path to be written to a file*/
		update_env(pkg->path);
		return 0;	
	}
	else if(base != NULL && !strcmp(base->type,"XRT_FLAT") && accel_info == NULL) {
		printf("No accel found for %s\n",accel_name);
	}
	/* For SIHA slotted architecture */
	else if(base != NULL && strcmp(base->type,"SLOTTED") == 0) {
		if (platform.active_base == NULL) {
			sprintf(pkg->name,"%s",base->name);
			acapd_print("Loading base shell design %s\n",__func__,base->name);
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
			base->active = 1;
			base->slots = (slot_info_t **)malloc(sizeof(slot_info_t *) * base->num_slots);
			for (i = 0; i < base->num_slots; i++)
				base->slots[i] = NULL;
			platform.active_base = base;
			acapd_print("Loaded %s successfully\n",base->name);
		}
		else if(strcmp(platform.active_base->base_path, accel_info->parent_path)) {
			acapd_perror("Active base design doesn't match this accel base\n");
			return -1;
		}
		for (i = 0; i < base->num_slots; i++) {
			printf("Finding empty slot for %s i %d \n",accel_name,i);
			if (base->slots[i] == NULL){
				sprintf(path,"%s/%s_slot%d", accel_info->path, accel_info->name,i);
				strcpy(pkg->name, accel_name);
				pkg->path = path;
				pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
				init_accel(accel, pkg);
				acapd_perror("%s: Loading accel %s to slot %d \n", __func__, pkg->name,i);
				/* Set rm_slot before load_accel() so isolation for appropriate slot can be applied*/
				accel->rm_slot = i;
				accel->type = SIHA_SHELL;

				ret = load_accel(accel, shell_path, 0);
				if (ret < 0){
					acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
				goto out;
				}
				base->slots[i] = slot;
				acapd_print("Loaded %s successfully\n",pkg->name);
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

void remove_accelerator(int slot)
{
	struct basePLDesign *base = platform.active_base;
	int i;

	acapd_accel_t *accel;
	if (base == NULL || base->slots[slot] == NULL){
		acapd_perror("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	accel = base->slots[slot]->accel;
	acapd_print("Removing accel %s from slot %d\n",accel->pkg->name,slot);
    remove_accel(accel, 0);
	free(accel);
	base->slots[slot] = NULL;
	for (i = 0; i < base->num_slots; i++){
		if (base->slots[i] != NULL) break;
	}
	if (i == base->num_slots){
		acapd_print("All slots for base %s are empty.\n",base->name);
		base->active = 0;
		platform.active_base = NULL;
	}
}
void allocBuffer(uint64_t size)
{
	printf("%s: allocating buffer of size %lu\n",__func__,size);
	allocateBuffer(size, socket_d);
}
void freeBuff(uint64_t pa)
{
	printf("%s: free buffer PA %lu\n",__func__,pa);
	freeBuffer(pa);
}

void getShellFD(int slot)
{
	acapd_accel_t *accel = active_slots[slot];
	if (active_slots == NULL || active_slots[slot] == NULL){
		acapd_perror("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	get_shell_fd(accel);
}
void getClockFD(int slot)
{
	acapd_accel_t *accel = active_slots[slot];
	if (active_slots == NULL || active_slots[slot] == NULL){
		acapd_perror("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	get_shell_clock_fd(accel);
}
void listAccelerators()
{
    int i,j;
	printf("%32s%15s%10s%10s\n","Accelerator","Type","#slots","Active");
    for (i = 0; i < MAX_WATCH; i++) {
		if (base_designs[i].base_path[0] != '\0') {
			printf("%32s%15s%10d%10d\n",base_designs[i].name,base_designs[i].type,base_designs[i].num_slots, base_designs[i].active);
			for (j = 0; j < 10; j++) {
				if (base_designs[i].accel_list[j].path[0] != '\0') {
					printf("%32s\n",base_designs[i].accel_list[j].name);
				}
			}
		}
	}
}
struct msg{
    char cmd[32];
    char arg[128];
};
struct resp{
    char data[128];
    int len;
};
#define EXAMPLE_RX_BUFFER_BYTES sizeof(struct msg)
struct payload
{
    unsigned char data[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
    size_t len;
} received_payload;

unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + sizeof(struct resp) + LWS_SEND_BUFFER_POST_PADDING];

static int msgs;
static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
    struct msg *m = (struct msg*)malloc(sizeof(struct msg));
    struct resp *r = (struct resp *)&buf[LWS_SEND_BUFFER_PRE_PADDING];
	int slot;

    switch( reason )
    {
		case LWS_CALLBACK_ESTABLISHED:
			lwsl_user("LWS_CALLBACK_ESTABLISHED user %s\n",(char *)user);
			break;

        case LWS_CALLBACK_RECEIVE:
            acapd_debug("LWS_CALLBACK_RECEIVE len%ld\n",len);
            memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len );
            received_payload.len = len;
            m = (struct msg *)&received_payload.data[LWS_SEND_BUFFER_PRE_PADDING];
            acapd_debug("server received cmd %s arg %s\n",m->cmd,m->arg);
			msgs++;
			if(strcmp(m->cmd,"-load") == 0){
				lwsl_user("Received %s \n",m->cmd);
				slot = load_accelerator(m->arg);
				sprintf(r->data,"%d",slot);
				r->len = 1;
				acapd_debug("daemon: load done slot %s len %d\n", r->data, r->len);
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-remove") == 0){
				lwsl_debug("Received %s slot %s\n",m->cmd,m->arg);
				r->len = 0;
				remove_accelerator(atoi(m->arg));
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-allocBuffer") == 0){
				lwsl_debug("Received %s size %s\n",m->cmd,m->arg);
				allocBuffer(atoi(m->arg));
				sprintf(r->data,"%s","");
				r->len = 0;
				lws_callback_on_writable( wsi );
			}
			else if(strcmp(m->cmd,"-freeBuffer") == 0){
				lwsl_debug("Received %s size %s\n",m->cmd,m->arg);
				freeBuff(atoi(m->arg));
				sprintf(r->data,"%s","");
				r->len = 0;
				lws_callback_on_writable( wsi );
			}
			else if(strcmp(m->cmd,"-getShellFD") == 0){
				lwsl_debug("Received %s slot %s\n",m->cmd,m->arg);
				sprintf(r->data,"%s","");
				r->len = 0;
				getShellFD(atoi(m->arg));
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-getClockFD") == 0){
				lwsl_debug("Received %s slot %s\n",m->cmd,m->arg);
				sprintf(r->data,"%s","");
				r->len = 0;
				getClockFD(atoi(m->arg));
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-getRMInfo") == 0){
				lwsl_debug("Received -getRMInfo\n");
				sprintf(r->data,"%s","");
				r->len = 0;
				getRMInfo();
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-listPackage") == 0){
				lwsl_debug("Received -listPackage\n");
				sprintf(r->data,"%s","");
				r->len = 0;
				listAccelerators();
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else {
				lwsl_err("cmd not recognized\n");
				//return -1;
			}
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
			if (!msgs)
				return 0;
            lwsl_debug("LWS_CALLBACK_SERVER_WRITEABLE resp->len %d\n",r->len);
            lws_write( wsi, (unsigned char*)r, sizeof(struct resp), LWS_WRITE_TEXT );
			msgs--;
			//return -1;
            break;

        default:
            break;
    }

    return 0;
}

enum protocols
{
    PROTOCOL_HTTP = 0,
    PROTOCOL_EXAMPLE,
    PROTOCOL_COUNT
};

static const struct lws_protocols protocols[] = { 
	// first protocol must always be HTTP handler
  {
    "http",			/* name */
    lws_callback_http_dummy,	/* callback */
    0,				/* per session data */
	0,				/* max frame size/ rx buffer */
	0, NULL, 0,
  },
  {
	"example-protocol",
	callback_example,
    0,
    EXAMPLE_RX_BUFFER_BYTES,
	0, NULL, 0,
  },
  { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
};

void sigint_handler(int sig)
{
	lwsl_err("server interrupted signal %d \n",sig);
	interrupted = 1;
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

	printf("parsing packages in base %s\n",path);
    d = opendir(path);
    if (d == NULL) {
		acapd_perror("Directory %s not found\n",firmware_path);
	}
	
	while((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (strlen(dir->d_name) > 64 || strcmp(dir->d_name,".") == 0 ||
                            strcmp(dir->d_name,"..") == 0) {
                continue;
            }
            sprintf(new_dir,"%s/%s", path, dir->d_name);
            acapd_debug("Found dir %s\n",new_dir);
            wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
            if (wd == -1)
                acapd_perror("%s:inotify_add_watch failed on %s\n",__func__,new_dir);
            else {
				for (i=0; i < 10; i++) {
					if (base->accel_list[i].path[0] == '\0') {
						printf("Adding accel %s i %d\n",new_dir, i);
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
					if (base_designs[i].accel_list[j].path[0] == '\0') {
						printf("Adding accel %s j %d\n",path, j);
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
	char shell_path[600];
	struct stat info;

	if (strcmp(parent,firmware_path)) {
		printf("Add accel %s to base %s\n",name,parent);
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
			sprintf(shell_path,"%s/shell.json",base_designs[i].base_path);
			
			while( access(shell_path,F_OK) != 0);
				//printf("waiting on shell.json\n");

			/* If no shell.json provided, treat it as flat shell design */
			if (stat(shell_path,&info) != 0){
				base_designs[i].num_slots = 1;
				strncpy(base_designs[i].type,"XRT_FLAT", sizeof(base_designs[i].type)-1);
	            base_designs[i].type[sizeof(base_designs[i].type)-1] = '\0';
			}
			else {
				initBaseDesign(&base_designs[i], shell_path);
			}
			base_designs[i].active = 0;
			base_designs[i].wd = wd;
			parse_packages(&base_designs[i],path);
			return;
		}
	}
}
void remove_base_design(char *name,char *path,char *parent){
    int i, j;

	if (strcmp(parent,firmware_path)) {
		printf("Removing accel %s from base %s\n",name,parent);
		for (i = 0; i < MAX_WATCH; i++) {
			if (!strcmp(base_designs[i].base_path,parent)) {
				for(j=0; j < 10; j++) {
					if (!strcmp(base_designs[i].accel_list[j].path,path)) {
						printf("Found matching accel to remove\n");
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
			printf("Removing base desing %s \n",path);
			base_designs[i].base_path[0] = '\0';
			base_designs[i].num_slots = 0;
			free(base_designs[i].slots);
			base_designs[i].active = 0;
			base_designs[i].wd = -1;
			return;
		}
	}
}


#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
void *threadFunc()
{
    int wd, i, j;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p, *parent = NULL;
    char new_dir[512];
    struct inotify_event *event;
	DIR *d;
	struct dirent *dir;
	//int new_base = 0;
	
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

    wd = inotify_add_watch(inotifyFd, firmware_path, IN_ALL_EVENTS);
    if (wd == -1) {
		acapd_perror("%s:%s not found, can't add inotify watch\n",__func__,firmware_path);
		exit(EXIT_SUCCESS);
	}
    add_to_watch(wd,firmware_path);

	/* Add already existing packages to inotify */
    d = opendir(firmware_path);
    if (d == NULL) {
		acapd_perror("Directory %s not found\n",firmware_path);
	}
	while((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_DIR) {
			if (strlen(dir->d_name) > 64 || strcmp(dir->d_name,".") == 0 ||
							strcmp(dir->d_name,"..") == 0) {
				continue;
			}
			sprintf(new_dir,"%s/%s", firmware_path, dir->d_name);
			acapd_perror("Found dir %s\n",new_dir);
			wd = inotify_add_watch(inotifyFd, new_dir, IN_ALL_EVENTS);
			if (wd == -1)
				acapd_perror("%s:inotify_add_watch failed on %s\n",__func__,new_dir);
			else {
				add_to_watch(wd, new_dir);
				add_base_design(dir->d_name, new_dir, firmware_path, wd);
			}
		}
	}

	/* Listen for new updates in firmware path*/
    for (;;) {
        numRead = read(inotifyFd, buf, BUF_LEN);
        if (numRead == 0)
            acapd_perror("read() from inotify fd returned 0!");

        if (numRead == -1)
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
					else {
						add_to_watch(wd, new_dir);
						add_base_design(event->name, new_dir, parent, wd);
					}
				}
            }
			else if((event->mask & IN_DELETE) && (event->mask & IN_ISDIR)) {
                char * parent = wd_to_pathname(event->wd);
				if (parent == NULL)
					break;
                sprintf(new_dir,"%s/%s",parent, event->name);
				printf("Removing watch on %s parent %s\n",event->name, parent);
				acapd_debug("%s:removing watch on  %s \n",__func__,new_dir);
                remove_watch(new_dir);
				remove_base_design(event->name, new_dir, parent);
            }
            else if((event->mask & IN_MOVED_FROM) && (event->mask & IN_ISDIR)) {
                char * parent = wd_to_pathname(event->wd);
				if (parent == NULL)
					break;
                sprintf(new_dir,"%s/%s",parent, event->name);
				remove_base_design(event->name, new_dir, parent);
            }
            else if((event->mask & IN_MOVED_TO) && (event->mask & IN_ISDIR)) {
                char * parent = wd_to_pathname(event->wd);
				if (parent == NULL)
					break;
                sprintf(new_dir,"%s/%s",parent, event->name);
				add_base_design(event->name, new_dir, parent, event->wd);
			}
            p += sizeof(struct inotify_event) + event->len;
        }
    }
    //exit(EXIT_SUCCESS);
}
void socket_fd_setup()
{
   struct sockaddr_un serveraddr;

    socket_d = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_d < 0) {
        acapd_perror("%s socket creation failed\n",__func__);
        //return ACAPD_ACCEL_FAILURE;
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, SERVER_PATH);

    if (bind(socket_d, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr))) {
        acapd_perror("%s socket bind() failed\n",__func__);
        //return ACAPD_ACCEL_FAILURE;
    }
    acapd_debug("%s socket bind done\n",__func__);

    //socket will queue upto 10 incoming connections
    if (listen(socket_d, 10)) {
        acapd_perror("%s socket listen() failed\n",__func__);
        //return ACAPD_ACCEL_FAILURE;
    }
    printf("%s Server started %s ready for client connect.\n",
                                    __func__,SERVER_PATH);
}

int main(int argc, const char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	pthread_t t;
	const char *p;
	//int ret;

	int n = 0, logs = LLL_ERR | LLL_WARN
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */;

	acapd_debug("Starting http daemon\n");
	signal(SIGINT, sigint_handler);

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS minimal http server dynamic | visit http://localhost:7681\n");

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	//info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
	//	       LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
	//	LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	info.port = 7681;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.vhost_name = "localhost";
	info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}


	if (!lws_create_vhost(context, &info)) {
		lwsl_err("Failed to create tls vhost\n");
		goto bail;
	}
	strcpy(platform.boardName,"Xilinx board");
	pthread_create(&t, NULL,threadFunc, NULL);
	printf("pthread created \n");
	socket_fd_setup();
	while (n >= 0 && !interrupted)
		n = lws_service(context, 0);

bail:
	lws_context_destroy(context);

	return 0;
}
