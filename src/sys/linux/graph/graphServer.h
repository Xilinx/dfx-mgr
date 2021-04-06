/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>

#define SERVER_SOCKET       "/tmp/dfx-mgrd.socket"

#define MAX_MESSAGE_SIZE          4*1024
#define BACKLOG                   10

#define GRAPH_INIT                1
#define GRAPH_FINALISE            2
#define GRAPH_SCHEDULED           3
#define GRAPH_GET_IOBUFF          4
#define GRAPH_SET_IOBUFF          5
#define LOAD_ACCEL				6
#define REMOVE_ACCEL			7
#define LIST_PACKAGE			8

#define QUIT                      0

#define GRAPH_INIT_DONE           11
#define GRAPH_FINALISE_DONE       12

#define MAX_CLIENTS 200

#define HEADERSIZE 24
struct message {
	uint32_t id;
	uint32_t size;
	uint32_t fdcount;
	char data [32*1024];
};

extern void error (char *msg);
