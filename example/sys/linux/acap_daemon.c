#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <acapd/accel.h>
#include <acapd/shell.h>

static int interrupted;
static acapd_accel_t **active_slots;
static acapd_accel_t accel;

struct pss {
	struct lws_spa *spa;
};

static const char * const param_names[] = {
	"text1",
};

enum enum_param_names {
    EPN_TEXT1,
};

void load_accelerator(const char *pkg_path, const char *shell)
{
	int i;
	printf("Initializing accel with %s.\n", pkg_path);
    init_accel(&accel, (acapd_accel_pkg_hd_t *)pkg_path);
	printf("Loading accel %s. \n", pkg_path);
	if (active_slots == NULL)
	{	
		printf("%s allocating active_slots\n",__func__);
		active_slots = (acapd_accel_t **)calloc(3, sizeof(acapd_accel_t *));
	}
	for (i = 0; i < 3; i++) {
		if (active_slots[i] == NULL){
			accel.rm_slot = i;
			load_accel(&accel, shell, 0);
			active_slots[i] = &accel;
			printf("Loaded accel to slot %d mm2s fd %d\n",i,accel.mm2s_fd);	
			break;
		}
	} 
}

void remove_accelerator(int slot)
{
	printf("Removing accel from slot %d\n",slot);
	if (active_slots == NULL || active_slots[slot] == NULL){
		printf("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
    remove_accel(&accel, 0);
	active_slots[slot] = NULL;
}
void getInputFD(int slot)
{
	//acapd_accel_t * accel = active_slots[slot];
	printf("%s Enter\n mm2s fd %d\n",__func__,accel.mm2s_fd);
	if (active_slots == NULL || active_slots[slot] == NULL){
		printf("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	
	get_mm2s_fd(&accel);
}
void getOutputFD(int slot)
{
	printf("%s Enter\n",__func__);
	if (active_slots == NULL || active_slots[slot] == NULL){
		printf("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	get_s2mm_fd(active_slots[slot]);

}
static char *requested_uri;
static const char *arg;
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
					arg = lws_spa_get_string(pss->spa,n);
				}
			}
		}
		if(strcmp(requested_uri,"/loadpdi") == 0){
			lwsl_user("Received loadpdi \n");
			load_accelerator(arg,NULL);
		}
		else if(strcmp(requested_uri,"/remove") == 0){
			lwsl_user("Received remove \n");
			remove_accelerator(atoi(arg));
		}
		else if(strcmp(requested_uri,"/getInFD") == 0){
			lwsl_user("Received getInFD\n");
			getInputFD(atoi(arg));
		}
		else if(strcmp(requested_uri,"/getOutFD") == 0){
			lwsl_user("received getOutFD \n");
			getOutputFD(atoi(arg));
		}
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

	printf("Starting http daemon\n");
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
