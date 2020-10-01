#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

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
    struct resp *r;
    switch( reason )
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("LWS_CALLBACK_CLIENT_ESTABLISHED user %s\n",(char *)user);
            lws_callback_on_writable( wsi );
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf("LWS_CALLBACK_CLIENT_RECEIVE len %ld\n",len);
			msgs_sent--;
            memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len );
            received_payload.len = len;
            r = (struct resp *)&received_payload.data[LWS_SEND_BUFFER_PRE_PADDING];
            printf("client recieved %s len %d\n",r->data,r->len);
			if(!msgs_sent)
				interrupted = 1;
            /* Handle incomming messages here. */
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            printf("LWS_CALLBACK_CLIENT_WRITEABLE len %ld\n",len);
            unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + sizeof(struct msg) + LWS_SEND_BUFFER_POST_PADDING];
            //unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
            //size_t n = sprintf( (char *)p, "%u", rand() );
            m = (struct msg *)&buf[LWS_SEND_BUFFER_PRE_PADDING];
            sprintf(m->cmd,"%s",cmd);
            sprintf(m->arg,"%s",arg);
			printf("client writing cmd %s arg %s\n",m->cmd,m->arg);
            lws_write( wsi, (unsigned char *)m, sizeof(struct msg), LWS_WRITE_TEXT );
			if(strcmp(cmd,"-loadpdi"))
				interrupted = 1;
				//lws_cancel_service(lws_get_context(wsi));
			msgs_sent++;
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



/*static int
callback_http(struct lws *wsi, enum lws_callback_reasons reason,
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
		lwsl_user("CLIENT_CONNECTION_ERROR: %s\n",
			 in ? (char *)in : "(null)");
		interrupted = 1;
		break;

	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		if (!lws_http_is_redirected_to_get(wsi)) {
			lwsl_user("doing POST flow\n");
			lws_client_http_body_pending(wsi, 1);
			lws_callback_on_writable(wsi);
		}
		else
			lwsl_user("doing GET flow\n");
		break;

	case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
		lwsl_user("LWS_CALLBACK_CLIENT_HTTP_WRITEABLE\n");
		
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
		lwsl_user("LWS_CALLBACK_CLOSED_CLIENT_HTTP\n");
		interrupted = 1;
		lws_cancel_service(lws_get_context(wsi));
		break;

	default:
		break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);
}*/

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


/*static const struct lws_protocols protocols[] = {
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
};*/

static void
sigint_handler(int sig)
{
	printf("Recieved signal %d\n",sig);
	interrupted = 1;
}

int main(int argc, const char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	struct lws_client_connect_info ccinfo = {0};
	const char *option;

	signal(SIGINT, sigint_handler);

	memset(&info, 0, sizeof info); 
	memset(&ccinfo, 0, sizeof ccinfo);

	lws_cmdline_option_handle_builtin(argc, argv, &info);

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
	//ccinfo.ssl_connection = LCCSCF_HTTP_MULTIPART_MIME | LCCSCF_ALLOW_SELFSIGNED;	
	if ((option = lws_cmdline_option(argc, argv, "-loadpdi"))) {
		cmd = "-loadpdi";
		arg = option;
	}
	else if ((option = lws_cmdline_option(argc, argv, "-remove"))) {
		cmd = "-remove";
		arg = option;
	}
	else if ((option = lws_cmdline_option(argc, argv, "-getFD"))) {
		cmd = "-getFD";
		arg = option;
	}
	else if ((lws_cmdline_option(argc, argv, "-getShellFD"))) {
		cmd = "-getShellFD";
	}
	else if ((lws_cmdline_option(argc, argv, "-getClockFD"))) {
		cmd = "-getClockFD";
	}
	else if ((lws_cmdline_option(argc, argv, "-getRMInfo"))) {
		cmd = "-getRMInfo";
	}
	else
		printf("Option not recognized, check again.\n");
	//ccinfo.method = "POST";

	web_socket = lws_client_connect_via_info(&ccinfo);
	lwsl_user("http client started\n");

	while (!interrupted)
		if(lws_service(context, 0))
			interrupted = 1;

	lws_context_destroy(context);

	return 0;
}
