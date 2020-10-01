#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "acapdwebsoc.h"

static int interrupted;
static struct lws *web_socket;
static const char *arg;

struct pss {
	char body_part;
};

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, 
  		void *user, void *in, size_t len)
{
	struct pss *pss = (struct pss *)user;
	char buf[LWS_SEND_BUFFER_PRE_PADDING + 1024];
	char *start = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	char *p = start;
	char *end = &buf[sizeof(buf) - LWS_SEND_BUFFER_PRE_PADDING -1];
	int n;
	switch (reason) {
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
			//printf("CLIENT_CONNECTION_ERROR: %s\n",
			// in ? (char *)in : "(null)");
			interrupted = 1;
			break;

		case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
			if (!lws_http_is_redirected_to_get(wsi)) {
				lwsl_user("doing POST flow\n");
				lws_client_http_body_pending(wsi, 1);
				lws_callback_on_writable(wsi);
			}
			else
				printf("doing GET flow\n");
			break;
			
		case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
			//printf("LWS_CALLBACK_CLIENT_HTTP_WRITEABLE\n");
			
			if (lws_http_is_redirected_to_get(wsi))
				break;
			switch (pss->body_part++) {
				case 0:
					if (lws_client_http_multipart(wsi, "text1", NULL, NULL,&p,end))
						return -1;
					p += lws_snprintf(p, end - p,"%s",arg);
					n = LWS_WRITE_HTTP;
					break;
				case 1:
					if (lws_client_http_multipart(wsi, NULL, NULL, NULL,&p,end))
						return -1;
					lws_client_http_body_pending(wsi, 0);
					n = LWS_WRITE_HTTP_FINAL;
					break;
				default:
					return 0;
			}
			if (lws_write(wsi, (uint8_t *)start, lws_ptr_diff(p, start), n)
					!= lws_ptr_diff(p, start))
				return 1;
			if (n != LWS_WRITE_HTTP_FINAL)
				lws_callback_on_writable(wsi);				
			return 0;
			
		case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
			lwsl_user("LWS_CALLBACK_COMPLETED_CLIENT_HTTP\n");
			interrupted = 1;
			lws_cancel_service(lws_get_context(wsi));
			break;
			
		case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
			interrupted = 1;
			lws_cancel_service(lws_get_context(wsi));
			break;
			
		default:
			break;
	}
	
	return lws_callback_http_dummy(wsi, reason, user, in, len);
}



static const struct lws_protocols protocols[] = {
	{
		"http",
		callback_http,
		sizeof(struct pss),
		0,
		0,
		NULL,
		0
	},
	{ NULL, NULL, 0, 0, 0, NULL, 0 }
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
	interrupted = 0;
	signal(SIGINT, sigint_handler);
	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
	info.protocols = protocols;
	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}
	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	
	info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
	info.protocols = protocols;
	//info.connect_timeout_secs = 30;
	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}
	memset(&ccinfo, 0, sizeof ccinfo); /* otherwise uninitialized garbage */
	ccinfo.context = context;
	ccinfo.address = "localhost";
	ccinfo.port = 7681;
	ccinfo.protocol = protocols[0].name;
	ccinfo.ssl_connection = LCCSCF_HTTP_MULTIPART_MIME | LCCSCF_ALLOW_SELFSIGNED;	
	ccinfo.path = path; //"/loadpdi";
	arg = argvalue;
	ccinfo.method = "POST";
	web_socket = lws_client_connect_via_info(&ccinfo);
	lwsl_user("http client started\n");
	
	while (!interrupted){
		if(lws_service(context, 0))
			interrupted = 1;
	}
	lws_context_destroy(context);
	return 0;
}

int loadpdi(char* pdifilename)
{
	//char command[100];
	//sprintf(command, "/home/root/xrt/acap-static -loadpdi %s &", pdifilename);
	//system(command);
	return exchangeCommand("/loadpdi", pdifilename);
}

int removepdi(char* argvalue)
{
	return exchangeCommand("/remove", argvalue);
}

int getFD(char* argvalue)
{
	char command[100];
	sprintf(command, "/home/root/xrt/acap-static -getFD %s &", argvalue);
	system(command);
	return 0; //exchangeCommand("/getFD", argvalue);
}

int getPA(char* argvalue)
{
	char command[100];
	sprintf(command, "/home/root/xrt/acap-static -getPA %s &", argvalue);
	system(command);
	return 0; //exchangeCommand("/getPA", argvalue);
}

int getShellFD()
{
	return exchangeCommand("/getShellFD", "");
}

int getClockFD()
{
	return exchangeCommand("/getClockFD", "");
}


/**************************************************************************/
/* Constants used by this program                                         */
/**************************************************************************/
#define SERVER_PATH_0     "/tmp/server_rm0"
#define SERVER_PATH_1     "/tmp/server_rm1"
#define SERVER_PATH_2     "/tmp/server_rm2"
#define BUFFER_LENGTH    250
#define FALSE              0

#define TXTSRV "server\n"
#define TXTCLI "client\n"

char *server_paths[] = {"/tmp/server_rm0", "/tmp/server_rm1", "/tmp/server_rm2"};

int sendfd(int socket, int fd) {
    	char dummy = '$';
	struct msghdr msg;
	struct iovec iov;

	char cmsgbuf[CMSG_SPACE(sizeof(int))];

	iov.iov_base = &dummy;
	iov.iov_len = sizeof(dummy);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = CMSG_LEN(sizeof(int));

	struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));

	*(int*) CMSG_DATA(cmsg) = fd;

	int ret = sendmsg(socket, &msg, 0);

	if (ret == -1) {
		printf("sendmsg failed with %s", strerror(errno));
	}

	return ret;
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
	read(socket, &arr, sizeof(uint64_t)*6);
	printf("client recvd %lx %lu %lx %lu %lx %lu\n",arr[0],arr[1],arr[2],arr[3],arr[4],arr[5]); 
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
/* Pass in 1 parameter which is either the */
/* path name of the server as a UNICODE    */
/* string, or set the server path in the   */
/* #define SERVER_PATH which is a CCSID    */
/* 500 string.                             */

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
	
	printf("%s\n", server_paths[slot]);
	while(cfileexists(server_paths[slot]) == 0){
		sleep(1);
		printf("##############$$\n");
	}

	rc = connect(sd, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
	if (rc < 0)
	{
		perror("connect() failed");
		return -1;
	}
	printf("started client socket\n");
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
	printf("started client socket\n");
	//fd = recvfd(sd);
	recvPA(sd, fds);
     	if (sd != -1)
	  	close(sd);
	return 0;
}
