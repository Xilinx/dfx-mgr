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
static int socket_d[3];

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
	acapd_accel_pkg_hd_t *pkg;
	char template[] = "/tmp/accel.XXXXXX";
	char *tmp_dirname;
	char cmd[512];
	int ret;
	char *pkg_name;
	char *env_config_path, config_path[128];

	env_config_path = getenv("ACCEL_CONFIG_PATH");
	memset(config_path, 0, sizeof(config_path));
	if(env_config_path == NULL) {
		size_t len;

		/* Use timestamp to name the tmparary directory */
		acapd_debug("%s: Creating tmp dir for package.\n", __func__);
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
		len = sizeof(config_path) - strlen("accel.json") - 1;
		if (len > strlen(accel->sys_info.tmp_dir)) {
			len = strlen(accel->sys_info.tmp_dir);
		} else {
			acapd_perror("%s: accel config path is too long.\n");
			return ACAPD_ACCEL_FAILURE;
		}
		strncpy(config_path, accel->sys_info.tmp_dir, len);
		strcat(config_path, "accel.json");
	} else {
		size_t len;

		len = sizeof(config_path) - 1;
		if (len > strlen(env_config_path)) {
			len = strlen(env_config_path);
		} else {
			acapd_perror("%s: accel config env path is too long.\n");
			return ACAPD_ACCEL_FAILURE;
		}
		strncpy(config_path, env_config_path, len);
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
		acapd_debug("%s, need to load.\n", __func__);
		return 1;
	}
}

/* flags indicate 0:Full, 1:Partial */
int sys_fetch_accel(acapd_accel_t *accel, int flags)
{
	int ret;

	acapd_assert(accel != NULL);
	ret = fpga_cfg_init(accel->sys_info.tmp_dir, 0, flags);
	if (ret < 0) {
		acapd_perror("%s: Failed to initialize fpga config, %d.\n",__func__, ret);
		return ACAPD_ACCEL_FAILURE;
	}
	accel->sys_info.fpga_cfg_id = ret;
	return ACAPD_ACCEL_SUCCESS;
}

void sys_zocl_alloc_bo(acapd_accel_t *accel)
{
	accel->drm_fd = open("/dev/dri/renderD128", O_RDWR);
	if (accel->drm_fd < 0) {
		return;
	}
    
	struct drm_zocl_create_bo mm2s = {0x4000000, 0xffffffff, DRM_ZOCL_BO_FLAGS_COHERENT | DRM_ZOCL_BO_FLAGS_CMA};
	struct drm_zocl_create_bo s2mm = {0x4000000, 0xffffffff, DRM_ZOCL_BO_FLAGS_COHERENT | DRM_ZOCL_BO_FLAGS_CMA};
	struct drm_zocl_create_bo config = {4096, 0xffffffff, DRM_ZOCL_BO_FLAGS_COHERENT | DRM_ZOCL_BO_FLAGS_CMA};
    int result = ioctl(accel->drm_fd, DRM_IOCTL_ZOCL_CREATE_BO, &mm2s);
    result = ioctl(accel->drm_fd, DRM_IOCTL_ZOCL_CREATE_BO, &s2mm);
    result = ioctl(accel->drm_fd, DRM_IOCTL_ZOCL_CREATE_BO, &config);
	
	struct drm_zocl_info_bo mm2sInfo = {mm2s.handle, 0, 0};
    result = ioctl(accel->drm_fd, DRM_IOCTL_ZOCL_INFO_BO, &mm2sInfo);
    acapd_debug("m2ss BO size %lu paddr 0x%lx\n",mm2sInfo.size, mm2sInfo.paddr);
	struct drm_zocl_info_bo s2mmInfo = {s2mm.handle, 0, 0};
    result = ioctl(accel->drm_fd, DRM_IOCTL_ZOCL_INFO_BO, &s2mmInfo);
    acapd_debug("s2mm BO size %lu paddr 0x%lx\n",s2mmInfo.size, s2mmInfo.paddr);
	struct drm_zocl_info_bo configInfo = {config.handle, 0, 0};
    result = ioctl(accel->drm_fd, DRM_IOCTL_ZOCL_INFO_BO, &configInfo);
    acapd_debug("config BO size %lu paddr 0x%lx\n",configInfo.size, configInfo.paddr);

	struct drm_prime_handle mm2s_h = {mm2s.handle, DRM_RDWR, -1};
	result = ioctl(accel->drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &mm2s_h);
	if (result) {
		acapd_perror("%s MM2S DRM_IOCTL_PRIME_HANDLE_TO_FD failed %d\n",__func__,result);
	}
	struct drm_prime_handle s2mm_h = {s2mm.handle, DRM_RDWR, -1};
	result = ioctl(accel->drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &s2mm_h);
	if (result) {
		acapd_perror("%s S2MM DRM_IOCTL_PRIME_HANDLE_TO_FD failed %d\n",__func__,result);
	}
	struct drm_prime_handle config_h = {config.handle, DRM_RDWR, -1};
	result = ioctl(accel->drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &config_h);
	if (result) {
		acapd_perror("%s S2MM DRM_IOCTL_PRIME_HANDLE_TO_FD failed ret %d\n",__func__,result);
	}
	accel->fd[0] = mm2s_h.fd;
	accel->fd[1] = s2mm_h.fd;
	accel->fd[2] = config_h.fd;
	accel->handle[0] = mm2s.handle;
	accel->handle[1] = s2mm.handle;
	accel->handle[2] = config.handle;
	accel->PA[0] = mm2sInfo.paddr;
	accel->PA[1] = mm2sInfo.size;
	accel->PA[2] = s2mmInfo.paddr;
	accel->PA[3] = s2mmInfo.size;
	accel->PA[4] = configInfo.paddr;
	accel->PA[5] = configInfo.size;
	acapd_debug("%s: MM2S_FD %d S2MM_FD %d config_FD %d slot %d\n",__func__,
						accel->fd[0], accel->fd[1],accel->fd[2],accel->rm_slot);
}

int sys_load_accel(acapd_accel_t *accel, unsigned int async, int full_bitstream)
{
	int ret;
	int fpga_cfg_id;
	struct sockaddr_un serveraddr;
	char path[strlen(SERVER_PATH)+2];
	(void)async;

	acapd_assert(accel != NULL);
	if (accel->is_cached == 0 && !full_bitstream) {
		acapd_perror("%s: accel is not cached.\n");
		return ACAPD_ACCEL_FAILURE;
	}
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	ret = fpga_cfg_load(fpga_cfg_id);
	if (ret != 0) {
		acapd_perror("Failed to load fpga config: %d\n",
		     fpga_cfg_id);
		fpga_cfg_destroy(fpga_cfg_id);
		return ACAPD_ACCEL_FAILURE;
	}
	if (full_bitstream) {
		acapd_debug("%s:Loaded Full bitstream\n",__func__);
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
	sys_zocl_alloc_bo(accel);

	//Create a socket to send dmabuf FD 
	if (socket_d[accel->rm_slot] > 0)
		return ACAPD_ACCEL_SUCCESS;
	socket_d[accel->rm_slot] = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_d[accel->rm_slot] < 0)
		acapd_perror("%s socket creation failed\n",__func__);

	memset(&serveraddr, 0, sizeof(serveraddr));
	sprintf(path,"%s%d",SERVER_PATH,accel->rm_slot);
	serveraddr.sun_family = AF_UNIX;
	strcpy(serveraddr.sun_path, path);

	ret = bind(socket_d[accel->rm_slot], (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
	if (ret < 0)
		acapd_perror("%s socket bind() failed ret %d",__func__,ret);
	printf("%s socket bind done\n",__func__);

	//socket will queue upto 10 incoming connections
	ret = listen(socket_d[accel->rm_slot], 10);
	if (ret < 0)
		acapd_perror("%s socket listen() failed ret %d",__func__,ret);
	printf("%s Server started %s (desc %d) ready for client connect. \n",__func__,path,socket_d[accel->rm_slot]);

	return ACAPD_ACCEL_SUCCESS;
	
}

int sys_load_accel_post(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	char cmd[512];
	char tmpstr[512];

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
		int ret;

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
	system(tmpstr);

	sprintf(tmpstr," -e \"ACCEL_CONFIG_PATH=%s/accel.json\"",accel->sys_info.tmp_dir);
	strcat(cmd, tmpstr);
	strcat(cmd, " -it container");
	acapd_debug("%s: docker run cmd: %s\n",__func__,cmd);
	system(cmd);
	return 0;
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
	if(accel->handle[0]){
		printf("Close zocl BO handle\n");
		struct drm_gem_close closeInfo = {accel->handle[0], 0};
		ioctl(accel->drm_fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
		closeInfo.handle = accel->handle[1];
		ioctl(accel->drm_fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
		closeInfo.handle = accel->handle[1];
		ioctl(accel->drm_fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
		
	}
	if(accel->drm_fd > 0){
		close(accel->drm_fd);
		close(accel->fd[0]);
		close(accel->fd[1]);
		close(accel->fd[1]);
	}
	return 0;
}

int sys_remove_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret, fpga_cfg_id;
	char path[strlen(SERVER_PATH)+2];

	/* TODO: for now, only synchronous mode is supported */
	(void)async;
	fpga_cfg_id = accel->sys_info.fpga_cfg_id;
	printf("%s Removing accel %s\n",__func__,accel->sys_info.tmp_dir);
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
	if (socket_d[accel->rm_slot] > 0) {
		close(socket_d[accel->rm_slot]);
		socket_d[accel->rm_slot] = -1;
		sprintf(path,"%s%d",SERVER_PATH,accel->rm_slot);
		unlink(path);
		printf("%s server %s unlink done\n",__func__,path);
	}
	return ACAPD_ACCEL_SUCCESS;
}

void sys_send_fd(acapd_accel_t *accel, int fd)
{
	char dummy = '$';
    struct msghdr msg;
    struct iovec iov;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];

	int socket_d2 = accept(socket_d[accel->rm_slot], NULL, NULL);
    if (socket_d2 < 0){
		acapd_perror("%s failed to accept() connections ret %d",__func__,socket_d2);
		return;
	}
	
	iov.iov_base = &dummy;
    iov.iov_len = sizeof(dummy);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = CMSG_LEN(sizeof(int));

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    *(int*) CMSG_DATA(cmsg) = fd;

    int ret = sendmsg(socket_d2, &msg, 0);

    if (ret == -1) {
        acapd_perror("%s send FD failed for slot %d  with %s\n", __func__,
												accel->rm_slot, strerror(errno));
		return;
    }
	//close(socket_d2);
	printf("%s Send FD succesfull for slot %d\n",__func__, accel->rm_slot);
}

void sys_send_fds(acapd_accel_t *accel, int *fds, int num_fds)
{
	char dummy = '$';
    struct msghdr msg;
    struct iovec iov;
    char cmsgbuf[CMSG_SPACE(sizeof(int) * num_fds)];
	memset(cmsgbuf, '\0',sizeof(cmsgbuf));
	int socket_d2 = accept(socket_d[accel->rm_slot], NULL, NULL);
    if (socket_d2 < 0){
		acapd_perror("%s failed to accept() connections for slot %d ret %d",__func__,accel->rm_slot, socket_d2);
		return;
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
    cmsg->cmsg_len = CMSG_LEN(sizeof(int) * num_fds);

	memcpy((int*) CMSG_DATA(cmsg), fds, sizeof(int)*num_fds);

    int ret = sendmsg(socket_d2, &msg, 0);

    if (ret == -1) {
        printf("%s send FD's failed for slot %d with %s\n", __func__,accel->rm_slot, strerror(errno));
		return;
    }
	printf("%s Send FD's succesful for slot %d\n",__func__, accel->rm_slot);
	//close(socket_d2);
}

void sys_get_PA(acapd_accel_t *accel)
{
	printf("%s: slot %d mm2s PA %lx size %lx\n",__func__,accel->rm_slot, accel->PA[0],accel->PA[1]);
	int socket_d2 = accept(socket_d[accel->rm_slot], NULL, NULL);
    if (socket_d2 < 0){
		acapd_perror("%s failed to accept() connections ret %d",__func__,socket_d2);
		return;
	}
	write(socket_d2,&accel->PA,6*sizeof(uint64_t));
	//close(socket_d2);
}
void sys_get_fds(acapd_accel_t *accel, int slot)
{
	int fd[5];

	fd[0] = accel->fd[0];
	fd[1] = accel->fd[1];
	fd[2] = accel->fd[2];
	fd[3] = accel->ip_dev[2*slot].id;
	fd[4] = accel->ip_dev[2*slot+1].id;
	acapd_debug("%s Daemon slot %d mm2s %d s2mm %d config %d accel_config %d d_hls %d\n",
												__func__,slot,fd[0],fd[1],fd[2],fd[3],fd[4]);
	sys_send_fds(accel, &fd[0], 5);
}

void sys_get_fd(acapd_accel_t *accel, int fd)
{
	sys_send_fd(accel, fd);
}
