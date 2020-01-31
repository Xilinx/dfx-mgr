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
	acapd_device_t dma_dev;

	memset(&dma_dev, 0, sizeof(dma_dev));

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

	for(i=1; i < ret; i++){
		if (token[i].type == JSMN_OBJECT)
			continue;
		if (jsoneq(jsonData, &token[i],"accel_devices") == 0){
			int j;
			int numDevices = token[i+1].size;
			acapd_device_t devs[numDevices];
			memset(devs, 0, sizeof(devs));
			i+=2;

			for(j=0; j < numDevices; j++){
				if (jsoneq(jsonData, &token[i+j+1],"dev_name") == 0)
					devs[j].dev_name = strndup(jsonData+token[i+j+2].start, token[i+j+2].end - token[i+j+2].start);
				if (jsoneq(jsonData, &token[i+j+3],"reg_base") == 0)
					devs[j].reg_pa = (uint64_t)atoi(strndup(jsonData+token[i+j+4].start, token[i+j+4].end - token[i+j+4].start));
				if (jsoneq(jsonData, &token[i+j+5],"reg_size") == 0)
					devs[j].reg_size = (size_t)atoi(strndup(jsonData+token[i+j+2].start, token[i+j+6].end - token[i+j+6].start));
				if (jsoneq(jsonData, &token[i+j+7],"dev_path") == 0)
					devs[j].path = strndup(jsonData+token[i+j+8].start, token[i+j+8].end - token[i+j+8].start);
				i+=8;
			}
			accel->ip_dev = devs;
		}
		if (jsoneq(jsonData, &token[i],"accel_reg_base") == 0){}
		if (jsoneq(jsonData, &token[i],"accel_reg_size") == 0){}
		if (jsoneq(jsonData, &token[i],"sizeInKB")== 0){}
		if (jsoneq(jsonData, &token[i],"sharedMemType") == 0){}
		if (jsoneq(jsonData, &token[i],"dma_dev_name") == 0)
			dma_dev.dev_name = strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start);
		if (jsoneq(jsonData, &token[i],"dma_driver") == 0)
			dma_dev.driver = strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start);
		if (jsoneq(jsonData, &token[i],"dma_reg_base") == 0){}
		if (jsoneq(jsonData, &token[i],"iommu_group") == 0)
			dma_dev.iommu_group = atoi(strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start));
		if (jsoneq(jsonData, &token[i],"Bus") == 0){}
		if (jsoneq(jsonData, &token[i],"HWType") == 0){}
		if (jsoneq(jsonData, &token[i],"dataMoverCacheCoherent") == 0){}
		if (jsoneq(jsonData, &token[i],"dataMoverVirtualAddress") == 0){}
		if (jsoneq(jsonData, &token[i],"dataMoverChnls") == 0){
			int j;
			int numChnls = token[i+1].size;
			acapd_chnl_t chnls[numChnls];
			memset(chnls, 0, sizeof(chnls));

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
				chnls[j].dev = &dma_dev;
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
}

int parseShellJson(acapd_accel_t *accel)
{
	FILE *fp;
	long numBytes;
	jsmn_parser parser;
	jsmntok_t token[128];
	int ret,i;
	char filename[128];
	char *jsonData;

	strcpy(filename, accel->sys_info.tmp_dir);
	strcat(filename, "shell.json");
	printf("Reading shell.json %s\n",filename);

	fp = fopen(filename, "r");
	if (fp == NULL)
		return -1;
	fseek(fp, 0L, SEEK_END);
	numBytes = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	jsonData = (char *)calloc(numBytes, sizeof(char));
	if (jsonData == NULL)
		return -1;
	fread(jsonData, sizeof(char), numBytes, fp);
	fclose(fp);
	printf("jsonData read:\n %s\n",jsonData);

	jsmn_init(&parser);
	ret = jsmn_parse(&parser, jsonData, numBytes, token, sizeof(token)/sizeof(token[0]));
	if (ret < 0){
		printf("Failed to parse JSON: %d\n", ret);
	}
	for(i=1; i < ret; i++){
		if (token[i].type == JSMN_OBJECT)
			continue;
		if(jsoneq(jsonData, &token[i],"device_name") == 0)
			accel->shell_dev->dev_name = strndup(jsonData+token[i+1].start, token[i+1].end - token[i+1].start);
		if(jsoneq(jsonData, &token[i],"register_addr") == 0){}
		if(jsoneq(jsonData, &token[i],"size") == 0){}
		if(jsoneq(jsonData, &token[i],"numPL_AccelSlots") == 0){}
		if(jsoneq(jsonData, &token[i],"numAIE_AccelSlots") == 0){}
		if(jsoneq(jsonData, &token[i],"dataMoverNum") == 0){}
		if(jsoneq(jsonData, &token[i],"dataMoverType") == 0){}
	}

	free(jsonData);
	return 0;
}

int sys_load_accel(acapd_accel_t *accel, unsigned int async)
{
	acapd_accel_pkg_hd_t *pkg;
	char template[] = "/tmp/accel.XXXXXX";
	char *tmp_dirname;
	char cmd[512];
	int ret;
	int fpga_cfg_id;
	char *pkg_name;

	/* TODO: only support synchronous mode for now */
	(void)async;
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
	parseAccelJson(accel);
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
