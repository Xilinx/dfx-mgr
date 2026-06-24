/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <dfx-mgr/accel.h>
#include <dfx-mgr/assert.h>
#include <dfx-mgr/shell.h>
#include <dfx-mgr/model.h>
#include <dfx-mgr/daemon_helper.h>
#include <dfx-mgr/json-config.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/select.h>
#include <dfx-mgr/dfxmgr_client.h>
#include <systemd/sd-daemon.h>

static volatile int interrupted = 0;
static int socket_d;

void intHandler(int dummy)
{
	_unused(dummy);
	interrupted = 1;
	exit(EXIT_SUCCESS);
}

void dfx_exit(char *msg)
{
	DFX_ERR("%s", msg);
	exit(EXIT_FAILURE);
}

static void format_response_with_warning(struct message *msg, int value)
{
	if (is_pkg_listing_dirty())
		msg->size = 1 + sprintf(msg->data,
			"WARNING: Package IDs have changed since last -listPackage. %d",
			value);
	else
		msg->size = 1 + sprintf(msg->data, "%d", value);
}

static void
process_dfx_req(int fd, fd_set *fdset)
{
	struct message recv_msg, send_msg;
	ssize_t numbytes;
	int ret, slot;
	char *binfile = NULL, *overlay = NULL, *region = NULL, *tmp;
	char *accel_name = NULL, *cma_path = NULL;

	numbytes = read(fd, &recv_msg, sizeof(struct message));
	if (numbytes <= 0) {
		if (numbytes < 0)
			DFX_ERR("read(%d)", fd);
		else
			DFX_DBG("Socket %d closed by client", fd);

		if (close(fd) == -1)
			DFX_ERR("close(%d)", fd);
		FD_CLR(fd, fdset);
		return;
	}

	// data from client
	memset(&send_msg, 0, sizeof(struct message));
	switch (recv_msg.id) {
	case LOAD_ACCEL:
		tmp = strdup(recv_msg.data);
		accel_name = strtok(tmp, ":");
		cma_path = strtok(NULL, ":");
		DFX_PR("daemon loading accel %s", recv_msg.data);
		slot = load_accelerator(accel_name, cma_path);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", slot);

		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("LOAD_ACCEL write(%d)", fd);
		free(tmp);
		break;

	case UNLOAD_ACCEL:
		slot = -1;	/* assume base */
		if (strcasecmp(recv_msg.data, "base")) {
			slot = atoi(recv_msg.data);
			DFX_PR("daemon UNLOAD_ACCEL in slot %d", slot);
		}
		ret = unload_accelerator(slot);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", ret);

		if (write(fd, &send_msg, HEADERSIZE+ send_msg.size) < 0)
			DFX_ERR("UNLOAD_ACCEL write(%d)", fd);
		break;

	case LIST_PACKAGE:
		// change to: listAccelerators(buf, size))
		char *msg = listAccelerators(recv_msg.flags);
		send_msg.size = strnlen(msg, sizeof(send_msg.data));
		memcpy(send_msg.data, msg, send_msg.size);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("LIST_PACKAGE write(%d)", fd);
		free(msg);
		break;

	case LIST_ACCEL_UIO:
		slot = recv_msg._u.slot;
		if (recv_msg.data[0] == 0)
			list_accel_uio(slot, send_msg.data, sizeof(send_msg.data));
		else {
			char *p = get_accel_uio_by_name(slot, recv_msg.data);
			DFX_DBG("%s", recv_msg.data);
			if (p != NULL)
				sprintf(send_msg.data, "%s", p);
		}
		send_msg.size = strnlen(send_msg.data, sizeof(send_msg.data));
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("LIST_ACCEL_UIO write(%d)", fd);
		break;

	case SIHA_IR_LIST:
		ret = siha_ir_buf_list(sizeof(send_msg.data), send_msg.data);
		/*
		 * The siha_ir_buf_list returns 0 on error or the size of
		 * the buffer. The (ret < 0) check is to avoid a large
		 * (e.g: 4GB = unsigned(-1)) write request.
		 */
		send_msg.size = ret < 0 ? 0 : ret;
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("SIHA_IR_LIST: write(%d)", fd);
		break;

	case SIHA_IR_SET:
		ret = siha_ir_buf_set(recv_msg.data);
		send_msg.size = 1 + sprintf(send_msg.data,
				"%d SIHA_IR_SET: %.18s", ret, recv_msg.data);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("SIHA_IR_SET: write(%d)", fd);
		break;

	case QUIT:
		if (close(fd) == -1)
			DFX_ERR("close(%d)", fd);
		FD_CLR(fd, fdset);
		break;

	case USER_LOAD:
		tmp = strdup(recv_msg.data);
		binfile = strtok(tmp, " : ");
		overlay = strtok(NULL, " : ");
		region = strtok(NULL, " : ");

		slot = user_load(recv_msg.flags, binfile, overlay, region);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", slot);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("USER_LOAD write(%d)", fd);
		free(tmp);
		break;

	case USER_UNLOAD:
		ret = user_unload_overlay(recv_msg.data);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", ret);
		if (write(fd, &send_msg, HEADERSIZE+ send_msg.size) < 0)
			DFX_ERR("USER_UNLOAD write(%d)", fd);
		break;

	case LOAD_ACCEL_BY_ID:
		char *id_str;
		tmp = strdup(recv_msg.data);
		id_str = strtok(tmp, ":");
		cma_path = strtok(NULL, ":");
		int load_id = atoi(id_str);
		DFX_PR("daemon loading accel by ID %d", load_id);
		slot = load_accelerator_by_id(load_id, cma_path);
		format_response_with_warning(&send_msg, slot);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("LOAD_ACCEL_BY_ID write(%d)", fd);
		free(tmp);
		break;

	case UNLOAD_ACCEL_BY_ID:
		int unload_id = atoi(recv_msg.data);
		DFX_PR("daemon unloading accel by ID %d", unload_id);
		ret = unload_accelerator_by_id(unload_id);
		format_response_with_warning(&send_msg, ret);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("UNLOAD_ACCEL_BY_ID write(%d)", fd);
		break;

	case LOAD_ACCEL_BY_NAME:
		tmp = strdup(recv_msg.data);
		accel_name = strtok(tmp, ":");
		cma_path = strtok(NULL, ":");
		DFX_PR("daemon loading accel by name %s", accel_name);
		slot = load_accelerator(accel_name, cma_path);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", slot);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("LOAD_ACCEL_BY_NAME write(%d)", fd);
		free(tmp);
		break;

	case UNLOAD_ACCEL_BY_NAME:
		DFX_PR("daemon unloading accel by name %s", recv_msg.data);
		ret = unload_accelerator_by_name(recv_msg.data);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", ret);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("UNLOAD_ACCEL_BY_NAME write(%d)", fd);
		break;

	case UNLOAD_ACCEL_BY_HANDLE: ;
		int handle = atoi(recv_msg.data);
		DFX_PR("daemon unloading accel by handle %d", handle);
		ret = unload_accelerator(handle);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", ret);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("UNLOAD_ACCEL_BY_HANDLE write(%d)", fd);
		break;

	default:
		send_msg.size = 1 + sprintf(send_msg.data,
				"Unsupported message id %d", recv_msg.id);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("default write(%d)", fd);
		DFX_PR("%s", send_msg.data);
	}
}

int main(int argc, char **argv)
{
	const struct sockaddr_un su = {
		.sun_family = AF_UNIX,
		.sun_path   = SERVER_SOCKET,
	};
	struct stat statbuf;
	fd_set fds, readfds;
	int fd_new, fdmax;

	signal(SIGINT, intHandler);
	_unused(argc);
	_unused(argv);
	// initialize the complaint queue
	dfx_init();

	if (stat(SERVER_SOCKET, &statbuf) == 0) {
		if (unlink(SERVER_SOCKET) == -1)
			dfx_exit("unlink " SERVER_SOCKET);
	}

	if ((socket_d = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1)
		dfx_exit("socket");

	if (bind(socket_d, (const struct sockaddr *)&su, sizeof(su)) == -1)
		dfx_exit("bind");

	// Mark socket for accepting incoming connections using accept
	if (listen(socket_d, BACKLOG) == -1)
		dfx_exit("listen");

	FD_ZERO(&fds);
	FD_SET(socket_d, &fds);
	fdmax = socket_d;

	DFX_PR("dfx-mgr daemon started");
	// Notify systemd service that we're ready
	sd_notify(0, "READY=1\nSTATUS=dfx-mgr daemon started");

	while (1) {
		readfds = fds;
		// monitor readfds for readiness for reading
		if (select(fdmax + 1, &readfds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			dfx_exit("select");
		}

		// Some sockets are ready. Examine readfds
		for (int fd = 0; fd < (fdmax + 1); fd++) {
			if (FD_ISSET(fd, &readfds)) {
				// fd is ready for reading
				if (fd == socket_d) {
					// request for new connection
					fd_new = accept(socket_d, NULL, NULL);
					if (fd_new  == -1)
						dfx_exit("accept");

					FD_SET(fd_new, &fds);
					if (fd_new > fdmax)
						fdmax = fd_new;
				} else {
					// data from an existing connection
					process_dfx_req(fd, &fds);
				}
			}
		}
	}
	exit(EXIT_SUCCESS);
}
