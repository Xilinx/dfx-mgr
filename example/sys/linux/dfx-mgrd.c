/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include <libwebsockets.h>
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
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>
#include <dfx-mgr/dfxmgr_client.h>

#define MAX_CLIENTS 200

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

static void
process_dfx_req(int fd, fd_set *fdset)
{
	struct message recv_msg, send_msg;
	ssize_t numbytes;
	int ret, slot;

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
		DFX_PR("daemon loading accel %s", recv_msg.data);
		slot = load_accelerator(recv_msg.data);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", slot);

		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("LOAD_ACCEL write(%d)", fd);
		break;

	case REMOVE_ACCEL:
		slot = -1;	/* assume base */
		if (strcasecmp(recv_msg.data, "base")) {
			slot = atoi(recv_msg.data);
			DFX_PR("daemon REMOVE_ACCEL in slot %d", slot);
		}
		ret = remove_accelerator(slot);
		send_msg.size = 1 + sprintf(send_msg.data, "%d", ret);

		if (write(fd, &send_msg, HEADERSIZE+ send_msg.size) < 0)
			DFX_ERR("REMOVE_ACCEL write(%d)", fd);
		break;

	case LIST_PACKAGE:
		// change to: listAccelerators(buf, size))
		char *msg = listAccelerators();
		send_msg.size = strnlen(msg, sizeof(send_msg.data));
		memcpy(send_msg.data, msg, send_msg.size);
		if (write(fd, &send_msg, HEADERSIZE + send_msg.size) < 0)
			DFX_ERR("LIST_PACKAGE write(%d)", fd);
		free(msg);
		break;

	case LIST_ACCEL_UIO:
		/* slot = atoi(recv_msg.data); */
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

	case QUIT:
		if (close(fd) == -1)
			DFX_ERR("close(%d)", fd);
		FD_CLR(fd, fdset);
		break;

	default:
		DFX_PR("Unexpected message from client");
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
	while (1) {
		readfds = fds;
		// monitor readfds for readiness for reading
		if (select(fdmax + 1, &readfds, NULL, NULL, NULL) == -1)
			dfx_exit("select");
        
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

/*
#define SERVER_PATH     "/tmp/dfx-mgrd_socket"

static int interrupted;;

static int socket_d;
//
struct pss {
	struct lws_spa *spa;
};
*/
/*static const char * const param_names[] = {
	"text1",
};

enum enum_param_names {
    EPN_TEXT1,
};*/
/*struct msg{
    char cmd[32];
    char arg[128];
};
struct resp{
    char data[4096];
    int len;
};
#define EXAMPLE_RX_BUFFER_BYTES sizeof(struct msg)
struct payload
{
    unsigned char data[LWS_SEND_BUFFER_PRE_PADDING + EXAMPLE_RX_BUFFER_BYTES + LWS_SEND_BUFFER_POST_PADDING];
    size_t len;
} received_payload;

unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + sizeof(struct resp) + LWS_SEND_BUFFER_POST_PADDING];

static int msgs;
static int callback_example( struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len )
{
    struct msg *m = (struct msg*)malloc(sizeof(struct msg));
    struct resp *r = (struct resp *)&buf[LWS_SEND_BUFFER_PRE_PADDING];
	int slot;

    switch( reason )
    {
		case LWS_CALLBACK_ESTABLISHED:
			lwsl_user("LWS_CALLBACK_ESTABLISHED user %s\n",(char *)user);
			break;

        case LWS_CALLBACK_RECEIVE:
            acapd_debug("LWS_CALLBACK_RECEIVE len%ld\n",len);
            memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len );
            received_payload.len = len;
            m = (struct msg *)&received_payload.data[LWS_SEND_BUFFER_PRE_PADDING];
            acapd_debug("server received cmd %s arg %s\n",m->cmd,m->arg);
			msgs++;
			if(strcmp(m->cmd,"-load") == 0){
				lwsl_debug("Received %s \n",m->cmd);
				slot = load_accelerator(m->arg);
				sprintf(r->data,"%d",slot);
				r->len = 1;
				acapd_debug("daemon: load done slot %s len %d\n", r->data, r->len);
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-remove") == 0){
				lwsl_debug("Received %s slot %s\n",m->cmd,m->arg);
				r->len = 0;
				remove_accelerator(atoi(m->arg));
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-allocBuffer") == 0){
				lwsl_debug("Received %s size %s\n",m->cmd,m->arg);
				sendBuff(atoi(m->arg));
				sprintf(r->data,"%s","");
				r->len = 0;
				lws_callback_on_writable( wsi );
			}
			else if(strcmp(m->cmd,"-freeBuffer") == 0){
				lwsl_debug("Received %s size %s\n",m->cmd,m->arg);
				freeBuff(atoi(m->arg));
				sprintf(r->data,"%s","");
				r->len = 0;
				lws_callback_on_writable(wsi);
			}
			else if(strcmp(m->cmd,"-getShellFD") == 0){
				lwsl_debug("Received %s \n",m->cmd);
				sprintf(r->data,"%s","");
				r->len = 0;
				getShellFD();
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-getClockFD") == 0){
				lwsl_debug("Received %s \n",m->cmd);
				sprintf(r->data,"%s","");
				r->len = 0;
				getClockFD();
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-getFDs") == 0){
				lwsl_debug("Received %s slot %s\n",m->cmd,m->arg);
				sprintf(r->data,"%s","");
				r->len = 0;
				getFDs(atoi(m->arg));
				lws_callback_on_writable(wsi);
			}
			else if(strcmp(m->cmd,"-listPackage") == 0){
				lwsl_debug("Received -listPackage\n");
				sprintf(r->data,"%s","");
				r->len = 0;
				listAccelerators();
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else {
				lwsl_err("cmd not recognized\n");
				//return -1;
			}
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
			if (!msgs)
				return 0;
            lwsl_debug("LWS_CALLBACK_SERVER_WRITEABLE resp->len %d\n",r->len);
            lws_write( wsi, (unsigned char*)r, sizeof(struct resp), LWS_WRITE_TEXT );
			msgs--;
			//return -1;
            break;

        default:
            break;
    }

    return 0;
}

enum protocols
{
    PROTOCOL_HTTP = 0,
    PROTOCOL_EXAMPLE,
    PROTOCOL_COUNT
};

static const struct lws_protocols protocols[] = { 
	// first protocol must always be HTTP handler
  {
    "http",			
    lws_callback_http_dummy,	
    0,				
	0,				
	0, NULL, 0,
  },
  {
	"example-protocol",
	callback_example,
    0,
    EXAMPLE_RX_BUFFER_BYTES,
	0, NULL, 0,
  },
  { NULL, NULL, 0, 0, 0, NULL, 0 } 
};

void sigint_handler(int sig)
{
	lwsl_err("server interrupted signal %d \n",sig);
	interrupted = 1;
}

void socket_fd_setup()
{
	struct sockaddr_un serveraddr;

	if (access(SERVER_PATH, F_OK) == 0) 
		unlink(SERVER_PATH);

    socket_d = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_d < 0) {
        acapd_perror("%s socket creation failed\n",__func__);
        //return ACAPD_ACCEL_FAILURE;
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, SERVER_PATH);

    if (bind(socket_d, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr))) {
        acapd_perror("%s socket bind() failed\n",__func__);
        //return ACAPD_ACCEL_FAILURE;
    }
    acapd_debug("%s socket bind done\n",__func__);

    //socket will queue upto 10 incoming connections
    if (listen(socket_d, 10)) {
        acapd_perror("%s socket listen() failed\n",__func__);
        //return ACAPD_ACCEL_FAILURE;
    }
    printf("Server started %s ready for client connect.\n",
                                    SERVER_PATH);
}

int main(int argc, const char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	struct daemon_config config;
	pthread_t t;
	const char *p;

	int n = 0, logs = LLL_ERR | LLL_WARN
			// for LLL_ verbosity above NOTICE to be built into lws,
			// lws must have been configured and built with
			// -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE
			// | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER
			// | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY
			// | LLL_DEBUG */;
/*
	acapd_debug("Starting http daemon\n");
	signal(SIGINT, sigint_handler);

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS minimal http server dynamic | visit http://localhost:7681\n");

	memset(&info, 0, sizeof info); 
	//info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
	//	       LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
	//	LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	info.port = 7681;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.vhost_name = "localhost";
	info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}


	if (!lws_create_vhost(context, &info)) {
		lwsl_err("Failed to create tls vhost\n");
		goto bail;
	}
	sem_init(&mutex, 0, 0);
	pthread_create(&t, NULL,threadFunc, NULL);
	socket_fd_setup();
	while (n >= 0 && !interrupted)
		n = lws_service(context, 0);

bail:
	lws_context_destroy(context);

	return 0;
}*/
