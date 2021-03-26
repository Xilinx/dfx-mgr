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
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <dfx-mgr/accel.h>
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

#define SERVER_PATH     "/tmp/dfx-mgrd_socket"

static int interrupted;;

static int socket_d;
//
struct pss {
	struct lws_spa *spa;
};

/*static const char * const param_names[] = {
	"text1",
};

enum enum_param_names {
    EPN_TEXT1,
};*/
struct msg{
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
    "http",			/* name */
    lws_callback_http_dummy,	/* callback */
    0,				/* per session data */
	0,				/* max frame size/ rx buffer */
	0, NULL, 0,
  },
  {
	"example-protocol",
	callback_example,
    0,
    EXAMPLE_RX_BUFFER_BYTES,
	0, NULL, 0,
  },
  { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
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
	pthread_t t;
	const char *p;
	//int ret;

	int n = 0, logs = LLL_ERR | LLL_WARN
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */;

	acapd_debug("Starting http daemon\n");
	signal(SIGINT, sigint_handler);

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS minimal http server dynamic | visit http://localhost:7681\n");

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
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
	pthread_create(&t, NULL,threadFunc, NULL);
	socket_fd_setup();
	while (n >= 0 && !interrupted)
		n = lws_service(context, 0);

bail:
	lws_context_destroy(context);

	return 0;
}
