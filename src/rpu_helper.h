/*
 * Copyright (C) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	rpu_helper.h
 * @brief	rpu helper types functions definition.
 */

#ifndef _ACAPD_RPU_HELPER_H
#define _ACAPD_RPU_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <linux/rpmsg.h>
#include <sys/ioctl.h>
#include <dfx-mgr/print.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/shell.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/helper.h>
#include <dfx-mgr/print.h>


#define RPMSG_BUS_SYS "/sys/bus/rpmsg"


/* Function taken from openamp application */
int app_rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo);


/* Function taken from openamp application */
char *get_rpmsg_ept_dev_name(const char *rpmsg_char_name,
		const char *ept_name,
		char *ept_dev_name);


/* Function taken from openamp application */
int bind_rpmsg_chrdev(const char *rpmsg_dev_name);


/* Function taken from openamp application */
int get_rpmsg_chrdev_fd(const char *rpmsg_dev_name, char *rpmsg_ctrl_name);


/* Function taken from openamp application */
void set_src_dst(char *out, struct rpmsg_endpoint_info *pep);

/**
 *
 * setup_rpmsg_ept_dev - To setup rpmsg end point dev
 * @rpmsg_dev_name - rpmsg device name
 * @rpmsg_ctrl_dev_name - rpmsg control device name
 * @ept_dev_name - rpmsg end point device (output parameter)
 *
 * This function sets up rpmsg endpoint device
 *
 * return - 0 on success
 * 	    -1 on failure
 * */
int setup_rpmsg_ept_dev(char *rpmsg_dev_name, char *rpmsg_ctrl_dev_name, char *ept_dev_name);


/**
 *
 * create_rpmsg_dev -
 * @rpmsg_dev_name - rpmsg device name
 * @rpmsg_ctrl_dev - rpmsg control device name
 *
 * This function creates an rpmsg_dev node with rpmsg_channel_name and
 * ept_rpmsg_dev_name
 *
 * return - *rpmsg_dev_t on success
 * 	    NULL on failure
 * */
rpmsg_dev_t* create_rpmsg_dev(char *rpmsg_dev_name, char *rpmsg_ctrl_dev);

/**
 *
 * reset_active - reset active flag
 * @acapd_list_t *rpmsg_dev_list -- list to rpmsg_dev
 *
 * This function resets active flag from all dev in list
 *
 * return - void
 *
 * */
void reset_active(acapd_list_t *rpmsg_dev_list);

/**
 *
 * check_rpmsg_dev_active - check if rpmsg dev is recorded
 * @acapd_list_t *rpmsg_dev_list -- list to rmsg_dev
 * @rpmsg_dev_name -- rpmsg dev name
 * @virtio_num -- virtio number
 *
 * This function checks if new rpmsg_dev is already recorded and sets the active
 * flag
 *
 * return - 0 - if not found
 * 	    1 - if found
 *
 * */
int check_rpmsg_dev_active(acapd_list_t *rpmsg_dev_list, char* rpmsg_dev_name, int virtio_num);


/**
 *
 * delete_inactive_rpmsgdev - delete inactive nodes from list
 * @acapd_list_t *rpmsg_dev_list -- list to rmsg_dev
 *
 * This function deletes all the inactive nodes from list
 *
 * return - void
 *
 * */
void delete_inactive_rpmsgdev(acapd_list_t *rpmsg_dev_list);

#ifdef __cplusplus
}
#endif

#endif /*  _ACAPD_RPU_HELPER_H */
