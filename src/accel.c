/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/device.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/shell.h>
#include <sys/stat.h>
#include <libdfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

void init_accel(acapd_accel_t *accel, acapd_accel_pkg_hd_t *pkg)
{
	acapd_assert(accel != NULL);
	memset(accel, 0, sizeof(*accel));
	accel->pkg = pkg;
	accel->rm_slot = -1;
	accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
}

int acapd_parse_config(acapd_accel_t *accel, const char *shell_config)
{
	int ret;

	ret = sys_accel_config(accel);
	if (ret < 0) {
		acapd_perror("%s: failed to config accel.\n", __func__);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = acapd_shell_config(shell_config);
	if (ret < 0) {
		acapd_perror("%s: failed to config shell.\n", __func__);
		return ACAPD_ACCEL_FAILURE;
	}
	return ret;
}

int load_accel(acapd_accel_t *accel, const char *shell_config, unsigned int async)
{
	int ret;

	acapd_assert(accel != NULL);
	ret = acapd_parse_config(accel, shell_config);
	if (ret < 0) {
		acapd_perror("%s: failed to parse config files.\n", __func__);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = sys_needs_load_accel(accel);
	if (ret == 0) {
		acapd_debug("%s: no need to load accel.\n", __func__);
		return 0;
	}
	/* assert isolation before programming */
	if (accel->type == SIHA_SHELL) {
		ret = acapd_shell_assert_isolation(accel);
		if (ret < 0) {
			acapd_perror("%s, failed to assert isolaction.\n",
										     __func__);
			return ret;
		}
	}

	if (accel->is_cached == 0) {
		ret = sys_fetch_accel(accel, DFX_NORMAL_EN);
		if (ret != ACAPD_ACCEL_SUCCESS) {
			acapd_perror("%s, failed to fetch partial bistream\n",__func__);
			return ret;
		}
		accel->is_cached = 1;
	}
	ret = sys_load_accel(accel, async);
	if (ret == ACAPD_ACCEL_SUCCESS) {
		accel->status = ACAPD_ACCEL_STATUS_INUSE;
	} else if (ret == ACAPD_ACCEL_INPROGRESS) {
		accel->status = ACAPD_ACCEL_STATUS_LOADING;
	} else {
		acapd_perror("%s: Failed to load partial bitstream ret %d\n",__func__,ret);
		accel->load_failure = ret;
		return ret;
	}
	if (accel->status == ACAPD_ACCEL_STATUS_INUSE && accel->type == SIHA_SHELL) {
		ret = acapd_shell_release_isolation(accel);
		if (ret != 0) {
			acapd_perror("%s: failed to release isolation.\n",__func__);
			return ACAPD_ACCEL_FAILURE;
		}
		acapd_debug("%s: releasing isolation done.\n", __func__);
	}
	//ret = sys_load_accel_post(accel);
	return ret;
}

int accel_load_status(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	if (accel->load_failure != ACAPD_ACCEL_SUCCESS) {
		return accel->load_failure;
	} else if (accel->status != ACAPD_ACCEL_STATUS_INUSE) {
		return ACAPD_ACCEL_INVALID;
	} else {
		return ACAPD_ACCEL_SUCCESS;
	}
}
int remove_base(int fpga_cfg_id)
{
	if (fpga_cfg_id <= 0) {
		acapd_perror("Invalid fpga cfg id: %d.\n", fpga_cfg_id);
		return -1;
	}
	return sys_remove_base(fpga_cfg_id);
}

int remove_accel(acapd_accel_t *accel, unsigned int async)
{
	acapd_assert(accel != NULL);
	if (accel->status == ACAPD_ACCEL_STATUS_UNLOADED) {
		acapd_perror("%s: accel is not loaded.\n", __func__);
		return ACAPD_ACCEL_SUCCESS;
	} else if (accel->status == ACAPD_ACCEL_STATUS_UNLOADING) {
		acapd_perror("%s: accel is unloading .\n", __func__);
		return ACAPD_ACCEL_INPROGRESS;
	} else {
		int ret;
		ret = sys_close_accel(accel);
		if (ret < 0) {
			acapd_perror("%s: failed to close accel.\n", __func__);
			return ACAPD_ACCEL_FAILURE;
		}
		ret = sys_needs_load_accel(accel);
		if (ret == 0) {
			acapd_debug("%s: no need to load accel.\n", __func__);
			accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
			return ACAPD_ACCEL_SUCCESS;
		} else {
			if (accel->type == SIHA_SHELL) {
				ret = acapd_shell_assert_isolation(accel);
				if (ret < 0) {
					acapd_perror("%s, failed to assert isolaction.\n",
				             __func__);
					return ret;
				}
			}
			ret = sys_remove_accel(accel, async);
		}
		if (ret == ACAPD_ACCEL_SUCCESS) {
			acapd_debug("%s:Succesfully removed accel\n",__func__);
			accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
		} else if (ret == ACAPD_ACCEL_INPROGRESS) {
			accel->status = ACAPD_ACCEL_STATUS_UNLOADING;
		} else {
			accel->status = ACAPD_ACCEL_STATUS_UNLOADING;
		}
		return ret;
	}
}

int acapd_accel_wait_for_data_ready(acapd_accel_t *accel)
{
	/* TODO: always ready */
	(void)accel;
	return 1;
}

int acapd_accel_open_channel(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	acapd_assert(accel->chnls != NULL);
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_chnl_t *chnl = NULL;
		int ret;

		chnl = &accel->chnls[i];
		acapd_debug("%s: opening chnnl\n", __func__);
		ret = acapd_dma_open(chnl);
		if (ret < 0) {
			acapd_perror("%s: failed to open channel.\n", __func__);
			return ret;
		}
	}
	return 0;
}

/* This function only reset AXIS DMA channel not the accelerator
 */
int acapd_accel_reset_channel(acapd_accel_t *accel)
{
	int ret;

	acapd_assert(accel != NULL);
	acapd_assert(accel->chnls != NULL);
	ret = acapd_accel_open_channel(accel);
	if (ret < 0) {
		acapd_perror("%s: open channel fails.\n", __func__);
		return ret;
	}
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_chnl_t *chnl = NULL;

		chnl = &accel->chnls[i];
		acapd_dma_stop(chnl);
		acapd_dma_reset(chnl);
	}
	return 0;
}

void *acapd_accel_get_reg_va(acapd_accel_t *accel, const char *name)
{
	acapd_device_t *dev = NULL;

	acapd_assert(accel != NULL);
	if (name == NULL) {
		acapd_perror("Enter a non-empty device name.\n");
		return NULL;
	} else {
		for (int i = 0; i < accel->num_ip_devs; i++) {
			if(!strcmp(accel->ip_dev[i].dev_name, name)){
				dev = &accel->ip_dev[i];
				break;
			}
		}
	}
	if (dev == NULL) {
		acapd_perror("%s: failed to find %s device.\n", __func__, name);
		return NULL;
	}
	if (dev->va == NULL) {
		int ret;
		ret = acapd_device_open(dev);
		if (ret < 0) {
			acapd_perror("%s: failed to open dev %s.\n",
			     __func__, dev->dev_name);
			return NULL;
		}
	}
	return dev->va;
}

void sendBuffer(uint64_t size,int socket)
{
	acapd_assert(size);
	sys_send_buff(size, socket);
}
void allocateBuffer(uint64_t size)
{
	acapd_assert(size);
	sys_alloc_buffer(size);
}
void freeBuffer(uint64_t pa)
{
	sys_free_buffer(pa);
}
void get_fds(acapd_accel_t *accel, int slot, int socket){
	acapd_assert(accel != NULL);
	sys_get_fds(accel, slot, socket);
}
void get_shell_fd(int socket)
{
	sys_get_fd(acapd_shell_fd(),socket);	
}

void get_shell_clock_fd(int socket)
{
	sys_get_fd(acapd_shell_clock_fd(),socket);
}

char *get_accel_path(const char *name)
{
    char *base_path = malloc(sizeof(char)*1024);
    char *slot_path = malloc(sizeof(char)*1024);
    char *accel_path = malloc(sizeof(char)*1024);
    DIR *d, *base_d,*accel_d;
    struct dirent *dir, *base_dir,*accel_dir;
    struct stat info;

    d = opendir(FIRMWARE_PATH);
    if (d == NULL) {
        acapd_perror("Directory %s not found\n",FIRMWARE_PATH);
    }
    while((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            sprintf(base_path,"%s/%s", FIRMWARE_PATH, dir->d_name);
			base_d = opendir(base_path);
			while((base_dir = readdir(base_d)) != NULL) {
				if (base_dir->d_type == DT_DIR && !strcmp(base_dir->d_name, name)) {
					sprintf(accel_path,"%s/%s", base_path, base_dir->d_name);
					accel_d = opendir(accel_path);
					while((accel_dir = readdir(accel_d)) != NULL) {
						if (!strcmp(accel_dir->d_name, ".") || !strcmp(accel_dir->d_name, ".."))
							continue;
						if (accel_dir->d_type == DT_DIR) {
							sprintf(slot_path,"%s/%s", accel_path, accel_dir->d_name);
							if (stat(slot_path,&info) != 0)
								return NULL;
							if (info.st_mode & S_IFDIR){
								acapd_debug("Reading accel.json from %s\n",slot_path);
								goto out;
							}
						}
					}
				}
			}
		}
	}
    return NULL;
out:
	free(base_path);
	free(accel_path);
	return slot_path;
}

char * getAccelMetadata(char *package_name){
	char *filename = malloc(sizeof(char)*1024);
	char *path;
	FILE *fptr;
    long numBytes;
	char *jsonData;
	int ret;

	path = get_accel_path(package_name);
	if (path == NULL) {
		acapd_perror("No accel.json found for %s\n",package_name);
		return NULL;
	}
	sprintf(filename,"%s/accel.json",path);
	fptr = fopen(filename, "r");
    if (fptr == NULL){
        acapd_debug("%s: Cannot open accel.json, %s\n",__func__,strerror(errno));
        return NULL;
    }
	fseek(fptr, 0L, SEEK_END);
    numBytes = ftell(fptr);
    fseek(fptr, 0L, SEEK_SET);

    jsonData = (char *)calloc(numBytes, sizeof(char));
    if (jsonData == NULL)
        return NULL;
    ret = fread(jsonData, sizeof(char), numBytes, fptr);
    if (ret < numBytes)
        acapd_perror("%s: Error reading Accel.json\n",__func__);
    fclose(fptr);

	return jsonData;
}
