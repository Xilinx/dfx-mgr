/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/json-config.h>
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
#include <zynq_ioctl.h>
#include "generic-device.h"

#define MAX_BUFFERS 40
acapd_buffer_t *buffer_list = NULL;

static int remove_directory(const char *path)
{
	DIR *d = opendir(path);
	int r = -1;

	DFX_DBG("%s", path);
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
				DFX_ERR("Failed to allocate memory");
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

	DFX_DBG("%s", pkg ? pkg->path : "");
	env_config_path = getenv("ACCEL_CONFIG_PATH");
	memset(config_path, 0, sizeof(config_path));
	if(env_config_path == NULL) {
		if(pkg->type == ACAPD_ACCEL_PKG_TYPE_TAR_GZ) {
			DFX_DBG("Found .tar.gz package, extracting %s",
				pkg->path);
			tmp_dirname = mkdtemp(template);
			if (tmp_dirname == NULL) {
				DFX_ERR("mkdtemp");
				return ACAPD_ACCEL_FAILURE;
			}
			sprintf(accel->sys_info.tmp_dir, "%s/", tmp_dirname);
			sprintf(cmd, "tar -C %s -xzf %s", tmp_dirname, pkg->path);
			ret = system(cmd);
			if (ret != 0) {
				DFX_ERR("%s", cmd);
				return ACAPD_ACCEL_FAILURE;
			}
			len = sizeof(config_path) - strlen("accel.json") - 1;
			if (len > strlen(accel->sys_info.tmp_dir)) {
				len = strlen(accel->sys_info.tmp_dir);
			} else {
				DFX_ERR("path is too long: %s", tmp_dirname);
				return ACAPD_ACCEL_FAILURE;
			}
		}
		else if(pkg->type == ACAPD_ACCEL_PKG_TYPE_NONE) {
			DFX_DBG("No need to extract pkg %s", pkg->path);
			sprintf(accel->sys_info.tmp_dir, "%s/", pkg->path);
			len = strlen(accel->sys_info.tmp_dir);
		}
		else {
			DFX_ERR("unhandled package type for accel %s",
				pkg->path);
			return ACAPD_ACCEL_FAILURE;
		}
		strncpy(config_path, accel->sys_info.tmp_dir, len);
		strcat(config_path, "accel.json");
	} else {
		if (sizeof(config_path) < strlen(env_config_path)) {
			DFX_ERR("path is too long: %s", env_config_path);
			return ACAPD_ACCEL_FAILURE;
		}
		strncpy(config_path, env_config_path, sizeof(config_path));
	}
	DFX_DBG("Json Path %s", config_path);
	parseAccelJson(accel, config_path);
	if (sys_needs_load_accel(accel) == 0) {
		for (int i = 0; i < accel->num_ip_devs; i++) {
			char *tmppath;
			char tmpstr[32];
			acapd_device_t *dev;

			dev = &(accel->ip_dev[i]);
			sprintf(tmpstr, "ACCEL_IP%d_PATH", i);
			tmppath = getenv(tmpstr);
			if (tmppath == NULL)
				break;

			if (strlen(tmppath) >= sizeof(dev->path)) {
				DFX_ERR("%s is too long: %s", tmpstr, tmppath);
				return ACAPD_ACCEL_FAILURE;
			}
			strncpy(dev->path, tmppath, sizeof(dev->path));
		}
	}
	return ACAPD_ACCEL_SUCCESS;
}

int sys_needs_load_accel(acapd_accel_t *accel)
{
	char *tmpstr;

	(void)accel;
	tmpstr = getenv("ACCEL_CONFIG_PATH");
	DFX_DBG("ACCEL_CONFIG_PATH %s", tmpstr);
	if (tmpstr != NULL) {
		DFX_DBG("no need to load");
		return 0;
	} else {
		return 1;
	}
}

int sys_fetch_accel(acapd_accel_t *accel, int flags)
{
	int ret;

	DFX_DBG("");
	acapd_assert(accel != NULL);
	ret = dfx_cfg_init(accel->sys_info.tmp_dir, 0, flags);
	if (ret < 0) {
		DFX_ERR("Failed to initialize fpga config, %d", ret);
		return ACAPD_ACCEL_FAILURE;
	}
	accel->sys_info.fpga_cfg_id = ret;
	return ACAPD_ACCEL_SUCCESS;
}

int sys_load_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret;
	int fpga_cfg_id;
	(void)async;

	DFX_DBG("");
	acapd_assert(accel != NULL);
	if (accel->is_cached == 0) {
		DFX_ERR("accel is not cached.");
		return ACAPD_ACCEL_FAILURE;
	}
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	ret = dfx_cfg_load(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to load fpga config: %d", fpga_cfg_id);
		dfx_cfg_destroy(fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	}
	if ( !strcmp(accel->type,"XRT_FLAT") || !strcmp(accel->type, "PL_FLAT")) {
		DFX_PR("Successfully loaded base design.");
		return ACAPD_ACCEL_SUCCESS;
	}
	for (int i = 0; i < accel->num_ip_devs; i++) {
		int ret;
		ret = acapd_device_open(&accel->ip_dev[i]);
		if (ret != 0) {
			DFX_ERR("failed to open accel ip %s",
				accel->ip_dev[i].dev_name);
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

	DFX_DBG("");
	sprintf(cmd,"docker run --ulimit memlock=67108864:67108864 --rm -v /usr/lib:/x_usrlib -v /usr/bin/:/xbin/ -v /lib/:/xlib -v %s:%s ",accel->sys_info.tmp_dir,accel->sys_info.tmp_dir);
	for (int i = 0; i < accel->num_ip_devs; i++) {
		//int ret;
		char tmpstr[512];

		//ret = acapd_device_open(&accel->ip_dev[i]);
		//if (ret != 0) {
		//	DFX_ERR("%s: failed to open accel ip %s.\n",
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
			DFX_ERR("failed to open chnl dev %s",
				accel->chnls[i].dev->dev_name);
			return -EINVAL;
		}
	}

	sprintf(tmpstr, "%s/container.tar", accel->sys_info.tmp_dir);
	if (access(tmpstr, F_OK) != 0) {
		DFX_DBG("no need to launch container");
		return 0;
	}
	sprintf(tmpstr,"docker load < %s/container.tar",accel->sys_info.tmp_dir);
	DFX_DBG("Loading docker container");
	ret = system(tmpstr);

	sprintf(tmpstr," -e \"ACCEL_CONFIG_PATH=%s/accel.json\"",accel->sys_info.tmp_dir);
	strcat(cmd, tmpstr);
	strcat(cmd, " -it container");
	DFX_DBG("docker run cmd: %s", cmd);
	ret = system(cmd);
	return ret;
}

int sys_close_accel(acapd_accel_t *accel)
{
	DFX_DBG("");
	/* Close devices and free memory */
	acapd_assert(accel != NULL);
	for (int i = 0; i < accel->num_chnls; i++) {
		DFX_DBG("closing channel %d", i);
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
		DFX_DBG("closing accel ip %d %s", i, accel->ip_dev[i].dev_name);
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
int sys_remove_base(int fpga_cfg_id)
{
	int ret;

	DFX_DBG("");
	ret = dfx_cfg_remove(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to remove accel: %d", ret);
		return -1;
	}
	ret = dfx_cfg_destroy(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to destroy accel: %d", ret);
		return -1;
	}
	return 0;
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret, fpga_cfg_id;

	DFX_DBG("");
	/* TODO: for now, only synchronous mode is supported */
	(void)async;
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	DFX_DBG("Removing accel %s", accel->sys_info.tmp_dir);
	if (accel->pkg->type == ACAPD_ACCEL_PKG_TYPE_TAR_GZ) {
		DFX_DBG("Removing tmp dir for .tar.gz");
		ret = remove_directory(accel->sys_info.tmp_dir);
		if (ret != 0) {
			DFX_ERR("Failed to remove %s", accel->sys_info.tmp_dir);
		}
	}
	if (fpga_cfg_id <= 0) {
		DFX_ERR("Invalid fpga cfg id: %d", fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	}
	ret = dfx_cfg_remove(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to remove accel: %d", ret);
		goto out;
	}
	ret = dfx_cfg_destroy(fpga_cfg_id);
	if (ret != 0) {
		DFX_ERR("Failed to destroy accel: %d", ret);
		goto out;
	}
out:
	return ACAPD_ACCEL_SUCCESS;
}
int sys_send_fds(int *fds, int num_fds, int socket)
{
	char dummy = '$';
    struct msghdr msg;
    struct iovec iov;
    char cmsgbuf[CMSG_SPACE(sizeof(int)*num_fds)];
	DFX_DBG("");
	memset(cmsgbuf, '\0',sizeof(cmsgbuf));
	int socket_d2 = accept(socket, NULL, NULL);

    if (socket_d2 < 0){
		DFX_ERR("accept(%d, NULL, NULL)", socket);
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
    cmsg->cmsg_len = CMSG_LEN(sizeof(int)*num_fds);

	memcpy((int*) CMSG_DATA(cmsg), fds, sizeof(int)*num_fds);
    int ret = sendmsg(socket_d2, &msg, 0);
    if (ret == -1) {
		DFX_ERR("send FD's failed");
		return -1;
	}
	return 0;
}
int sys_get_fds(acapd_accel_t *accel, int slot, int socket)
{
	int fd[2];

	DFX_DBG("");
	fd[0] = accel->ip_dev[2*slot].id;
	fd[1] = accel->ip_dev[2*slot+1].id;
	DFX_ERR("Daemon slot %d accel_config %d d_hls %d",
		slot, fd[0], fd[1]);
	return sys_send_fds(&fd[0], 2, socket);
}
void sys_get_fd(int fd, int socket)
{
	DFX_DBG("");
	sys_send_fds(&fd, 1, socket);
}
int sys_send_fd_pa(acapd_buffer_t *buff)
{
	char dummy = '$';
    struct msghdr msg;
    struct iovec iov;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
	DFX_DBG("");
	memset(cmsgbuf, '\0',sizeof(cmsgbuf));
	int socket_d2 = accept(buff->socket_d, NULL, NULL);
    if (socket_d2 < 0){
		DFX_ERR("accept(%d, NULL, NULL)", buff->socket_d);
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
    int ret = sendmsg(socket_d2, &msg, 0);
    if (ret == -1) {
		DFX_ERR("sendmsg");
		return -1;
	}
	DFX_PR("Server Sending fd %d PA %lu",buff->fd, buff->PA);
	ret =  write(socket_d2,&buff->PA,sizeof(uint64_t));
	if (ret == -1) {
		DFX_ERR("write. Failed to send PA for buffer");
       return -1;
	}
	//close(socket_d2);
	return 0;
}
acapd_buffer_t * sys_alloc_buffer(uint64_t size)
{
	acapd_buffer_t *buff;
	int i;

	DFX_DBG("");
	if(buffer_list == NULL) {
		buffer_list = (acapd_buffer_t *) malloc(MAX_BUFFERS * sizeof(acapd_buffer_t));
		for (i = 0; i < MAX_BUFFERS; i++) {
			buffer_list[i].PA = 0;
		}
	}
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (!buffer_list[i].PA) {
			buff = &buffer_list[i];
			break;
		}
	}
	if (i == MAX_BUFFERS){
		DFX_ERR("all buffers are full");
		return NULL;
	}

	buff->drm_fd = open("/dev/dri/renderD128", O_RDWR);
	if (buff->drm_fd < 0) {
		return NULL;
	}
	struct drm_zocl_create_bo bo = {size, 0xffffffff,
                   DRM_ZOCL_BO_FLAGS_COHERENT | DRM_ZOCL_BO_FLAGS_CMA};
	struct drm_gem_close closeInfo = {0, 0};
    if (ioctl(buff->drm_fd, DRM_IOCTL_ZOCL_CREATE_BO, &bo) < 0) {
		DFX_ERR("DRM_IOCTL_ZOCL_CREATE_BO failed");
		goto err1;
	}
	buff->handle = bo.handle;
	struct drm_zocl_info_bo boInfo = {bo.handle, 0, 0};
	if (ioctl(buff->drm_fd, DRM_IOCTL_ZOCL_INFO_BO, &boInfo) < 0) {
		DFX_ERR("DRM_IOCTL_ZOCL_INFO_BO failed");
		goto err2;
	}
	buff->PA = boInfo.paddr;
	buff->size = boInfo.size;
	DFX_PR("allocated BO size %lu paddr %lu",boInfo.size,buff->PA);

	struct drm_prime_handle bo_h = {bo.handle, DRM_RDWR, -1};
	if (ioctl(buff->drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &bo_h) < 0) {
		DFX_ERR("DRM_IOCTL_PRIME_HANDLE_TO_FD failed");
		goto err2;
	}
	buff->fd = bo_h.fd;
	return buff;
err2:
	closeInfo.handle = buff->handle;
	ioctl(buff->drm_fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
err1:
	close(buff->drm_fd);
	return NULL;
}
int sys_send_buff(uint64_t size,int socket)
{
	DFX_DBG("");
	acapd_buffer_t *buff = sys_alloc_buffer(size);
	if (buff == NULL){
		DFX_ERR("sys_alloc_buffer");
		return -1;
	}
	buff->socket_d = socket;
	sys_send_fd_pa(buff);
	return 0;
}

int sys_free_buffer(uint64_t pa){
	int i;
	DFX_DBG("");
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (buffer_list[i].PA == pa) {
			DFX_PR("Free buffer pa %lu", pa);
			DFX_PR("Free buffer size %lu", buffer_list[i].size);
			struct drm_gem_close closeInfo = {0, 0};
			closeInfo.handle = buffer_list[i].handle;
			ioctl(buffer_list[i].drm_fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
			buffer_list[i].handle = -1;
			buffer_list[i].PA = 0;
			close(buffer_list[i].fd);
			close(buffer_list[i].drm_fd);
			return 0;
		}
	}
	DFX_ERR("No buffer allocation found for pa %lu", pa);
	return -1;
}

int sys_print_buffers(){
	int i;
	DFX_DBG("");
	for (i = 0; i < MAX_BUFFERS; i++) {
		DFX_PR("buffer pa %lu", buffer_list[i].PA);
	}
	return 0;
}
