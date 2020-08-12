#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <acapd/accel.h>

static int interrupted;

struct pss {
	struct lws_spa *spa;
};

static const char * const param_names[] = {
	"text1",
};

enum enum_param_names {
    EPN_TEXT1,
};

void load_accelerator(const char *pkg_path)
{
	acapd_accel_t accel;
	printf("Initializing accel with %s.\n", pkg_path);
    init_accel(&accel, (acapd_accel_pkg_hd_t *)pkg_path);
	printf("Loading accel %s.\n", pkg_path);
    load_accel(&accel, 0);
	//sleep(2);
    printf("Removing accel %s.\n", pkg_path);
    remove_accel(&accel, 0);
}

static char *requested_uri;
static const char *path;
static int
callback_http(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct pss *pss = (struct pss *)user;
	int n;	
	switch (reason) {
	case LWS_CALLBACK_HTTP:
		requested_uri = (char *)in;
		break;

	case LWS_CALLBACK_HTTP_BODY:
		if(!pss->spa) {
			pss->spa = lws_spa_create(wsi, param_names, LWS_ARRAY_SIZE(param_names), 1024, NULL, NULL);
			if(!pss->spa)
				return -1;
		}
		if (lws_spa_process(pss->spa, in, (int)len))
			return -1;
		break;

	case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		lwsl_user("LWS_CALLBACK_HTTP_BODY_COMPLETION\n");
		lws_spa_finalize(pss->spa);
		if(pss->spa) {
			for(n = 0; n < (int)LWS_ARRAY_SIZE(param_names); n++) {
				if (!lws_spa_get_string(pss->spa, n))
					printf("undefined string in http body %s\n", param_names[n]);
				else {
					//printf("http body %s value %s\n",param_names[n],lws_spa_get_string(pss->spa, n));
					path = lws_spa_get_string(pss->spa,n);
				}
			}
		}
		if(strcmp(requested_uri,"/loadpdi") == 0){
			lwsl_user("Loading pdi \n");
			load_accelerator(path);
		}
		else if(strcmp(requested_uri,"/remove") == 0)
			lwsl_user("DUMMY Removing pdi");
		//if (pss->spa && lws_spa_destroy(pss->spa))
		//	return -1;
		
		break;

	case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
		if (pss->spa) {
			lws_spa_destroy(pss->spa);
			pss->spa = NULL;
		}
		break;
	
	default:
		break;
	}

	return 0;//lws_callback_http_dummy(wsi, reason, user, in, len);
}

static const struct lws_protocols protocols[] = { 
	// first protocol must always be HTTP handler
  {
    "http",
    callback_http,
    sizeof(struct pss),
	0, 0 , NULL, 0
  },
  {
    NULL, NULL, 0, 0, 0, NULL, 0
  }
};

/* override the default mount for /dyn in the URL space */

//static const struct lws_http_mount mount = {
//	/* .mount_next */		NULL,		/* linked-list "next" */
//	/* .mountpoint */		"/load",		/* mountpoint URL */
//	/* .origin */			NULL,	/* protocol */
//	/* .def */			NULL,
//	/* .protocol */			"http",
//	/* .cgienv */			NULL,
//	/* .extra_mimetypes */		NULL,
//	/* .interpret */		NULL,
//	/* .cgi_timeout */		0,
//	/* .cache_max_age */		0,
//	/* .auth_mask */		0,
//	/* .cache_reusable */		0,
//	/* .cache_revalidate */		0,
//	/* .cache_intermediaries */	0,
//	/* .origin_protocol */		LWSMPRO_CALLBACK, /* dynamic */
//	/* .mountpoint_len */		5,		/* char count */
//	/* .basic_auth_login_file */	NULL,
//};

void sigint_handler(int sig)
{
	printf("server interrupted signal %d \n",sig);
	interrupted = 1;
}


int main(int argc, const char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	const char *p;
	int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */;

	signal(SIGINT, sigint_handler);

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

	lws_set_log_level(logs, NULL);
	lwsl_user("LWS minimal http server dynamic | visit http://localhost:7681\n");

	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	//info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
	//	       LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
	//	LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}

	/* http on 7681 */

	info.port = 7681;
	info.protocols = protocols;
	//info.mounts = &mount;
	info.vhost_name = "localhost";

	if (!lws_create_vhost(context, &info)) {
		lwsl_err("Failed to create tls vhost\n");
		goto bail;
	}

	while (n >= 0 && !interrupted)
		n = lws_service(context, 0);

bail:
	lws_context_destroy(context);

	return 0;
}
