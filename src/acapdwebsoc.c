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
            /* Handle incomming messages here. */
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            printf("LWS_CALLBACK_CLIENT_WRITEABLE count %d\n",msgs_sent);
            unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + sizeof(struct msg) + LWS_SEND_BUFFER_POST_PADDING];
            //unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
            //size_t n = sprintf( (char *)p, "%u", rand() );
            m = (struct msg *)&buf[LWS_SEND_BUFFER_PRE_PADDING];
            sprintf(m->cmd,"%s",cmd);
            sprintf(m->arg,"%s",arg);
            printf("client writing cmd %s arg %s\n",m->cmd,m->arg);
            lws_write( wsi, (unsigned char *)m, sizeof(struct msg), LWS_WRITE_TEXT );
			if(strcmp(cmd,"-loadpdi")){
					sleep(2);
					interrupted = 1;
			}else {
				sleep(1);
				msgs_sent++;
			}
			printf("LWS_CALLBACK_CLIENT_WRITEABLE done count %d\n",msgs_sent);
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
    { NULL, NULL, 0, 0, 0, NULL, 0} /* terminator */
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
    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
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
        if(lws_service(context, 0))
            interrupted = 1;
	}

    lws_context_destroy(context);

    return 0;
}

int loadpdi(char* pdifilename)
{
	exchangeCommand("-loadpdi", pdifilename);
	printf("loaded accel %s resp->data %s resp->len %d\n",pdifilename,response->data,response->len);
	if (response->len)
		return atoi(response->data);
	else
		return -1;
}

int removepdi(char* argvalue)
{
	return exchangeCommand("-remove", argvalue);
}

int getFD(char* argvalue)
{
	return exchangeCommand("-getFD", argvalue);
}

int getShellFD()
{
	return exchangeCommand("-getShellFD", "");
}

int getClockFD()
{
	return exchangeCommand("-getClockFD", "");
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
