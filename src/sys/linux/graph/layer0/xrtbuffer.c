/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "zynq_ioctl.h"
#include <stdio.h>
#include "uio.h"
#include "xrtbuffer.h"
#include "debug.h"

//static plDevices pldevices;
//static Buffers buffers;
//static int slot = 0;
int xrt_allocateBuffer(int drm_fd, int size, int* handle, uint8_t** ptr, unsigned long* paddr, int* fd){ 
	struct drm_zocl_create_bo info1 = {size, 0xffffffff, DRM_ZOCL_BO_FLAGS_COHERENT | DRM_ZOCL_BO_FLAGS_CMA};
	int result = ioctl(drm_fd, DRM_IOCTL_ZOCL_CREATE_BO, &info1);
	*handle = info1.handle;

	
	struct drm_zocl_info_bo infoInfo1 = {info1.handle, 0, 0};
	result = ioctl(drm_fd, DRM_IOCTL_ZOCL_INFO_BO, &infoInfo1);
	if(result < 0){
		printf( "error @ drm_zocl_info_bo\n");
		return result;
	}

	//struct drm_prime_handle mm2s_h = {mm2s.handle, DRM_RDWR, -1};
	//result = ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &mm2s_h);
	//if (result) {
	//		acapd_perror("%s MM2S DRM_IOCTL_PRIME_HANDLE_TO_FD failed\n",__func__);
	//}	

	struct drm_zocl_map_bo mapInfo1 = {info1.handle, 0, 0};
	result = ioctl(drm_fd, DRM_IOCTL_ZOCL_MAP_BO, &mapInfo1);
	if(result < 0){
		printf( "error @ drm_zocl_map_bo\n");
		return result;
	}

	*ptr = mmap(0, info1.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mapInfo1.offset);
	*paddr = infoInfo1.paddr;
	printf("%lx\n", infoInfo1.paddr);

	struct drm_prime_handle bo_h = {info1.handle, DRM_RDWR, -1};
	if (ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &bo_h) < 0) {
		//acapd_perror("%s:DRM_IOCTL_PRIME_HANDLE_TO_FD failed: %s\n",
                //                                 __func__,strerror(errno));
		return -1;
	}

	*fd = bo_h.fd;
	return 0;
}

int xrt_deallocateBuffer(int drm_fd, int handle){
        struct drm_gem_close closeInfo = {handle, 0};
        int result = ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
        if(result < 0){
                printf( "error @ close handle\n");
                return result;
        }
        return 0;
}

int mapBuffer(int fd, int size, uint8_t** ptr){
	*ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	//INFO("%p \n", *ptr);
	return 0;
}
int unmapBuffer(int fd, int size, uint8_t** ptr){
	munmap(*ptr, size);
	_unused(fd);
	//close(fd);
	return 0;
}


int initXrtBuffer(int slot, Buffers_t* buffers){
	int status;
	if(!buffers->fd){
		buffers->fd = open("/dev/dri/renderD128",  O_RDWR);
		if (buffers->fd < 0) {
			return -1;
		}
	}
	int tmpfd;
	status = xrt_allocateBuffer(buffers->fd, buffers->config_size[slot], &buffers->config_handle[slot],
		       	&buffers->config_ptr[slot], &buffers->config_paddr[slot], &tmpfd);
        if(status < 0){
		printf( "error @ config allocation\n");
		return status;
	}
	status = xrt_allocateBuffer(buffers->fd, buffers->S2MM_size[slot], &buffers->S2MM_handle[slot],
			&buffers->S2MM_ptr[slot], &buffers->S2MM_paddr[slot], &tmpfd);
        if(status < 0){
		printf( "error @ S2MM allocation\n");
		return status;
	}
	status = xrt_allocateBuffer(buffers->fd, buffers->MM2S_size[slot], &buffers->MM2S_handle[slot],
			&buffers->MM2S_ptr[slot], &buffers->MM2S_paddr[slot], &tmpfd);
        if(status < 0){
		printf( "error @ MM2S allocation\n");
		return status;
	}

	return 0;
}


int unallocateBuffer(int fd, int handle){ 
	struct drm_gem_close closeInfo = {handle, 0};
	int result = ioctl(fd, DRM_IOCTL_GEM_CLOSE, &closeInfo);
	if(result < 0){
		printf( "error @ close handle\n");
		return result;
	}
	return 0;
}

int printBuffer(Buffers_t* buffers, int slot){
	INFO("slot                      : %d\n", slot);
	INFO("Buffers->config_ptr[slot] : %p\n", buffers->config_ptr[slot]);
	INFO("Buffers->S2MM_ptr[slot]   : %p\n", buffers->S2MM_ptr[slot]);
	INFO("Buffers->MM2S_ptr[slot]   : %p\n", buffers->MM2S_ptr[slot]);
	_unused(buffers);
	_unused(slot);
	return 0;
}

int finaliseXrtBuffer(int slot, Buffers_t* buffers){

	int status;
	status = unallocateBuffer(buffers->fd, buffers->config_handle[slot]); 
	if(status < 0){
		printf( "error @ config\n");
		return status;
	}
	status = unallocateBuffer(buffers->fd, buffers->S2MM_handle[slot]); 
	if(status < 0){
		printf( "error @ S2MM\n");
		return status;
	}
	status = unallocateBuffer(buffers->fd, buffers->MM2S_handle[slot]); 
	if(status < 0){
		printf( "error @ MM2S\n");
		return status;
	}
	if(buffers->fd && (! buffers->config_handle[2]) && (! buffers->config_handle[1]) && (! buffers->config_handle[2])){
		status = close(buffers->fd);
		if(status < 0){
			printf( "error @ FD Close\n");
			return status;
		}
	}
	return 0;
}

