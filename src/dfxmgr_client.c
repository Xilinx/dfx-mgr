/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dfx-mgr/print.h>
#include "dfxmgr_client.h"

#define MAX_FD 25
struct message message;
int sock_fd;

#if 0
#include <libwebsockets.h>

static int interrupted;
static struct lws *web_socket;
static const char *arg;
static const char *cmd;

struct pss {
	char body_part;
};

struct msg{
    char cmd[32];
    char arg[32];
};
struct resp{
    char data[128];
    int len;
};

struct resp *response;
static int msgs_sent;

#define EXAMPLE_RX_BUFFER_BYTES (sizeof(struct resp))
struct payload
{
    unsigned char data[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
    size_t len;
} received_payload;

static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
    struct msg *m;
    //struct resp *r;
    switch( reason )
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("LWS_CALLBACK_CLIENT_ESTABLISHED user %s\n",(char *)user);
            lws_callback_on_writable( wsi );
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            msgs_sent--;
            printf("LWS_CALLBACK_CLIENT_RECEIVE count %d\n",msgs_sent);
            memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len );
            received_payload.len = len;
            response = (struct resp *)&received_payload.data[LWS_SEND_BUFFER_PRE_PADDING];
            printf("client recieved data %s len %d\n",response->data,response->len);
            if(!msgs_sent)
                interrupted = 1;
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            printf("LWS_CALLBACK_CLIENT_WRITEABLE count %d\n",msgs_sent);
            unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + sizeof(struct msg) + LWS_SEND_BUFFER_POST_PADDING];
            //unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
            //size_t n = sprintf( (char *)p, "%u", rand() );
            m = (struct msg *)&buf[LWS_SEND_BUFFER_PRE_PADDING];
            sprintf(m->cmd,"%s",cmd);
            sprintf(m->arg,"%s",arg);
            acapd_perror("client writing cmd %s arg %s\n",m->cmd,m->arg);
            lws_write( wsi, (unsigned char *)m, sizeof(struct msg), LWS_WRITE_TEXT );
			if(strcmp(cmd,"-loadpdi")){
					//sleep(2);
					interrupted = 1;
			}else {
				//sleep(1);
				msgs_sent++;
			}
			acapd_perror("LWS_CALLBACK_CLIENT_WRITEABLE done count %d\n",msgs_sent);
            break;
        case LWS_CALLBACK_CLOSED:
            printf("LWS_CALLBACK_CLOSED\n");
            interrupted = 1;
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            lwsl_user("CLIENT_CONNECTION_ERROR: %s\n",
                 in ? (char *)in : "(null)");
            interrupted = 1;
            break;

        default:
            break;
    }

    return  lws_callback_http_dummy(wsi, reason, user, in, len);
}

static struct lws_protocols protocols[] =
{
    {
        "example-protocol",
        callback_example,
        0,
        EXAMPLE_RX_BUFFER_BYTES, 0, NULL, 0
    },
    { NULL, NULL, 0, 0, 0, NULL, 0} // terminator 
};

static void sigint_handler(int sig)
{
	printf("Recieved signal %d\n",sig);
	interrupted = 1;
}

int exchangeCommand(char* path, char* argvalue){
    struct lws_context_creation_info info;
    struct lws_context *context;
    struct lws_client_connect_info ccinfo = {0};

    signal(SIGINT, sigint_handler);

    memset(&info, 0, sizeof info);
    memset(&ccinfo, 0, sizeof ccinfo);

	interrupted = 0;
    info.port = CONTEXT_PORT_NO_LISTEN; 
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    //info.connect_timeout_secs = 30;

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        return 1;
    }
    ccinfo.context = context;
    ccinfo.address = "localhost";
    ccinfo.port = 7681;
    ccinfo.protocol = protocols[0].name;
    ccinfo.path = "/";
	cmd = path;
	arg = argvalue;
    //ccinfo.ssl_connection = LCCSCF_HTTP_MULTIPART_MIME | LCCSCF_ALLOW_SELFSIGNED;
    //ccinfo.method = "POST";
    web_socket = lws_client_connect_via_info(&ccinfo);
    lwsl_user("WS client started\n");

    while (!interrupted){
		//printf("starting lws_service client\n");
        if(lws_service(context, 15))
            interrupted = 1;
	}

    lws_context_destroy(context);

    return 0;
}
#endif /*   0 */

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

int dfxmgr_remove(int slot)
{
	struct message send_msg, recv_msg;
	socket_t gs;

	if (slot < 0){
		DFX_ERR("invalid slot %d", slot);
		return -1;
	}

	initSocket(&gs);
	send_msg.id = REMOVE_ACCEL;
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

	DFX_PR("remove from slot %d returns: %s (%s)", slot, recv_msg.data,
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

/**
 * dfxmgr_siha_ir_buf_set - Set up Inter-RM buffers for I/O between slots
 * @sz:		the number of elements in slot_seq array
 * @slot_seq:	array of slot IDs to connect
 *
 * In 2RP design there are only two possible slot_seq:
 * {0, 1} slot 0 writes to IR-buf 1; slot 1 reads from its IR-buf
 * {1, 0} slot 1 writes to IR-buf 0; slot 0 reads from its IR-buf
 * In 3RP design there are 12 sets, 6 w/ two slots and 6 w/ 3 slots.
 * E.g.: {1, 2, 0}
 *	 slot 1 writes to IR-buf 2,
 *	 slot 2 reads from its IR-buf 2 and writes to IR-buf 0,
 *	 slot 0 reads from its IR-buf 0
 *
 * Returns: 0 if connected successfully; non-0 otherwise
 */
#if 0
int
dfxmgr_siha_ir_buf_set(char *user_slot_seq)
{
}
#endif

/**
 * dfxmgr_siha_ir_list - list DMs configuration, see siha_ir_buf_list
 * @sz:  the size of the buf
 * @buf: buffer to put the DMs configuration
 *
 * Returns: pointer to the buffer obuf
 */
char *
dfxmgr_siha_ir_list(uint32_t sz, char *obuf)
{
	struct message send_msg, recv_msg;
	socket_t gs;

	if (!obuf) {
		DFX_ERR("obuf is 0");
		return NULL;
	}

	initSocket(&gs);
	send_msg.id = SIHA_IR_LIST;
	send_msg.size = 0;
	if (write(gs.sock_fd, &send_msg, HEADERSIZE + send_msg.size) < 0) {
		DFX_ERR("write(%d)", gs.sock_fd);
		return NULL;
	}

	if (read(gs.sock_fd, &recv_msg, sizeof(struct message)) < 0) {
		DFX_ERR("No message or read(%d) error", gs.sock_fd);
		return NULL;
	}
	strncpy(obuf, recv_msg.data, sz);
	return obuf;
}

ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int *fd, int fdcount)
{
    ssize_t size;
    struct msghdr msg;
    struct iovec iov;
    union {
		struct cmsghdr  cmsghdr;
        char control[CMSG_SPACE(sizeof (int) * MAX_FD)];
    } cmsgu;
    struct cmsghdr *cmsg;

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
        cmsg->cmsg_len = CMSG_LEN(MAX_FD * sizeof (int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        memset(CMSG_DATA(cmsg), '\0', MAX_FD * sizeof(int));
        memcpy(CMSG_DATA(cmsg), fd, fdcount * sizeof(int));
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
    }

    size = sendmsg(sock, &msg, 0);

    if (size < 0)
        acapd_perror("%s:sendmsg() failed\n",__func__);
    return size;
}

ssize_t sock_fd_read(int sock, struct message *buf, int *fd, int *fdcount)
{
	ssize_t size = 0;
    struct msghdr msg;
    struct iovec iov;
    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof (int) * MAX_FD)];
    } cmsgu;

    struct cmsghdr *cmsg;
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
        acapd_perror("%s:recvmsg() failed\n",__func__);
        exit(1);
    }
	*fdcount = buf->_u.fdcount;
    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
        if (cmsg->cmsg_level != SOL_SOCKET) {
            acapd_perror("%s: invalid cmsg_level %d\n",__func__, cmsg->cmsg_level);
            exit(1);
        }
        if (cmsg->cmsg_type != SCM_RIGHTS) {
            acapd_perror("%s: invalid cmsg_type %d\n",__func__, cmsg->cmsg_type);
            exit(1);
        }
	}
    memcpy(fd, CMSG_DATA(cmsg), sizeof(int)*(*fdcount));
    return size;
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
		DFX_ERR("connect(%s)", SERVER_SOCKET);
		return -1;
	}
	return 0;
}

int graphClientSubmit(socket_t *gs, char* json, int size, int *fd, int *fdcount){
	struct message send_message, recv_message;
	int ret;

	memset(&send_message, '\0', sizeof(struct message));
	send_message.id = GRAPH_INIT;
	send_message.size = size;
	send_message._u.fdcount = 0;
	memcpy(send_message.data, json, size);
	ret = write(gs->sock_fd, &send_message, HEADERSIZE + send_message.size);
	if (ret < 0){
		acapd_perror("%s:graphClientSubmit write() failed\n",__func__);
		return -1;
	}
	memset(&recv_message, '\0', sizeof(struct message));
	size = sock_fd_read(gs->sock_fd, &recv_message, fd, fdcount);
	if (size <= 0)
		return -1;
	return 0;
}

int graphClientFinalise(socket_t *gs, char* json, int size){	
	struct message send_message, recv_message;
	int ret;

	memset(&send_message, '\0', sizeof(struct message));
	send_message.id = GRAPH_FINALISE;
	send_message.size = size;
	send_message._u.fdcount = 0;
	memcpy(send_message.data, json, size);
	ret = write(gs->sock_fd, &send_message, HEADERSIZE + send_message.size);
	if (ret < 0){
		acapd_perror("%s:graphClientFinalise write() failed\n",__func__);
		return -1;
	}
	memset(&recv_message, '\0', sizeof(struct message));
	size = read(gs->sock_fd, &recv_message, sizeof (struct message));
	if (size <= 0)
		return -1;
	return 0;
}

