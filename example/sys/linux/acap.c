#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

static int interrupted;
static struct lws *web_socket;
static const char *arg;

struct pss {
	char body_part;
};

static int
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

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */

	lws_cmdline_option_handle_builtin(argc, argv, &info);

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
	if ((option = lws_cmdline_option(argc, argv, "-loadpdi"))) {
		ccinfo.path = "/loadpdi";
		arg = option;
	}
	else if ((option = lws_cmdline_option(argc, argv, "-remove"))) {
		ccinfo.path = "/remove";
		arg = option;
	}
	else if ((option = lws_cmdline_option(argc, argv, "-getFD"))) {
		ccinfo.path = "/getFD";
		arg = option;
	}
	else if ((option = lws_cmdline_option(argc, argv, "-getPA"))) {
		ccinfo.path = "/getPA";
		arg = option;
	}
	else if ((lws_cmdline_option(argc, argv, "-getShellFD"))) {
		ccinfo.path = "/getShellFD";
	}
	else if ((lws_cmdline_option(argc, argv, "-getClockFD"))) {
		ccinfo.path = "/getClockFD";
	}
	else if ((lws_cmdline_option(argc, argv, "-getRMInfo"))) {
		ccinfo.path = "/getRMInfo";
	}
	else
		printf("Option not recognized, check again.\n");
	ccinfo.method = "POST";

	web_socket = lws_client_connect_via_info(&ccinfo);
	lwsl_user("http client started\n");

	while (!interrupted)
		if(lws_service(context, 0))
			interrupted = 1;

	lws_context_destroy(context);

	return 0;
}
