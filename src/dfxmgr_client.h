/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _ACAPD_SOCKET_H
#define _ACAPD_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/un.h>
#include <stdint.h>
#include <sys/types.h>

#define SERVER_SOCKET       "/tmp/dfx-mgrd.socket"
#define MAX_MESSAGE_SIZE          4*1024
#define BACKLOG                   10

#define QUIT                      0
#define GRAPH_INIT                1
#define GRAPH_FINALISE            2
#define GRAPH_STAGED              3
#define GRAPH_GET_IOBUFF          4
#define GRAPH_SET_IOBUFF          5
#define LOAD_ACCEL              6
#define REMOVE_ACCEL            7
#define LIST_PACKAGE            8


#define GRAPH_INIT_DONE           11
#define GRAPH_FINALISE_DONE       12
#define GRAPH_STAGED_DONE         13

#define MAX_CLIENTS 200

#define HEADERSIZE 24
struct message {
    uint32_t id;
    uint32_t size;
    uint32_t fdcount;
    char data [32*1024];
};

extern void error (char *msg);

typedef struct {
        int sock_fd;
        struct sockaddr_un socket_address;
} socket_t;

extern int initSocket(socket_t *gs);
extern int graphClientSubmit(socket_t *gs, char* json, int size, int *fd, int *fdcount);
extern int graphClientFinalise(socket_t *gs, char* json, int size);
extern ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int *fd, int fdcount);
extern ssize_t sock_fd_read(int sock, struct message *buf, int *fd, int *fdcount);

typedef struct fds{
	int s2mm_fd;
	int mm2s_fd;
	int config_fd;
	int accelconfig_fd;
	int dma_hls_fd;
	uint64_t mm2s_pa;
	uint64_t mm2s_size;
	uint64_t s2mm_pa;
	uint64_t s2mm_size;
	uint64_t config_pa;
	uint64_t config_size;

} fds_t;

extern int dfxmgr_load(char* packageName);
extern int dfxmgr_remove(int slot);
//extern int getFD(char* argvalue);
//extern int getPA(char* argvalue);
//extern int getShellFD();
//extern int getClockFD();
//extern int socketGetFd(int slot, fds_t *fds);
//extern int socketGetPA(int slot, fds_t *fds);
//extern int test();

#ifdef __cplusplus
}
#endif

#endif
