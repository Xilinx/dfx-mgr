/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <acapd/accel.h>
#include <acapd/assert.h>
#include <acapd/print.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>
#include <libfpga.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "accel.h"
#include "jsmn.h"

#define DTBO_ROOT_DIR "/sys/kernel/config/device-tree/overlays"

static int remove_directory(const char *path)
{
	DIR *d = opendir(path);
	int r = -1;

	if (d) {
		struct dirent *p;
		size_t path_len;

		path_len = strlen(path);
		r = 0;
		while (!r && (p=readdir(d))) {
			int r2 = -1;
			char *buf;
			size_t len;
			struct stat statbuf;

			/* Skip the names "." and ".." as we don't want
			 * to recurse on them. */
			if (!strcmp(p->d_name, ".") ||
			    !strcmp(p->d_name, "..")) {
				continue;
			}
			len = path_len + strlen(p->d_name) + 2;
			buf = malloc(len);
			if (buf == NULL) {
				acapd_perror("Failed to allocate memory.\n");
				return -1;
			}

			sprintf(buf, "%s/%s", path, p->d_name);
			if (!stat(buf, &statbuf)) {
				if (S_ISDIR(statbuf.st_mode)) {
					r2 = remove_directory(buf);
				} else {
					r2 = unlink(buf);
				}
			}
			r = r2;
			free(buf);
		}
		closedir(d);
		if (r == 0) {
			r = rmdir(path);
		}
	}
	return r;
}
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

int parseAccelJson(acapd_accel_t *accel)
{
	FILE *fptr;
	long numBytes;
	jsmn_parser parser;
	jsmntok_t token[128];
	int ret,i;
	char filename[128]={0};
	char *jsonData;
	acapd_device_t *dma_dev;

	dma_dev = (acapd_device_t *)calloc(1, sizeof(*dma_dev));
	if (dma_dev == NULL) {
		acapd_perror("%s: failed to alloc mem for dma dev.\n", __func__);
		return -EINVAL;
	}

	strcpy(filename, accel->sys_info.tmp_dir);
	strcat(filename, "accel.json");
	printf("Reading accel.json %s\n",filename);

	fptr = fopen(filename, "r");
	if (fptr == NULL)
		return -1;
	fseek(fptr, 0L, SEEK_END);
	numBytes = ftell(fptr);
	fseek(fptr, 0L, SEEK_SET);

	jsonData = (char *)calloc(numBytes, sizeof(char));
	if (jsonData == NULL)
		return -1;
	fread(jsonData, sizeof(char), numBytes, fptr);
	fclose(fptr);
	printf("jsonData read:\n %s\n",jsonData);

	jsmn_init(&parser);
	ret = jsmn_parse(&parser, jsonData, sizeof(jsonData), token, sizeof(token)/sizeof(token[0]));
	if (ret < 0){
		printf("Failed to parse JSON: %d\n", ret);
	}

	accel->config_from_file = 1;
	for(i=1; i < ret; i++){
		if (token[i].type == JSMN_OBJECT)
			continue;
		if (jsoneq(jsonData, &token[i],"accel_devices") == 0){
			int j;
			int numDevices = token[i+1].size;
			acapd_device_t *devs;
			devs = (acapd_device_t *)calloc(numDevices, sizeof(*devs));
			if (devs == NULL) {
				acapd_perror("%s: Failed to alloc mem for accel devs.\n", __func__);
				ret = -ENOMEM;
				goto error;
			}
			i+=2;

			accel->num_ip_devs = numDevices;
			for(j=0; j < numDevices; j++){
				if (jsoneq(jsonData, &token[i+j+1],"dev_name") == 0)
					devs[j].dev_name = strndup(jsonData+token[i+j+2].start, token[i+j+2].end - token[i+j+2].start);
				if (jsoneq(jsonData, &token[i+j+3],"reg_base") == 0)
					devs[j].reg_pa = (uint64_t)atoi(strndup(jsonData+token[i+j+4].start, token[i+j+4].end - token[i+j+4].start));
				if (jsoneq(jsonData, &token[i+j+5],"reg_size") == 0)
					devs[j].reg_size = (size_t)atoi(strndup(jsonData+token[i+j+6].start, token[i+j+6].end - token[i+j+6].start));
				i+=6;
			}
			accel->ip_dev = devs;
		}
		if (jsoneq(jsonData, &token[i],"accel_reg_base") == 0){}
		if (jsoneq(jsonData, &token[i],"accel_reg_size") == 0){}
		if (jsoneq(jsonData, &token[i],"sizeInKB")== 0){}
		if (jsoneq(jsonData, &token[i],"sharedMemType") == 0){}
		if (jsoneq(jsonData, &token[i],"dma_dev_name") == 0)
			dma_dev->dev_name = strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start);
		if (jsoneq(jsonData, &token[i],"dma_driver") == 0)
			dma_dev->driver = strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start);
		if (jsoneq(jsonData, &token[i],"dma_reg_base") == 0){}
		if (jsoneq(jsonData, &token[i],"iommu_group") == 0)
			dma_dev->iommu_group = atoi(strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start));
		if (jsoneq(jsonData, &token[i],"Bus") == 0){}
		if (jsoneq(jsonData, &token[i],"HWType") == 0){}
		if (jsoneq(jsonData, &token[i],"dataMoverCacheCoherent") == 0){}
		if (jsoneq(jsonData, &token[i],"dataMoverVirtualAddress") == 0){}
		if (jsoneq(jsonData, &token[i],"dataMoverChnls") == 0){
			int j;
			int numChnls = token[i+1].size;
			acapd_chnl_t *chnls;

			chnls = (acapd_chnl_t *)calloc(numChnls, sizeof(*chnls));
			if (chnls == NULL) {
				acapd_perror("%s: failed to alloc mem for chnls.\n", __func__);
				ret = -ENOMEM;
				goto error;
			}

			i+=2;
			for(j=0; j < numChnls; j++){
				if (jsoneq(jsonData, &token[i+j+1],"chnl_id") == 0){
					chnls[j].chnl_id = atoi(strndup(jsonData+token[i+j+2].start, token[i+j+2].end - token[i+j+2].start));
				}
				if (jsoneq(jsonData, &token[i+j+3],"chnl_dir") == 0){
					char *dir = strndup(jsonData+token[i+j+4].start, token[i+j+4].end - token[i+j+4].start);
					if (!strcmp(dir,"ACAPD_DMA_DEV_R"))
						chnls[j].dir = ACAPD_DMA_DEV_R;
					else if (!strcmp(dir,"ACAPD_DMA_DEV_W"))
						chnls[j].dir = ACAPD_DMA_DEV_W;
					else if (!strcmp(dir,"ACAPD_DMA_DEV_RW"))
						chnls[j].dir = ACAPD_DMA_DEV_RW;
				}
				chnls[j].dev = dma_dev;
				chnls[j].ops = &axidma_vfio_dma_ops;
				i+=4;//move token to point to next channel in array
			}
			accel->num_chnls = token[i+1].size;
			accel->chnls = chnls;
		}
		if (jsoneq(jsonData, &token[i],"AccelHandshakeType") == 0){}
		if (jsoneq(jsonData, &token[i],"fallbackBehaviour") == 0){}
	}
	return 0;
error:
	if (dma_dev != NULL) {
		free(dma_dev);
	}
	if (accel->ip_dev != NULL) {
		free(accel->ip_dev);
		accel->ip_dev = NULL;
		accel->num_ip_devs = 0;
	}
	if (accel->chnls != NULL) {
		free(accel->chnls);
		accel->chnls = NULL;
		accel->num_chnls = 0;
	}
	return ret;
}

int parseShellJson(acapd_accel_t *accel)
{
	FILE *fptr;
	long numBytes;
	jsmn_parser parser;
	jsmntok_t token[128];
	int ret,i;
	char filename[128]={0};
	char *jsonData;
	acapd_device_t *dev;

	strcpy(filename, accel->sys_info.tmp_dir);
	strcat(filename, "shell.json");
	printf("Reading shell.json %s\n",filename);

	fptr = fopen(filename, "r");
	if (fptr == NULL)
		return -1;
	fseek(fptr, 0L, SEEK_END);
	numBytes = ftell(fptr);
	fseek(fptr, 0L, SEEK_SET);

	jsonData = (char *)calloc(numBytes, sizeof(char));
	if (jsonData == NULL)
		return -1;
	fread(jsonData, sizeof(char), numBytes, fptr);
	fclose(fptr);
	printf("jsonData read:\n %s\n",jsonData);

	jsmn_init(&parser);
	ret = jsmn_parse(&parser, jsonData, sizeof(jsonData), token, sizeof(token)/sizeof(token[0]));
	if (ret < 0){
		printf("Failed to parse JSON: %d\n", ret);
	}

	dev = (acapd_device_t *)calloc(1, sizeof(*dev));
	if (dev == NULL) {
		acapd_perror("%s: failed to alloc mem.\n", __func__);
		return -ENOMEM;
	}
	accel->config_from_file = 1;
	for(i=1; i < ret; i++){
		if (token[i].type == JSMN_OBJECT)
			continue;
		if (jsoneq(jsonData, &token[i],"device_name") == 0)
			dev->dev_name = strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start);
		if (jsoneq(jsonData, &token[i],"shell_type") == 0)
			printf("Shell is %s\n",strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start));
		if (jsoneq(jsonData, &token[i],"reg_base")== 0)
			dev->reg_pa = (uint64_t)atoi(strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start));
		if (jsoneq(jsonData, &token[i],"reg_size") == 0){}
			dev->reg_size = (size_t)atoi(strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start));
	}
	accel->shell_dev = dev;
	return 0;
}

int parseRMJson(acapd_accel_t *accel)
{
	FILE *fptr;
	long numBytes;
	jsmn_parser parser;
	jsmntok_t token[128];
	int ret,i;
	char filename[128]={0};
	char *jsonData;
	acapd_device_t *dev;

	strcpy(filename, accel->sys_info.tmp_dir);
	strcat(filename, "rm.json");
	printf("Reading rm.json %s\n",filename);

	fptr = fopen(filename, "r");
	if (fptr == NULL)
		return -1;
	fseek(fptr, 0L, SEEK_END);
	numBytes = ftell(fptr);
	fseek(fptr, 0L, SEEK_SET);

	jsonData = (char *)calloc(numBytes, sizeof(char));
	if (jsonData == NULL)
		return -1;
	fread(jsonData, sizeof(char), numBytes, fptr);
	fclose(fptr);
	printf("jsonData read:\n %s\n",jsonData);

	jsmn_init(&parser);
	ret = jsmn_parse(&parser, jsonData, sizeof(jsonData), token, sizeof(token)/sizeof(token[0]));
	if (ret < 0){
		printf("Failed to parse JSON: %d\n", ret);
	}

	dev = (acapd_device_t *)calloc(1, sizeof(*dev));
	if (dev == NULL) {
		acapd_perror("%s: failed to alloc mem.\n", __func__);
		return -ENOMEM;
	}
	for(i=1; i < ret; i++){
		if (token[i].type == JSMN_OBJECT)
			continue;
		if (jsoneq(jsonData, &token[i],"rm_dev_name") == 0)
			dev->dev_name = strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start);
		if (jsoneq(jsonData, &token[i],"rm_reg_base") == 0)
			dev->reg_pa = (uint64_t)atoi(strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start));
		if (jsoneq(jsonData, &token[i],"rm_reg_size")== 0)
			dev->reg_size = (size_t)atoi(strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start));
		if (jsoneq(jsonData, &token[i],"isolation_slot") == 0){}
	}
	accel->rm_dev = dev;
	return 0;
}

int sys_accel_config(acapd_accel_t *accel)
{
	acapd_accel_pkg_hd_t *pkg;
	char template[] = "/tmp/accel.XXXXXX";
	char *tmp_dirname;
	char cmd[512];
	int ret;
	char *pkg_name;

	/* Use timestamp to name the tmparary directory */
	acapd_debug("Creating tmp dir for package.\n");
	tmp_dirname = mkdtemp(template);
	if (tmp_dirname == NULL) {
		acapd_perror("Failed to create tmp dir for package:%s.\n",
			     strerror(errno));
		return ACAPD_ACCEL_FAILURE;
	}
	sprintf(accel->sys_info.tmp_dir, "%s/", tmp_dirname);
	pkg = accel->pkg;
	pkg_name = (char *)pkg;
	/* TODO: Assuming the package is a tar.gz format */
	sprintf(cmd, "tar -C %s -xzf %s", tmp_dirname, pkg_name);
	ret = system(cmd);
	if (ret != 0) {
		acapd_perror("Failed to extract package %s.\n", pkg_name);
		return ACAPD_ACCEL_FAILURE;
	}
	parseShellJson(accel);
	parseAccelJson(accel);
	if (accel->shell_dev == NULL) {
		for (int i = 0; i < accel->num_ip_devs; i++) {
			const char *tmpname, *tmppath;
			char tmpstr[32];
			acapd_device_t *dev;

			dev = &(accel->ip_dev[i]);
			sprintf(tmpstr, "ACCEL_IP%d_NAME", i);
			tmpname = getenv(tmpstr);
			if (tmpname != NULL) {
				dev->dev_name = tmpname;
			}
			sprintf(tmpstr, "ACCEL_IP%d_PATH", i);
			tmppath = getenv(tmpstr);
			if (tmppath != NULL) {
				size_t len;
				len = sizeof(dev->path) - 1;
			    memset(dev->path, 0, len + 1);
				if (len > strlen(tmppath)) {
					len = strlen(tmppath);
				}
				strncpy(dev->path, tmppath, len);
			}
			if (tmpname == NULL && tmppath == NULL) {
				break;
			}
		}
		return ACAPD_ACCEL_SUCCESS;
	} else {
		return ACAPD_ACCEL_SUCCESS;
	}
}

int sys_load_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret;
	int fpga_cfg_id;

	(void)async;
	ret = fpga_cfg_init(accel->sys_info.tmp_dir, 0, 0);
	if (ret < 0) {
		acapd_perror("Failed to initialize fpga config, %d.\n", ret);
		return ACAPD_ACCEL_FAILURE;
	}
	fpga_cfg_id = ret;
	accel->sys_info.fpga_cfg_id = fpga_cfg_id;
	acapd_print("loading %d.\n",  fpga_cfg_id);
	ret = fpga_cfg_load(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to load fpga config: %d\n",
		     fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	} else {
		return ACAPD_ACCEL_SUCCESS;
	}
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret, fpga_cfg_id;

	/* TODO: for now, only synchronous mode is supported */
	(void)async;
	/* Close devices and free memory */

	acapd_assert(accel != NULL);
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_debug("%s: closing channel %d.\n", __func__, i);
		acapd_dma_close(&accel->chnls[i]);
	}
	if (accel->num_chnls > 0) {
		free(accel->chnls[0].dev);
		free(accel->chnls);
		accel->chnls = NULL;
		accel->num_chnls = 0;
	}
	for (int i = 0; i < accel->num_ip_devs; i++) {
		acapd_debug("%s: closing accel ip %d %s.\n", __func__, i, accel->ip_dev[i].dev_name);
		acapd_device_close(&accel->ip_dev[i]);
	}
	if (accel->num_ip_devs > 0) {
		free(accel->ip_dev);
		accel->ip_dev = NULL;
		accel->num_ip_devs = 0;
	}
	if (accel->rm_dev != NULL) {
		acapd_debug("%s: closing rm dev.\n", __func__);
		acapd_device_close(accel->rm_dev);
		free(accel->rm_dev);
		accel->rm_dev = NULL;
	}

	if (accel->shell_dev != NULL) {
		acapd_debug("%s: closing shell dev.\n", __func__);
		acapd_device_close(accel->shell_dev);
		free(accel->shell_dev);
		accel->shell_dev = NULL;
	}

	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	if (accel->sys_info.tmp_dir != NULL) {
		ret = remove_directory(accel->sys_info.tmp_dir);
		if (ret != 0) {
			acapd_perror("Failed to remove %s, %s\n",
				     accel->sys_info.tmp_dir, strerror(errno));
		}
	}
	if (fpga_cfg_id <= 0) {
		acapd_perror("Invalid fpga cfg id: %d.\n", fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	};
	ret = fpga_cfg_remove(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to remove accel: %d.\n", ret);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = fpga_cfg_destroy(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to destroy accel: %d.\n", ret);
		return ACAPD_ACCEL_FAILURE;
	}
	return ACAPD_ACCEL_SUCCESS;
}
