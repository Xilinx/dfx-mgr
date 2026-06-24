/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <dfx-mgr/print.h>
#include "dfxmgr_client.h"

int dfxmgr_load(char* pkg_name)
{
	struct message send_msg, recv_msg;
	socket_t gs;

	if (pkg_name == NULL || pkg_name[0] == 0) {
		DFX_ERR("need package name");
		return -1;
	}

	initSocket(&gs);
	send_msg.id = LOAD_ACCEL;
	send_msg.size = strlen(pkg_name);
	strncpy(send_msg.data, pkg_name, sizeof(send_msg.data));
	if (write(gs.sock_fd, &send_msg, HEADERSIZE + send_msg.size) < 0) {
		DFX_ERR("write(%d)", gs.sock_fd);
		return -1;
	}

	if (read(gs.sock_fd, &recv_msg, sizeof(struct message)) < 0) {
		DFX_ERR("No message or read(%d) error", gs.sock_fd);
		return -1;
	}

	DFX_PR("Accelerator %s loaded to slot %s", pkg_name, recv_msg.data);
	return atoi(recv_msg.data);
}

int dfxmgr_unload(int slot)
{
	struct message send_msg, recv_msg;
	socket_t gs;

	if (slot < 0){
		DFX_ERR("invalid slot %d", slot);
		return -1;
	}

	initSocket(&gs);
	send_msg.id = UNLOAD_ACCEL;
	send_msg.size = 2;
	sprintf(send_msg.data, "%d", slot);
	if (write(gs.sock_fd, &send_msg, HEADERSIZE + send_msg.size) < 0) {
		DFX_ERR("write(%d)", gs.sock_fd);
		return -1;
	}

	if (read(gs.sock_fd, &recv_msg, sizeof (struct message)) < 0) {
		DFX_ERR("No message or read(%d) error", gs.sock_fd);
		return -1;
	}

	DFX_PR("unload from slot %d returns: %s (%s)", slot, recv_msg.data,
		recv_msg.data[0] == '0' ? "Ok" : "Error");
	return  recv_msg.data[0] == '0' ? 0 : -1;
}

/*
 * We expect a short path in the recv_msg.data, e.g.: /dev/uioN,
 * and we will copy at most NAME_MAX characters via strncpy.
 * However, the compiler flags -Wall -Werror -Wextra force
 * "output may be truncated copying .." error for strncpy.
 * Add __attribute__((nonstring)) to avoid the error message.
 */
char *
dfxmgr_uio_by_name(char *obuf __attribute__((nonstring)),
		   int slot, const char *name)
{
	struct message send_msg, recv_msg;
	socket_t gs;

	if (slot < 0 || !name || name[0] == 0) {
		DFX_ERR("invalid slot %d, or no name", slot);
		return NULL;
	}

	initSocket(&gs);
	send_msg.id = LIST_ACCEL_UIO;
	send_msg._u.slot = slot;
	send_msg.size = 1 + strlen(name);
	strncpy(send_msg.data, name, sizeof(send_msg.data));
	if (write(gs.sock_fd, &send_msg, HEADERSIZE + send_msg.size) < 0) {
		DFX_ERR("write(%d)", gs.sock_fd);
		return NULL;
	}

	if (read(gs.sock_fd, &recv_msg, sizeof(struct message)) < 0) {
		DFX_ERR("No message or read(%d) error", gs.sock_fd);
		return NULL;
	}
	strncpy(obuf, recv_msg.data, NAME_MAX);
	return obuf;
}

int initSocket(socket_t* gs)
{
	const struct sockaddr_un su = {
		.sun_family = AF_UNIX,
		.sun_path   = SERVER_SOCKET,
	};

	if ((gs->sock_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1){
		DFX_ERR("socket(AF_UNIX, SOCK_SEQPACKET, 0)");
		return -1;
	}

	gs->socket_address = su;
	if (connect(gs->sock_fd, (const struct sockaddr *)&su, sizeof(su)) < 0){
		if (errno == EACCES || errno == EPERM)
			DFX_ERR("connect(%s): insufficient permissions"
				" (run with sudo)", SERVER_SOCKET);
		else
			DFX_ERR("connect(%s)", SERVER_SOCKET);
		return -1;
	}
	return 0;
}
