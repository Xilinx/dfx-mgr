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

#define SERVER_SOCKET "/var/run/dfx-mgrd.socket"
#define MAX_MESSAGE_SIZE          4*1024
#define BACKLOG                   10

enum dfx_mgr_request {
	DFX_MGR_REQ_FIRST,
	QUIT	= DFX_MGR_REQ_FIRST,
	GRAPH_INIT,
	GRAPH_FINALISE,
	GRAPH_STAGED,
	GRAPH_GET_IOBUFF,
	GRAPH_SET_IOBUFF,
	LOAD_ACCEL,
	REMOVE_ACCEL,
	LIST_PACKAGE,
	LIST_ACCEL_UIO,
	DFX_MGR_REQ_10,		/* unused */
	GRAPH_INIT_DONE,
	GRAPH_FINALISE_DONE,
	GRAPH_STAGED_DONE,
	SIHA_IR_LIST,
	SIHA_IR_SET,
	USER_LOAD,
	USER_UNLOAD,
};

#define MAX_CLIENTS 200

#define HEADERSIZE 24
struct message {
    uint32_t id;
    uint32_t size;
    uint32_t user_load_flag;
    union {
	    uint32_t fdcount;
	    uint32_t slot;
    } _u;
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
extern char *dfxmgr_uio_by_name(char *obuf, int slot, const char *name);
extern char *dfxmgr_siha_ir_list(uint32_t sz, char *obuf);
extern int dfxmgr_siha_ir_buf_set(const char *user_slot_seq);
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
