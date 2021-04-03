/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/stat.h>
#include "graphServer.h"
#include "layer0/debug.h"
#include <signal.h>
#include <setjmp.h>

struct message message;
int sock_fd;

struct graphSocket {
        int sock_fd;
	struct sockaddr_un socket_address;
};

#define N 25

ssize_t
sock_fd_write(int sock, void *buf, ssize_t buflen, int *fd, int fdcount)
{
    ssize_t     size;
    struct msghdr   msg;
    struct iovec    iov;
    union {
        struct cmsghdr  cmsghdr;
        char        control[CMSG_SPACE(sizeof (int) * N)];
    } cmsgu;
    struct cmsghdr  *cmsg;

    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;

    if (fd[0] != -1) {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(N * sizeof (int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        memset(CMSG_DATA(cmsg), '\0', N * sizeof(int));
        memcpy(CMSG_DATA(cmsg), fd, fdcount * sizeof(int));
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
    }

    size = sendmsg(sock, &msg, 0);

    if (size < 0)
        perror ("sendmsg");
    return size;
}

ssize_t
sock_fd_read(int sock, struct message *buf, int *fd, int *fdcount)
{
	ssize_t     size = 0;
        struct msghdr   msg;
        struct iovec    iov;
        union {
            struct cmsghdr  cmsghdr;
            char        control[CMSG_SPACE(sizeof (int) * N)];
        } cmsgu;
        struct cmsghdr  *cmsg;
        iov.iov_base = buf;
        iov.iov_len = 1024*4;

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
	msg.msg_flags = 0;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        size = recvmsg (sock, &msg, 0);
        if (size < 0) {
            perror ("recvmsg");
            exit(1);
        }
	*fdcount = buf->fdcount;
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
            if (cmsg->cmsg_level != SOL_SOCKET) {
                fprintf (stderr, "invalid cmsg_level %d\n",
                     cmsg->cmsg_level);
                exit(1);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS) {
                fprintf (stderr, "invalid cmsg_type %d\n",
                     cmsg->cmsg_type);
                exit(1);
            }
	}

            memcpy(fd, CMSG_DATA(cmsg), sizeof(int)* (*fdcount));
	//INFO("fdcount : %d\n", *fdcount);
	//INFO("fdcount : %d\n", fd[0]);
   
    return size;
}

//sigjmp_buf point;

//static void handler(int sig, siginfo_t *dont_care, void *dont_care_either)
//{
//	_unused(sig);
//	_unused(dont_care);
//	_unused(dont_care_either);
//	longjmp(point, 1);
   //exit(-1);
//}

int graphClientInit(struct graphSocket* gs){
	//INFO("\n");
	//struct sigaction sa;
	int status;
	
	//memset(&sa, 0, sizeof(sigaction));
	//sigemptyset(&sa.sa_mask);

	//sa.sa_flags     = SA_NODEFER;
	//sa.sa_sigaction = handler;
	//sigaction(SIGSEGV, &sa, NULL); /* ignore whether it works or not */ 

	if ((gs->sock_fd = socket (AF_UNIX, SOCK_SEQPACKET, 0)) == -1){
		error("socket error !!");
		return -1;
	}
	
	//INFO("\n");
	memset (&gs->socket_address, 0, sizeof(struct sockaddr_un));
	gs->socket_address.sun_family = AF_UNIX;
	strncpy (gs->socket_address.sun_path, SERVER_SOCKET, sizeof(gs->socket_address.sun_path) - 1);
	//INFO("%d\n", gs->sock_fd);
	//INFO("%p\n", &gs->socket_address);
	//INFO("%ld\n", sizeof(struct sockaddr_un));
	//if (setjmp(point) == 0){
		status = connect (gs->sock_fd, (const struct sockaddr *) &gs->socket_address, sizeof(struct sockaddr_un));
		if (status == -1){
		error ("connect");
		return -1;
		}
	//}
	//else{
	//	return -1;
	//}
	
	//sa.sa_sigaction = SIG_DFL;
	//sigaction(SIGSEGV, &sa, NULL); /* ignore whether it works or not */ 
	//INFO("\n");
	return 0;
}

int graphClientSubmit(struct graphSocket* gs, char* json, int size, int *fd, int *fdcount){
	//INFO("\n");
	struct message send_message, recv_message;
	memset (&send_message, '\0', sizeof(struct message));
	send_message.id = GRAPH_INIT;
	send_message.size = size;
	send_message.fdcount = 0;
	memcpy(send_message.data, json, size);
	//INFO("\n");
	if (write (gs->sock_fd, &send_message, HEADERSIZE + send_message.size) == -1){
		//INFO("Write error occured !!");
		//return -1;
		error("write");
	}
	memset (&recv_message, '\0', sizeof(struct message));
	size = sock_fd_read(gs->sock_fd, &recv_message, fd, fdcount);
	if (size <= 0)
		return -1;
	//INFO("\n");
	return 0;
}

int graphClientFinalise(struct graphSocket* gs, char* json, int size){	
	struct message send_message, recv_message;
	memset (&send_message, '\0', sizeof(struct message));
	send_message.id = GRAPH_FINALISE;
	send_message.size = size;
	send_message.fdcount = 0;
	memcpy(send_message.data, json, size);
	if (write (gs->sock_fd, &send_message, HEADERSIZE + send_message.size) == -1)
		error ("write");
	memset (&recv_message, '\0', sizeof(struct message));
	size = read (gs->sock_fd, &recv_message, sizeof (struct message));
	if (size <= 0)
		return -1;
	return 0;
}


