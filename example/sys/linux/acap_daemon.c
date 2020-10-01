#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <acapd/accel.h>
#include <acapd/shell.h>
#include <sys/stat.h>

static int interrupted;
static acapd_accel_t *active_slots[3];
char *default_shell = "/lib/firmware/xilinx";

struct pss {
	struct lws_spa *spa;
};

/*static const char * const param_names[] = {
	"text1",
};

enum enum_param_names {
    EPN_TEXT1,
};*/

char *find_accel(const char *name, int slot)
{
	char *accel_path = malloc(sizeof(char)*1024);
	DIR *d;
	struct dirent *dir;
	struct stat info;

    d = opendir(default_shell);
    if (d == NULL) {
		acapd_perror("Directory %s not found\n",default_shell);
	}
	while((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_DIR) {
			if (strlen(dir->d_name) > 64) {
				continue;
			}
			if(strcmp(dir->d_name, name) == 0){
				acapd_debug("Found dir %s in %s\n",name,default_shell);
				sprintf(accel_path,"%s/%s/%s_slot%d",default_shell,dir->d_name,dir->d_name,slot);
				if (stat(accel_path,&info) != 0)
					return NULL;
				if (info.st_mode & S_IFDIR){
					acapd_debug("Found accelerator path %s\n",accel_path);
					return accel_path;
				}
				else
					acapd_perror("No %s accel for slot %d found\n",name,slot);
			}
			else {
				acapd_debug("Found %s in %s\n",dir->d_name,default_shell);
			}
		}
	}
	return NULL;
}
void getRMInfo()
{
	FILE *fptr;
	int i;
	char line[512];
	acapd_accel_t *accel;

	fptr = fopen("/home/root/rm_info.txt","w");
	if (fptr == NULL){
		acapd_perror("Couldn't create /home/root/rm_info.txt");
		goto out;
	}

	for (i = 0; i < 3; i++) {
		if(active_slots[i] == NULL)
			sprintf(line, "%d,%s",i,"None");
		else {
			accel = active_slots[i];
			sprintf(line,"%d,%s",i,accel->pkg->name);
		}
		fprintf(fptr,"\n%s",line);	
	}
out:
	fclose(fptr);
}
int load_accelerator(const char *accel_name, const char *shell)
{
	int i, ret;
	char *path;
	acapd_accel_t *accel = malloc(sizeof(acapd_accel_t));
	acapd_accel_pkg_hd_t *pkg = malloc(sizeof(acapd_accel_pkg_hd_t));
	FILE *fptr;

	//if (active_slots == NULL)
	//{	
	//	printf("%s allocating active_slots\n",__func__);
	//	active_slots = calloc(3, sizeof(acapd_accel_t *));
	//}
	fptr = fopen("/home/root/slot.txt","w");
	if (fptr == NULL)
		acapd_perror("Couldn't create /home/root/slot.txt");

	for (i = 0; i < 3; i++) {
		if (active_slots[i] == NULL){
			path = find_accel(accel_name, i);
			if (path == NULL){
				printf("No accel package found for %s slot %d\n",accel_name,i);
				continue;
			}
			pkg->name = path;
			pkg->type = ACAPD_ACCEL_PKG_TYPE_NONE;
			init_accel(accel, pkg);
			printf("Loading accel %s to slot %d \n", pkg->name,i);
			accel->rm_slot = i;
			/* Set rm_slot before load_accel() so isolation for appropriate slot can be applied*/

			ret = load_accel(accel, shell, 0);
			if (ret < 0){
				acapd_perror("%s: Failed to load accel %s\n",__func__,accel_name);
				fprintf(fptr,"%d",-1);
				fclose(fptr);
				return -1;
			}
			active_slots[i] = accel;
			fprintf(fptr,"%d",i);
			getRMInfo();
			printf("Loaded accel succesfully \n");
			fclose(fptr);
			return accel->rm_slot;
		}
	}
	if (i >= 3){
		printf("Couldn't find empty slot for %s\n",accel_name);
		fprintf(fptr,"%d",-1);
	}
	fclose(fptr);
	return -1;
}

void remove_accelerator(int slot)
{
	acapd_accel_t *accel = active_slots[slot];
	if (active_slots == NULL || active_slots[slot] == NULL){
		printf("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	printf("Removing accel %s from slot %d\n",accel->sys_info.tmp_dir,slot);
    remove_accel(accel, 0);
	free(accel);
	active_slots[slot] = NULL;
	getRMInfo();
}
void getFD(int slot)
{
	printf("%s: received\n",__func__);
	acapd_accel_t *accel = active_slots[slot];
	if (active_slots == NULL || active_slots[slot] == NULL){
		printf("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	get_fds(accel, slot);
}
void getShellFD(int slot)
{
	acapd_accel_t *accel = active_slots[slot];
	if (active_slots == NULL || active_slots[slot] == NULL){
		printf("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	get_shell_fd(accel);
}
void getClockFD(int slot)
{
	acapd_accel_t *accel = active_slots[slot];
	if (active_slots == NULL || active_slots[slot] == NULL){
		printf("%s No Accel in slot %d\n",__func__,slot);
		return;
	}
	get_shell_clock_fd(accel);
}

struct msg{
    char cmd[32];
    char arg[32];
};
struct resp{
    char data[128];
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
			printf("LWS_CALLBACK_ESTABLISHED user %s\n",(char *)user);
			break;

        case LWS_CALLBACK_RECEIVE:
            printf("LWS_CALLBACK_RECEIVE len%ld\n",len);
            memcpy( &received_payload.data[LWS_SEND_BUFFER_PRE_PADDING], in, len );
            received_payload.len = len;
            m = (struct msg *)&received_payload.data[LWS_SEND_BUFFER_PRE_PADDING];
            printf("server received cmd %s arg %s\n",m->cmd,m->arg);
			msgs++;
			if(strcmp(m->cmd,"-loadpdi") == 0){
				lwsl_user("Received %s \n",m->cmd);
				slot = load_accelerator(m->arg,NULL);
				sprintf(r->data,"%d",slot);
				r->len = 1;
				printf("daemon: load done slot %s len %d\n", r->data, r->len);
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-remove") == 0){
				lwsl_user("Received %s slot %s\n",m->cmd,m->arg);
				r->len = 0;
				remove_accelerator(atoi(m->arg));
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-getFD") == 0){
				lwsl_user("Received %s slot %s\n",m->cmd,m->arg);
				getFD(atoi(m->arg));
				sprintf(r->data,"%s","");
				r->len = 0;
				lws_callback_on_writable( wsi );
				lwsl_user("Server recieve done slot %s\n",m->arg);
			}
			else if(strcmp(m->cmd,"-getShellFD") == 0){
				lwsl_user("Received %s slot %s\n",m->cmd,m->arg);
				sprintf(r->data,"%s","");
				r->len = 0;
				getShellFD(atoi(m->arg));
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-getClockFD") == 0){
				lwsl_user("Received %s slot %s\n",m->cmd,m->arg);
				sprintf(r->data,"%s","");
				r->len = 0;
				getClockFD(atoi(m->arg));
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else if(strcmp(m->cmd,"-getRMInfo") == 0){
				lwsl_user("Received -getRMInfo\n");
				sprintf(r->data,"%s","");
				r->len = 0;
				getRMInfo();
				lws_callback_on_writable_all_protocol( lws_get_context( wsi ), lws_get_protocol( wsi ) );
			}
			else {
				lwsl_user("cmd not recognized\n");
				//return -1;
			}
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
			if (!msgs)
				return 0;
            printf("LWS_CALLBACK_SERVER_WRITEABLE resp->len %d\n",r->len);
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


/*static char *requested_uri;
static const char *arg;
static int
callback_http(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct pss *pss = (struct pss *)user;
	int n;	
	switch (reason) {
	case LWS_CALLBACK_HTTP:
		acapd_debug("server LWS_CALLBACK_HTTP\n");
		requested_uri = (char *)in;
		break;

	case LWS_CALLBACK_HTTP_BODY:
		acapd_debug("server LWS_CALLBACK_HTTP_BODY\n");
		if(!pss->spa) {
			pss->spa = lws_spa_create(wsi, param_names, LWS_ARRAY_SIZE(param_names), 1024, NULL, NULL);
			if(!pss->spa)
				return -1;
		}
		if (lws_spa_process(pss->spa, in, (int)len))
			return -1;
		break;

	case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		lwsl_user("server LWS_CALLBACK_HTTP_BODY_COMPLETION\n");
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
		else if(strcmp(requested_uri,"/getFD") == 0){
			lwsl_user("Received getFD\n");
			getFD(atoi(arg));
		}
		else if(strcmp(requested_uri,"/getShellFD") == 0){
			lwsl_user("Received getShellFD\n");
			getShellFD(atoi(arg));
		}
		else if(strcmp(requested_uri,"/getClockFD") == 0){
			lwsl_user("Received getClockFD\n");
			getClockFD(atoi(arg));
		}
		else if(strcmp(requested_uri,"/getRMInfo") == 0){
			lwsl_user("Received getRMInfo\n");
			getRMInfo();
		}
		//if (pss->spa && lws_spa_destroy(pss->spa))
		//	return -1;
		if (lws_http_transaction_completed(wsi)) {
			return -1;
		}
	
		break;

	case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
		printf("server LWS_CALLBACK_HTTP_DROP_PROTOCOL\n");
		if (pss->spa) {
			lws_spa_destroy(pss->spa);
			pss->spa = NULL;
		}
		break;
	
	default:
		break;
	}

	return 0;//lws_callback_http_dummy(wsi, reason, user, in, len);
}*/

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
	printf("server interrupted signal %d \n",sig);
	interrupted = 1;
}


int main(int argc, const char **argv)
{
	struct lws_context_creation_info info;
	struct lws_context *context;
	const char *p;
	int ret;

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

	info.port = 7681;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return 1;
	}

	/* http on 7681 */

	//info.mounts = &mount;
	info.vhost_name = "localhost";

	if (!lws_create_vhost(context, &info)) {
		lwsl_err("Failed to create tls vhost\n");
		goto bail;
	}
	ret = load_full_bitstream(default_shell);
	printf("Loaded Full bitstream %s (ret:%d)\n",default_shell,ret);
	while (n >= 0 && !interrupted)
		n = lws_service(context, 0);

bail:
	lws_context_destroy(context);

	return 0;
}
