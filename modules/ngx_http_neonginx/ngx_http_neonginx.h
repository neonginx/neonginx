#define NEONGINX_API_BASE "/neonginx/api/"
#define NEONGINX_BIN_FILE "/etc/nginx/neonginx.bin"
#define NEONGINX_SESSION_MAXAGE 3600
#define NEONGINX_PASSWORD_LEN 24
#define NEONGINX_SALT_LEN 64
#define NEONGINX_API_V1_URI_MAXLENGTH 256

typedef struct {
	unsigned long password;
	char salt[NEONGINX_SALT_LEN + 1];
	ngx_shmtx_t neonginx_mutex;
} ngx_http_neonginx_main_conf_t;

typedef struct {
	ngx_atomic_t total_requests;
	ngx_atomic_t bytes_in;
	ngx_atomic_t bytes_out;
	// ngx_shmtx_t  mutex;
} ngx_http_neonginx_srv_conf_t;

static void *ngx_http_neonginx_create_srv_conf(ngx_conf_t *cf);
static void *ngx_http_neonginx_create_mainconf(ngx_conf_t *cf);
static ngx_int_t ngx_http_neonginx_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_neonginx_preaccess_phase_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_neonginx_log_phase_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_neonginx_print(ngx_http_request_t *r, char *string, ngx_uint_t status);

static ngx_http_module_t  ngx_http_neonginx_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_neonginx_init,         	   /* postconfiguration */

    ngx_http_neonginx_create_mainconf,     /* create main configuration */
    NULL,                                  /* init main configuration */

    ngx_http_neonginx_create_srv_conf,      /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,        													 /* create location configuration */
    NULL        													 /* merge location configuration */
};

ngx_module_t  ngx_http_neonginx_module = {
    NGX_MODULE_V1,
    &ngx_http_neonginx_module_ctx,         /* module context */
    NULL,           											 /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,         												 /* exit process */
    NULL,         												 /* exit master */
    NGX_MODULE_V1_PADDING
};


/* Other Libraries */
#include "lib/jWrite.c"

/* NeoNginx Stuffs */
#include "ngx_http_neonginx_utils.c"
#include "ngx_http_neonginx_api_v1.c"
