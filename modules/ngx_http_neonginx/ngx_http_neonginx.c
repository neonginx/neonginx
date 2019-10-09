/*
	CREDITS
	Module Developed By Andrea Picone (Telegram: @Vaiiry, GitHub: @imndr)
	DashBoard UI: 				Based on https://github.com/puikinsh/CoolAdmin
	JSON Encoding Lib:		jWrite, developed by TonyWilk
*/

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_neonginx.h"

static void *ngx_http_neonginx_create_srv_conf(ngx_conf_t *cf){
    ngx_http_neonginx_srv_conf_t  *nscf;
	ngx_shm_t shm;
	u_char *shared;
	shm.size = sizeof(ngx_http_neonginx_srv_conf_t);
    if (ngx_shm_alloc(&(shm)) != NGX_OK) {
        return NGX_ERROR;
    }
    shared = shm.addr;
	nscf = (ngx_http_neonginx_srv_conf_t *) shared;

    return nscf;
}

static void *ngx_http_neonginx_create_mainconf(ngx_conf_t *cf){

	ngx_http_neonginx_main_conf_t  *nmcf;
	FILE *fp;
	char *password,*salt;

	srand(time(NULL));

	nmcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_neonginx_main_conf_t));
	if (nmcf == NULL) {
	  return NULL;
	}

	fp = fopen(NEONGINX_BIN_FILE,"rb");
	if(fp == NULL){
		// Configuration Initialization

		// Generate a new random password
		password = generateRandomString(cf->pool, NEONGINX_PASSWORD_LEN + 1);
		ngx_log_error(NGX_LOG_EMERG, ngx_cycle->log, 0,
									"NeoNginx: Your new DashBoard root password: %s", password);
		nmcf->password = hash_djb2(password);

		// Generate a new random salt (used for the session validation)
		salt = generateRandomString(cf->pool, NEONGINX_SALT_LEN + 1);
		strncpy(nmcf->salt, salt, NEONGINX_SALT_LEN);

		// Save the configuration to the file for persistent storage
		fp = fopen(NEONGINX_BIN_FILE,"wb");
		if(fp == NULL){
			ngx_log_error(NGX_LOG_EMERG, ngx_cycle->log, 0,
										"NeoNginx: unable to write " NEONGINX_BIN_FILE);
			return NULL;
		} else {
			fwrite(nmcf,sizeof(ngx_http_neonginx_main_conf_t),1,fp);
			fclose(fp);
		}
	} else {
		// Load configuration from file
		fread(nmcf,sizeof(ngx_http_neonginx_main_conf_t),1,fp);
		fclose(fp);
	}

	return nmcf;
}


static ngx_int_t ngx_http_neonginx_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h1,*h2;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h1 = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    h2 = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers);
    if (h1 == NULL || h2 == NULL) {
        return NGX_ERROR;
    }

    *h1 = ngx_http_neonginx_log_phase_handler;
		*h2 = ngx_http_neonginx_preaccess_phase_handler;

    return NGX_OK;
}

static ngx_int_t ngx_http_neonginx_preaccess_phase_handler(ngx_http_request_t *r){

	unsigned int version = 0;
	ngx_http_neonginx_main_conf_t *nmcf;

	nmcf = ngx_http_get_module_main_conf(r, ngx_http_neonginx_module);

	if(r->uri.len >= sizeof(NEONGINX_API_BASE) - 1){
		if(strncmp(r->uri.data, NEONGINX_API_BASE, sizeof(NEONGINX_API_BASE) - 1) == 0){
			sscanf(r->uri.data, NEONGINX_API_BASE "v%u", &version);
			switch(version){
				case 1:
					if(r->uri.len > NEONGINX_API_V1_URI_MAXLENGTH){
						return ngx_http_neonginx_print(r, NEONGINX_API_V1_MSG_REQUEST_TOOLONG, NGX_HTTP_BAD_REQUEST);
					}
					// API VERSION 1
					return ngx_http_neonginx_api_v1(r, nmcf);
					break;
				default:
					return ngx_http_neonginx_print(r, NEONGINX_API_V1_MSG_VERSION_INVALID, NGX_HTTP_NOT_FOUND);
			}
		}
	}
	return NGX_DECLINED;
}


static ngx_int_t ngx_http_neonginx_log_phase_handler(ngx_http_request_t *r){
	ngx_http_neonginx_srv_conf_t  *nscf = ngx_http_get_module_srv_conf(r, ngx_http_neonginx_module);
	ngx_http_neonginx_main_conf_t *nmcf = ngx_http_get_module_main_conf(r, ngx_http_neonginx_module);
	ngx_atomic_fetch_add(&nscf->total_requests, 1);
	ngx_atomic_fetch_add(&nscf->bytes_out, r->connection->sent);
	ngx_atomic_fetch_add(&nscf->bytes_in, r->request_length);
	return NGX_DECLINED;
}
