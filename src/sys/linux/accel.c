/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/print.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>
#include <libdfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "accel.h"
#include "zynq_ioctl.h"
#include "generic-device.h"
#include "json-config.h"

#define DTBO_ROOT_DIR "/sys/kernel/config/device-tree/overlays"
#define SERVER_PATH     "/tmp/server_rm"
acapd_buffer_t *buffer_list = NULL;

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

int sys_accel_config(acapd_accel_t *accel)
{
	acapd_accel_pkg_hd_t *pkg = accel->pkg;
	char template[] = "/tmp/accel.XXXXXX";
	char *tmp_dirname;
	char cmd[512];
	int ret;
	char *env_config_path, config_path[128];
	size_t len;

	env_config_path = getenv("ACCEL_CONFIG_PATH");
	memset(config_path, 0, sizeof(config_path));
	if(env_config_path == NULL) {
		if(pkg->type == ACAPD_ACCEL_PKG_TYPE_TAR_GZ) {
			acapd_debug("%s: Found .tar.gz package, extracting %s. \n",
													__func__, pkg->path);
			tmp_dirname = mkdtemp(template);
			if (tmp_dirname == NULL) {
				acapd_perror("Failed to create tmp dir for package:%s.\n",
														 strerror(errno));
				return ACAPD_ACCEL_FAILURE;
			}
			sprintf(accel->sys_info.tmp_dir, "%s/", tmp_dirname);
			sprintf(cmd, "tar -C %s -xzf %s", tmp_dirname, pkg->path);
			ret = system(cmd);
			if (ret != 0) {
				acapd_perror("Failed to extract package %s.\n", pkg->path);
				return ACAPD_ACCEL_FAILURE;
			}
			len = sizeof(config_path) - strlen("accel.json") - 1;
			if (len > strlen(accel->sys_info.tmp_dir)) {
				len = strlen(accel->sys_info.tmp_dir);
			} else {
				acapd_perror("%s: accel config path is too long.\n");
				return ACAPD_ACCEL_FAILURE;
			}
		}
		else if(pkg->type == ACAPD_ACCEL_PKG_TYPE_NONE) {
			acapd_debug("%s: No need to extract pkg %s \n",__func__,pkg->path);
			sprintf(accel->sys_info.tmp_dir, "%s/", pkg->path);
			len = strlen(accel->sys_info.tmp_dir);
		}
		else {
			acapd_perror("%s: unhandled package type for accel %s\n",
													__func__, pkg->path);
			return ACAPD_ACCEL_FAILURE;
		}
		strncpy(config_path, accel->sys_info.tmp_dir, len);
		strcat(config_path, "accel.json");
	} else {
		if (sizeof(config_path) < strlen(env_config_path)) {
			acapd_perror("%s: accel config env path is too long.\n");
			return ACAPD_ACCEL_FAILURE;
		}
		strncpy(config_path, env_config_path, sizeof(config_path));
	}
	parseAccelJson(accel, config_path);
	if (sys_needs_load_accel(accel) == 0) {
		for (int i = 0; i < accel->num_ip_devs; i++) {
			char *tmppath;
			char tmpstr[32];
			acapd_device_t *dev;

			dev = &(accel->ip_dev[i]);
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
			if (tmppath == NULL) {
				break;
			}
		}
		return ACAPD_ACCEL_SUCCESS;
	} else {
		return ACAPD_ACCEL_SUCCESS;
	}
}

int sys_needs_load_accel(acapd_accel_t *accel)
{
	char *tmpstr;

	(void)accel;
	tmpstr = getenv("ACCEL_CONFIG_PATH");
	if (tmpstr != NULL) {
		acapd_debug("%s, no need to load.\n", __func__);
		return 0;
	} else {
		return 1;
	}
}

int sys_fetch_accel(acapd_accel_t *accel, int flags)
{
	int ret;

	acapd_assert(accel != NULL);
	ret = dfx_cfg_init(accel->sys_info.tmp_dir, 0, flags);
	if (ret < 0) {
		acapd_perror("%s: Failed to initialize fpga config, %d.\n",__func__, ret);
		return ACAPD_ACCEL_FAILURE;
	}
	accel->sys_info.fpga_cfg_id = ret;
	return ACAPD_ACCEL_SUCCESS;
}

int sys_load_accel(acapd_accel_t *accel, unsigned int async, int full_bitstream)
{
	int ret;
	int fpga_cfg_id;
	(void)async;

	acapd_assert(accel != NULL);
	if (accel->is_cached == 0 && !full_bitstream) {
		acapd_perror("%s: accel is not cached.\n");
		return ACAPD_ACCEL_FAILURE;
	}
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	ret = dfx_cfg_load(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to load fpga config: %d\n",
		     fpga_cfg_id);
		dfx_cfg_destroy(fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	}
	if (accel->type == FLAT_SHELL || full_bitstream) {
		printf("Succesfully loaded base design.\n");
		return ACAPD_ACCEL_SUCCESS;
	}
	for (int i = 0; i < accel->num_ip_devs; i++) {
		int ret;
		ret = acapd_device_open(&accel->ip_dev[i]);
		if (ret != 0) {
			acapd_perror("%s: failed to open accel ip %s.\n",
				     __func__, accel->ip_dev[i].dev_name);
			return -EINVAL;
		}
	}
	return ACAPD_ACCEL_SUCCESS;
}

int sys_load_accel_post(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	char cmd[512];
	char tmpstr[512];
	int ret;

	sprintf(cmd,"docker run --ulimit memlock=67108864:67108864 --rm -v /usr/lib:/x_usrlib -v /usr/bin/:/xbin/ -v /lib/:/xlib -v %s:%s ",accel->sys_info.tmp_dir,accel->sys_info.tmp_dir);
	for (int i = 0; i < accel->num_ip_devs; i++) {
		//int ret;
		char tmpstr[512];

		//ret = acapd_device_open(&accel->ip_dev[i]);
		//if (ret != 0) {
		//	acapd_perror("%s: failed to open accel ip %s.\n",
		//		     __func__, accel->ip_dev[i].dev_name);
		//	return -EINVAL;
		//}
		sprintf(tmpstr,"--device=%s:%s ",accel->ip_dev[i].path,accel->ip_dev[i].path);
		strcat(cmd,tmpstr);
		strcat(cmd,"--device=/dev/vfio:/dev/vfio ");
	}
	for (int i = 0; i < accel->num_chnls; i++) {
		ret = acapd_generic_device_bind(accel->chnls[i].dev,
						accel->chnls[i].dev->driver);
		if (ret != 0) {
			acapd_perror("%s: failed to open chnl dev %s.\n",
				     __func__, accel->chnls[i].dev->dev_name);
			return -EINVAL;
		}
	}

	sprintf(tmpstr, "%s/container.tar", accel->sys_info.tmp_dir);
	if (access(tmpstr, F_OK) != 0) {
		acapd_debug("%s: no need to launch container.\n", __func__);
		return 0;
	}
	sprintf(tmpstr,"docker load < %s/container.tar",accel->sys_info.tmp_dir);
	acapd_debug("%s:Loading docker container\n",__func__);
	ret = system(tmpstr);

	sprintf(tmpstr," -e \"ACCEL_CONFIG_PATH=%s/accel.json\"",accel->sys_info.tmp_dir);
	strcat(cmd, tmpstr);
	strcat(cmd, " -it container");
	acapd_debug("%s: docker run cmd: %s\n",__func__,cmd);
	ret = system(cmd);
	return ret;
}

int sys_close_accel(acapd_accel_t *accel)
{
	/* Close devices and free memory */
	acapd_assert(accel != NULL);
	for (int i = 0; i < accel->num_chnls; i++) {
		acapd_debug("%s: closing channel %d.\n", __func__, i);
		acapd_dma_close(&accel->chnls[i]);
	}
	if (accel->num_chnls > 0) {
		free(accel->chnls[0].dev->dev_name);
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
		free(accel->ip_dev->dev_name);
		free(accel->ip_dev);
		accel->ip_dev = NULL;
		accel->num_ip_devs = 0;
	}
	return 0;
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret, fpga_cfg_id;

	/* TODO: for now, only synchronous mode is supported */
	(void)async;
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	acapd_debug("%s Removing accel %s\n",__func__,accel->sys_info.tmp_dir);
	if (accel->sys_info.tmp_dir != NULL && accel->pkg->type ==
									ACAPD_ACCEL_PKG_TYPE_TAR_GZ) {
		acapd_debug("%s: Removing tmp dir for .tar.gz\n",__func__);
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
	ret = dfx_cfg_remove(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to remove accel: %d.\n", ret);
		goto out;
	}
	ret = dfx_cfg_destroy(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to destroy accel: %d.\n", ret);
		goto out;
	}
out:
	return ACAPD_ACCEL_SUCCESS;
}
int sys_get_fds(acapd_accel_t *accel, int slot)
{
//	int fd[5];
//
//	fd[0] = accel->fd[0];
//	fd[1] = accel->fd[1];
//	fd[2] = accel->fd[2];
//	fd[3] = accel->ip_dev[2*slot].id;
//	fd[4] = accel->ip_dev[2*slot+1].id;
//	acapd_perror("%s Daemon slot %d mm2s %d s2mm %d config %d accel_config %d d_hls %d\n",
//												__func__,slot,fd[0],fd[1],fd[2],fd[3],fd[4]);
//	return sys_send_fds(accel, &fd[0], 5);
printf("%d %d\n",slot,accel->rm_slot);
return 0;
}

void sys_get_fd(acapd_accel_t *accel, int fd)
{
//	sys_send_fd(accel, fd);
printf("fd%d %d\n",fd,accel->rm_slot);
}

int sys_send_fd_pa(acapd_buffer_t *buff)
{
	char dummy = '$';
    struct msghdr msg;
    struct iovec iov;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
	memset(cmsgbuf, '\0',sizeof(cmsgbuf));
	int socket_d2 = accept(buff->socket_d, NULL, NULL);
    if (socket_d2 < 0){
		acapd_perror("%s failed to accept() connections ret %d",
                                            __func__, socket_d2);
		return -1;
	}
	iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);

	msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

	memcpy((int*) CMSG_DATA(cmsg), &buff->fd, sizeof(int));
	printf("server sending fd %d\n",buff->fd);
    int ret = sendmsg(socket_d2, &msg, 0);
    if (ret == -1) {
		acapd_perror("%s send FD's failed for buffer (%s)\n", __func__,
                                                       strerror(errno));
		return -1;
	}
	acapd_perror("%s server Sending PA %lu\n",__func__,buff->PA);
	ret =  write(socket_d2,&buff->PA,sizeof(uint64_t));
	if (ret == -1) {
		acapd_perror("%s Failed to send PA for buffer (%s) \n", __func__,
                                                       strerror(errno));
       return -1;
	}
	//close(socket_d2);
	return 0;
}
int sys_alloc_buffer(uint64_t size, int socket)
{
	acapd_buffer_t *buff;
	int i;

	if(buffer_list == NULL) {
		buffer_list = (acapd_buffer_t *) malloc(20 * sizeof(acapd_buffer_t));
		for (i = 0; i < 20; i++) {
			buffer_list[i].PA = 0;
		}
	}
	for (i = 0; i < 20; i++) {
		if (!buffer_list[i].PA) {
			buff = &buffer_list[i];
			break;
		}
	}
	if (i == 20){
		acapd_perror("all buffers are full\n");
		return -1;
	}

	buff->socket_d = socket;
	buff->drm_fd = open("/dev/dri/renderD128", O_RDWR);
	if (buff->drm_fd < 0) {
		return -1;
	}
	struct drm_zocl_create_bo bo = {size, 0xffffffff,
                   DRM_ZOCL_BO_FLAGS_COHERENT | DRM_ZOCL_BO_FLAGS_CMA};
	struct drm_gem_close closeInfo = {0, 0};
    if (ioctl(buff->drm_fd, DRM_IOCTL_ZOCL_CREATE_BO, &bo) < 0) {
		acapd_perror("%s:DRM_IOCTL_ZOCL_CREATE_BO failed: %s\n",
                                               __func__,strerror(errno));
		goto err1;
	}
	buff->handle = bo.handle;
	struct drm_zocl_info_bo boInfo = {bo.handle, 0, 0};
	if (ioctl(buff->drm_fd, DRM_IOCTL_ZOCL_INFO_BO, &boInfo) < 0) {
		acapd_perror("%s:DRM_IOCTL_ZOCL_INFO_BO failed: %s\n",
                                               __func__,strerror(errno));
		goto err2;
	}
	buff->PA = boInfo.paddr;
	acapd_print("aloocated BO size %lu paddr %lu\n",boInfo.size,buff->PA);

	struct drm_prime_handle bo_h = {bo.handle, DRM_RDWR, -1};
	if (ioctl(buff->drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &bo_h) < 0) {
		acapd_perror("%s:DRM_IOCTL_PRIME_HANDLE_TO_FD failed: %s\n",
                                                   __func__,strerror(errno));
		return -1;
	}
	buff->fd = bo_h.fd;
	sys_send_fd_pa(buff);
	return 0;
err2:
	closeInfo.handle = buff->handle;
	ioctl(buff->drm_fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
err1:
	close(buff->drm_fd);
	return -1;
}

int sys_free_buffer(uint64_t pa){
	int i;
	for (i = 0; i < 20; i++) {
		if (buffer_list[i].PA == pa) {
			acapd_print("Free buffer pa %lu \n",pa);
			struct drm_gem_close closeInfo = {0, 0};
			closeInfo.handle = buffer_list[i].handle;
			ioctl(buffer_list[i].drm_fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
			buffer_list[i].handle = -1;
			buffer_list[i].PA = 0;
			return 0;
		}
	}
	acapd_perror("No buffer allocation found for pa %lu\n",pa);
	return -1;
}
