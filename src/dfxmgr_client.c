/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
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

int dfxmgr_load(char* packageName)
{
	socket_t *gs;
    struct message send_message, recv_message;
    int ret;

	if (packageName == NULL || !strcmp(packageName,""))
		acapd_perror("%s expects a package name\n",__func__);

	gs = (socket_t *) malloc(sizeof(socket_t));
    memset (&send_message, '\0', sizeof(struct message));
    memset (&recv_message, '\0', sizeof(struct message));
	initSocket(gs);

    memcpy(send_message.data, packageName, strlen(packageName));
    send_message.id = LOAD_ACCEL;
    send_message.size = strlen(packageName);
	ret = write(gs->sock_fd, &send_message, HEADERSIZE + send_message.size);
    if (ret < 0){
        acapd_perror("%s:error sending message from client %d\n",__func__,ret);
		return -1;
	}
    ret = read(gs->sock_fd, &recv_message, sizeof (struct message));
    if (ret <= 0){
        acapd_perror("%s:No message recieved %d\n",__func__, ret);
		return -1;
	}

    acapd_print("%s:Accelerator loaded to slot %s\n",__func__,recv_message.data);
	return atoi(recv_message.data);
}

int dfxmgr_remove(int slot)
{
	socket_t *gs;
    struct message send_message, recv_message;
    int ret;

	if (slot < 0){
		acapd_perror("invalid argument to dfxmgr_remove()\n");
		return -1;
	}
	gs = (socket_t *) malloc(sizeof(socket_t));
    memset (&send_message, '\0', sizeof(struct message));
    memset (&recv_message, '\0', sizeof(struct message));
	initSocket(gs);

	sprintf(send_message.data, "%d", slot);
    send_message.id = REMOVE_ACCEL;
    send_message.size = 2;
    if (write(gs->sock_fd, &send_message, HEADERSIZE + send_message.size) < 0)
        acapd_perror("%s:error sending message from client\n",__func__);
    ret = read(gs->sock_fd, &recv_message, sizeof (struct message));
    if (ret <= 0){
		acapd_perror("%s:No message recieved\n",__func__);
		return ret;
	}
	ret = atoi(recv_message.data);
	if (ret == 0)
		acapd_print("%s:Accelerator succesfully removed.\n",__func__);
	else
		acapd_perror("%s:Error trying to remove accelerator.\n",__func__);
	return ret;
}
/*int clientFD(char* argvalue)
{
	return exchangeCommand("-getFD", argvalue);
}

int clientShellFD()
{
	return exchangeCommand("-getShellFD", "");
}

int clientClockFD()
{
	return exchangeCommand("-getClockFD", "");
}


int recvfd(int socket, fds_t *fds) {
	int len;
	int fd[5];
	char buf[1];
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	char cms[CMSG_SPACE(sizeof(int) * 5)];

	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	msg.msg_control = (caddr_t) cms;
	msg.msg_controllen = sizeof cms;

	len = recvmsg(socket, &msg, 0);

	if (len < 0) {
		printf("recvmsg failed with %s", strerror(errno));
		return -1;
	}

	if (len == 0) {
		printf("recvmsg failed no data");
		return -1;
	}
	//printf("############ : %d\n", len);

	cmsg = CMSG_FIRSTHDR(&msg);
	memcpy(&fd, (int *)CMSG_DATA(cmsg), sizeof(int) * 5);
	fds->mm2s_fd = fd[0];
	fds->s2mm_fd = fd[1];
	fds->config_fd = fd[2];
	fds->accelconfig_fd = fd[3];
	fds->dma_hls_fd = fd[4];
	return 0;
}

void recvPA(int socket, fds_t *fds) {
	uint64_t arr[6];
	int ret;
	
	ret = read(socket, &arr, sizeof(uint64_t)*6);
	acapd_perror("client(ret %d) recvd %lx %lu %lx %lu %lx %lu\n",ret,
					arr[0],arr[1],arr[2],arr[3],arr[4],arr[5]); 
	fds->mm2s_pa     = arr[0];
	fds->mm2s_size   = arr[1];
	fds->s2mm_pa     = arr[2];
	fds->s2mm_size   = arr[3];
	fds->config_pa   = arr[4];
	fds->config_size = arr[5];

}

int cfileexists(const char* filename){
	struct stat buffer;
	int exist = stat(filename,&buffer);
	if(exist == 0)
		return 1;
	else
		return 0;
}

int socketGetFd(int slot, fds_t* fds){
	struct sockaddr_un serveraddr;
	int    sd=-1;
	int rc;
	
	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0)
	{
		perror("socket() failed");
		return -1;
	}
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sun_family = AF_UNIX;
	strcpy(serveraddr.sun_path, server_paths[slot]);
	
	acapd_debug("%s\n", server_paths[slot]);
	while(cfileexists(server_paths[slot]) == 0){
		sleep(1);
		acapd_debug("##############$$\n");
	}

	rc = connect(sd, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
	if (rc < 0)
	{
		perror("connect() failed");
		return -1;
	}
	acapd_debug("started client socket\n");
	//fd = recvfd(sd);
	recvfd(sd, fds);
	recvPA(sd, fds);
     	if (sd != -1)
	  	close(sd);
	return 0;
}
int socketGetPA(int slot, fds_t* fds){
	struct sockaddr_un serveraddr;
	int    sd=-1;
	int rc;
	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0)
	{
		perror("socket() failed");
		return -1;
	}
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sun_family = AF_UNIX;
	strcpy(serveraddr.sun_path, server_paths[slot]);
	rc = connect(sd, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
	if (rc < 0)
	{
		perror("connect() failed");
		return -1;
	}
	acapd_debug("started client socket\n");
	//fd = recvfd(sd);
	recvPA(sd, fds);
     	if (sd != -1)
	  	close(sd);
	return 0;
}

*/
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
	*fdcount = buf->fdcount;
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

int initSocket(socket_t* gs){
	int status;
	
	if ((gs->sock_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1){
		acapd_perror("%s:socket creation failed\n",__func__);
		return -1;
	}
	memset(&gs->socket_address, 0, sizeof(struct sockaddr_un));
	gs->socket_address.sun_family = AF_UNIX;
	strncpy(gs->socket_address.sun_path, SERVER_SOCKET, sizeof(gs->socket_address.sun_path) - 1);
	status = connect(gs->sock_fd, (const struct sockaddr *) &gs->socket_address, sizeof(struct sockaddr_un));
	if (status < 0){
		acapd_perror("%s:connect failed\n",__func__);
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
	send_message.fdcount = 0;
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
	send_message.fdcount = 0;
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

