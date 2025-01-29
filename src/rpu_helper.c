/*
 * Copyright (C) 2022 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * @file	rpu_helper.c
 * @brief	rpu helper functions.
 */

#include <dfx-mgr/rpu_helper.h>

/* Function taken from openamp application */
int app_rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo)
{
	int ret;

	ret = ioctl(rpfd, RPMSG_CREATE_EPT_IOCTL, eptinfo);
	if (ret)
		perror("Failed to create endpoint.\n");
	return ret;
}


/* Function taken from openamp application */
char *get_rpmsg_ept_dev_name(const char *rpmsg_char_name,
		const char *ept_name,
		char *ept_dev_name)
{
	char sys_rpmsg_ept_name_path[64];
	char svc_name[64];
	char *sys_rpmsg_path = "/sys/class/rpmsg";
	FILE *fp;
	int i;
	long unsigned int ept_name_len;

	for (i = 0; i < 128; i++) {
		sprintf(sys_rpmsg_ept_name_path, "%s/%s/rpmsg%d/name",
				sys_rpmsg_path, rpmsg_char_name, i);
		if (access(sys_rpmsg_ept_name_path, F_OK) < 0)
			continue;
		fp = fopen(sys_rpmsg_ept_name_path, "r");
		if (!fp) {
			DFX_DBG("failed to open %s", sys_rpmsg_ept_name_path);
			break;
		}
		if ( fgets(svc_name, sizeof(svc_name), fp) == NULL){
			DFX_DBG("failed to read %s\n", svc_name);
			return NULL;//added
		}

		fclose(fp);
		ept_name_len = strlen(ept_name);

		if (ept_name_len != (strlen(svc_name)-1))
			continue;

		if (!strncmp(svc_name, ept_name, ept_name_len)) {
			sprintf(ept_dev_name, "rpmsg%d", i);
			return ept_dev_name;
		}
	}

	DFX_DBG("Not able to RPMsg endpoint file for %s:%s.",
			rpmsg_char_name, ept_name);
	return NULL;
}


/* Function taken from openamp application */
int bind_rpmsg_chrdev(const char *rpmsg_dev_name)
{
	char fpath[512];
	const char *rpmsg_chdrv = "rpmsg_chrdev";
	char drv_override[64] = {0};
	int fd;
	int ret;

	/* rpmsg dev overrides path */
	sprintf(fpath, "%s/devices/%s/driver_override",
			RPMSG_BUS_SYS, rpmsg_dev_name);
	fd = open(fpath, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s, %s\n",
				fpath, strerror(errno));
		return -EINVAL;
	}

	ret = read(fd, drv_override, sizeof(drv_override));
	if (ret < 0) {
		fprintf(stderr, "Failed to read %s (%s)\n",
				fpath, strerror(errno));
		close(fd);
		return ret;
	}

	/*
	 * Check driver override. If "rpmsg_chrdev" string is
	 * found, then don't attempt to bind. If null string is found,
	 * then no driver is bound, and attempt to bind rpmsg char driver.
	 * Any other case, fail binding driver, as device is busy.
	 */
	if (strncmp(drv_override, rpmsg_chdrv, strlen(rpmsg_chdrv)) == 0) {
		close(fd);
		return 0;
	} else if (strncmp(drv_override, "(null)", strlen("(null)")) != 0) {
		fprintf(stderr, "error: device %s is busy, drv bind=%s\n",
				rpmsg_dev_name, drv_override);
		close(fd);
		return -EBUSY;
	}

	ret = write(fd, rpmsg_chdrv, strlen(rpmsg_chdrv) + 1);
	if (ret < 0) {
		fprintf(stderr, "Failed to write %s to %s, %s\n",
				rpmsg_chdrv, fpath, strerror(errno));
		close(fd);
		return -EINVAL;
	}
	close(fd);

	/* bind the rpmsg device to rpmsg char driver */
	sprintf(fpath, "%s/drivers/%s/bind", RPMSG_BUS_SYS, rpmsg_chdrv);
	fd = open(fpath, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s, %s\n",
				fpath, strerror(errno));
		return -EINVAL;
	}
	DFX_DBG("write %s to %s", rpmsg_dev_name, fpath);
	ret = write(fd, rpmsg_dev_name, strlen(rpmsg_dev_name) + 1);
	if (ret < 0) {
		fprintf(stderr, "Failed to write %s to %s, %s\n",
				rpmsg_dev_name, fpath, strerror(errno));
		close(fd);
		return -EINVAL;
	}
	close(fd);
	return 0;
}


/* Function taken from openamp application */
int get_rpmsg_chrdev_fd(const char *rpmsg_dev_name, char *rpmsg_ctrl_name)
{
	char dpath[2*NAME_MAX];
	DIR *dir;
	struct dirent *ent;
	int fd;

	sprintf(dpath, "%s/devices/%s/rpmsg", RPMSG_BUS_SYS, rpmsg_dev_name);
	dir = opendir(dpath);
	if (dir == NULL) {
		fprintf(stderr, "opendir %s, %s\n", dpath, strerror(errno));
		return -EINVAL;
	}
	while ((ent = readdir(dir)) != NULL) {
		if (!strncmp(ent->d_name, "rpmsg_ctrl", 10)) {
			sprintf(dpath, "/dev/%s", ent->d_name);
			closedir(dir);
			fd = open(dpath, O_RDWR | O_NONBLOCK);
			if (fd < 0) {
				fprintf(stderr, "open %s, %s\n",
						dpath, strerror(errno));
				return fd;
			}
			sprintf(rpmsg_ctrl_name, "%s", ent->d_name);
			return fd;
		}
	}

	fprintf(stderr, "No rpmsg_ctrl file found in %s\n", dpath);
	closedir(dir);
	return -EINVAL;
}


/* Function taken from openamp application */
void set_src_dst(char *out, struct rpmsg_endpoint_info *pep)
{
	long dst = 0;
	char *lastdot = strrchr(out, '.');

	if (lastdot == NULL)
		return;
	dst = strtol(lastdot + 1, NULL, 10);
	if ((errno == ERANGE && (dst == LONG_MAX || dst == LONG_MIN))
			|| (errno != 0 && dst == 0)) {
		return;
	}
	pep->dst = (unsigned int)dst;
}


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
int setup_rpmsg_ept_dev(char *rpmsg_dev_name, char *rpmsg_ctrl_dev_name, char *ept_dev_name)
{
	struct rpmsg_endpoint_info eptinfo;
	int charfd,ret=0;
	char rpmsg_char_name[16];
	char *ept_name="rpmsg-openamp-demo-channel";
	char *rpmsg_dev_cpy;

	/*
	 * extract end point name from rpmsg_dev_name
	 * rpmsg_dev_name : virtio0.rpmsg-openamp-demo-channel.-1.1024
	 * ept name : rpmsg-openamp-demo-channel
	 * */
	rpmsg_dev_cpy=strdup(rpmsg_dev_name);
	if(rpmsg_dev_cpy != NULL){
		ept_name = strtok(rpmsg_dev_cpy,".");
		ept_name = strtok(NULL,".");
	}

	/* setup rpmsg eptinfo structure (ept_name,src and dst)*/
	memcpy(eptinfo.name,ept_name,strlen(ept_name));
	eptinfo.name[strlen(ept_name)]='\0';
	eptinfo.src=0;
	set_src_dst(rpmsg_dev_name, &eptinfo);

	/* bind rpmsg chr device */
	ret = bind_rpmsg_chrdev(rpmsg_dev_name);
	if (ret < 0) {
		free(rpmsg_dev_cpy);
		return -1;
	}

	/* setup rpmsg_ctrl device */
	/* kernel >= 6.0 has new path for rpmsg_ctrl device */
	charfd = get_rpmsg_chrdev_fd(rpmsg_ctrl_dev_name, rpmsg_char_name);
	if (charfd < 0) {
		/* may be kernel is < 6.0 try previous path */
		charfd = get_rpmsg_chrdev_fd(rpmsg_dev_name, rpmsg_char_name);
		if (charfd < 0) {
			free(rpmsg_dev_cpy);
			return -1;
		}
	}

	/* Create endpoint from rpmsg char driver */
	DFX_DBG("app_rpmsg_create_ept: %s[src=%#x,dst=%#x]\n",
			eptinfo.name, eptinfo.src, eptinfo.dst);
	ret = app_rpmsg_create_ept(charfd, &eptinfo);
	if (ret) {
		fprintf(stderr, "app_rpmsg_create_ept %s\n", strerror(errno));
		free(rpmsg_dev_cpy);
		return -1;
	}

	/* get ept dev name */
	if (!get_rpmsg_ept_dev_name(rpmsg_char_name, eptinfo.name,
				ept_dev_name)) {
		free(rpmsg_dev_cpy);
		return -1;
	}
	free(rpmsg_dev_cpy);
	return 0;
}



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
rpmsg_dev_t* create_rpmsg_dev(char *rpmsg_dev_name, char *rpmsg_ctrl_dev)
{
	rpmsg_dev_t* rpmsg_dev;
	char *rpmsg_channel_name;
	char ept_dev_name[16];
	int ret = 0;

	if (rpmsg_dev_name == NULL && rpmsg_ctrl_dev == NULL) {
		DFX_DBG("invalid parameters");
		return NULL;
	}

	/* allocation memory */
	rpmsg_dev = (rpmsg_dev_t*)malloc(sizeof(rpmsg_dev_t));
	if (rpmsg_dev == NULL) {
		DFX_DBG("rpmsg dev allocation failed");
		return NULL;
	}

	/* setup endpoint dev */
	ret = setup_rpmsg_ept_dev(rpmsg_dev_name, rpmsg_ctrl_dev, ept_dev_name);
	if (ret != 0) {
		DFX_DBG("rpmsg dev ept setup failed");
		free(rpmsg_dev);
		return NULL;
	}

	/*
	 * store rpmsg channel name from rpmsg_dev_name (virtio0.rpmsg-openamp-demo-channel.-1.1024)
	 * - rpmsg_channel_name (rpmsg-openamp-demo-channel.-1.1024)
	 * */
	rpmsg_channel_name = strchr(rpmsg_dev_name, '.');
	strcpy(rpmsg_dev->rpmsg_channel_name,rpmsg_channel_name+1);

	/* store setup ept_dev_name (/dev/rpmsg#) */
	sprintf(rpmsg_dev->ept_rpmsg_dev_name, "/dev/%s", ept_dev_name);

	/* set node to active state */
	rpmsg_dev->active = 1;

	return rpmsg_dev;
}

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
void reset_active(acapd_list_t *rpmsg_dev_list)
{
	acapd_list_t *rpmsg_dev_node;

	if (rpmsg_dev_list == NULL) {
		DFX_DBG("List is empty\n");
	}

	acapd_list_for_each(rpmsg_dev_list, rpmsg_dev_node) {
		rpmsg_dev_t *rpmsg_dev;

		rpmsg_dev = (rpmsg_dev_t *)acapd_container_of(rpmsg_dev_node, rpmsg_dev_t,
				rpmsg_node);
		rpmsg_dev->active = 0;
	}
}

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
int check_rpmsg_dev_active(acapd_list_t *rpmsg_dev_list, char* rpmsg_dev_name, int virtio_num)
{
	acapd_list_t *rpmsg_dev_node;
	char rpmsg_virtio_dev_name[NAME_MAX + 10];
	int found = 0;

	if (rpmsg_dev_list == NULL) {
		DFX_DBG("List is empty\n");
		return found;
	}

	acapd_list_for_each(rpmsg_dev_list, rpmsg_dev_node) {
		rpmsg_dev_t *rpmsg_dev;

		rpmsg_dev = (rpmsg_dev_t *)acapd_container_of(rpmsg_dev_node, rpmsg_dev_t,
				rpmsg_node);
		/*
		 * Form virtio dev using virtio_num and rpmsg_channel_name
		 * virtio0.rpmsg-openamp-demo-channel.-1.1024
		 * */
		sprintf(rpmsg_virtio_dev_name,"virtio%d.%s",virtio_num,rpmsg_dev->rpmsg_channel_name);
		if (!strncmp(rpmsg_virtio_dev_name, rpmsg_dev_name, strlen(rpmsg_dev_name))) {
			DFX_DBG("Device %s already recorded",rpmsg_virtio_dev_name);
			found = 1;
			/* set active flag */
			rpmsg_dev->active = 1;
			break;
		}

	}
	return found;
}


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
void delete_inactive_rpmsgdev(acapd_list_t *rpmsg_dev_list)
{
	acapd_list_t *rpmsg_dev_node;

	if (rpmsg_dev_list == NULL) {
		DFX_DBG("List is empty\n");
		return;
	}

	acapd_list_for_each(rpmsg_dev_list, rpmsg_dev_node) {
		rpmsg_dev_t *rpmsg_dev;

		rpmsg_dev = (rpmsg_dev_t *)acapd_container_of(rpmsg_dev_node, rpmsg_dev_t,
				rpmsg_node);

		if (rpmsg_dev->active == 0) {
			acapd_list_del(&rpmsg_dev->rpmsg_node);
			free(rpmsg_dev);
		}
	}
}
