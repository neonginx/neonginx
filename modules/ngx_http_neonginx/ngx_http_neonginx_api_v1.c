static ngx_int_t ngx_http_neonginx_api_v1(ngx_http_request_t *r, ngx_http_neonginx_main_conf_t *nmcf);
static ngx_int_t ngx_http_neonginx_api_v1_stats(ngx_http_request_t *r, ngx_http_neonginx_main_conf_t *nmcf);

#define NEONGINX_API_V1_BUFSIZE 10485760 // 10MB

#define NEONGINX_API_V1_MSG_VERSION_INVALID        "{\"STATUS\": 0, \"MESSAGE\": \"INVALID API VERSION PROVIDED.\"}"
#define NEONGINX_API_V1_MSG_REQUEST_TOOLONG 	   "{\"STATUS\": 0, \"MESSAGE\": \"REQUEST URI TOO LONG.\"}"
#define NEONGINX_API_V1_MSG_LOGIN_SUCCESS          "{\"STATUS\": 1, \"MESSAGE\": \"LOGGED IN SUCCESFULLY\", \"SESSION\":\"%s\"}"
#define NEONGINX_API_V1_MSG_LOGIN_INVALID          "{\"STATUS\": 0, \"MESSAGE\": \"NO PASSWORD PROVIDED\"}"
#define NEONGINX_API_V1_MSG_LOGIN_FAIL             "{\"STATUS\": 0, \"MESSAGE\": \"WRONG CREDENTIALS\"}"
#define NEONGINX_API_V1_MSG_ENDPOINT_INVALID       "{\"STATUS\": 0, \"MESSAGE\": \"INVALID ENDPOINT PROVIDED.\"}"
#define NEONGINX_API_V1_MSG_AUTH_INVALID           "{\"STATUS\": 0, \"MESSAGE\": \"INVALID OR EXPIRED AUTHENTICATION HEADER PROVIDED.\"}"
#define NEONGINX_API_V1_MSG_AUTH_MISSING           "{\"STATUS\": 0, \"MESSAGE\": \"AUTHENTICATION HEADER NOT PROVIDED.\"}"
#define NEONGINX_API_V1_MSG_BUFFEROVERFLOW         "{\"STATUS\": 0, \"MESSAGE\": \"OUTPUT JSON CAUSED A BUFFER OVERFLOW (>10MB).\"}"

static ngx_int_t ngx_http_neonginx_api_v1(ngx_http_request_t *r, ngx_http_neonginx_main_conf_t *nmcf){
	ngx_http_neonginx_srv_conf_t *prova2 = ngx_http_get_module_srv_conf(r, ngx_http_neonginx_module);
	unsigned long password_hash;
	ngx_http_variable_value_t *auth_header_variable, *password_header_variable;
	ngx_str_t auth_header_name = ngx_string("http_neonginx_auth");
	ngx_str_t password_header_name = ngx_string("http_neonginx_password");
	char *endpoint, *session, *buffer;

	// Read X-Auth Header
	auth_header_variable = ngx_pcalloc(r->pool, sizeof(ngx_http_variable_value_t));
	if(ngx_http_variable_unknown_header(auth_header_variable, &auth_header_name, &r->headers_in.headers.part, sizeof("http_") - 1) != NGX_OK){
	 return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	// Read X-NeoNginx-Password Header
	password_header_variable = ngx_pcalloc(r->pool, sizeof(ngx_http_variable_value_t));
	if(ngx_http_variable_unknown_header(password_header_variable, &password_header_name, &r->headers_in.headers.part, sizeof("http_") - 1) != NGX_OK){
	 return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	endpoint = ngx_pcalloc(r->pool, NEONGINX_API_V1_URI_MAXLENGTH*sizeof(char));
	sscanf(r->uri.data, NEONGINX_API_BASE "v1/%s", endpoint);

	// Login Case
	if(strcmp(endpoint, "login") == 0){
		// Check if the password header is set
		if(password_header_variable->len == 0){
			return ngx_http_neonginx_print(r, NEONGINX_API_V1_MSG_LOGIN_INVALID, NGX_HTTP_FORBIDDEN);
		}

		// Check if the provided password is correct
		password_hash = hash_djb2(password_header_variable->data);
		if(password_hash == nmcf->password){
			session = generateSession(nmcf->salt, r->pool);
			buffer = ngx_pcalloc(r->pool, strlen(session) + sizeof(NEONGINX_API_V1_MSG_LOGIN_SUCCESS));
			ngx_sprintf(buffer, NEONGINX_API_V1_MSG_LOGIN_SUCCESS, session);
			return ngx_http_neonginx_print(r, buffer, NGX_HTTP_OK);
		}

		// Otherwise
		return ngx_http_neonginx_print(r, NEONGINX_API_V1_MSG_LOGIN_FAIL, NGX_HTTP_FORBIDDEN);
	}

	// If the request it's not a login request, we'll check for the session validity
	// Check if the auth header is set
	if(auth_header_variable->len == 0){
		return ngx_http_neonginx_print(r, NEONGINX_API_V1_MSG_AUTH_MISSING, NGX_HTTP_UNAUTHORIZED);
	}
	if(validateSession(auth_header_variable->data, nmcf->salt, r->pool) == 0){
		return ngx_http_neonginx_print(r, NEONGINX_API_V1_MSG_AUTH_INVALID, NGX_HTTP_UNAUTHORIZED);
	}

	// Stats Case
	if(strcmp(endpoint, "stats") == 0){
		return ngx_http_neonginx_api_v1_stats(r, nmcf);
	}
	return ngx_http_neonginx_print(r, NEONGINX_API_V1_MSG_ENDPOINT_INVALID, NGX_HTTP_NOT_FOUND);
}

static ngx_int_t ngx_http_neonginx_api_v1_stats(ngx_http_request_t *r, ngx_http_neonginx_main_conf_t *nmcf){
	ngx_http_core_srv_conf_t       **cscfp;
	ngx_http_core_main_conf_t      *cmcf;
	ngx_http_neonginx_srv_conf_t   *nscf;
	ngx_str_t                      *server_name, *upstream_name;
	ngx_http_upstream_srv_conf_t   *us, **uscfp;
    ngx_http_upstream_main_conf_t  *umcf;
	ngx_http_upstream_server_t     *server;
	ngx_http_upstream_rr_peers_t   *peers;
	int                             err;
	struct jWriteControl            jwc;

	u_char *ngx_page = ngx_pcalloc(r->pool, sizeof(char)*NEONGINX_API_V1_BUFSIZE+1);

	jwOpen(&jwc, ngx_page, NEONGINX_API_V1_BUFSIZE, JW_OBJECT, 0);

	cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module); // get the core main configuration
	cscfp = cmcf->servers.elts; // all servers's configurations

	jwObj_int(&jwc, "STATUS", 1);
	jwObj_int(&jwc, "REQUESTS_TOTAL", *ngx_stat_requests);
	jwObj_int(&jwc, "ACTIVE_CONNECTIONS", *ngx_stat_active);

	/* SERVERS STATS */
	jwObj_array(&jwc, "SERVERS" );
	for (int i = 0; i < cmcf->servers.nelts; i++) { // for each server
		server_name = &cscfp[i]->server_name; // server name
		nscf = cscfp[i]->ctx->srv_conf[ngx_http_neonginx_module.ctx_index]; // neonginx srv conf of server block X

		jwArr_object(&jwc );
			jwObj_string(&jwc, "NAME", server_name->data, server_name->len);
			jwObj_int(&jwc, "REQUESTS_TOTAL", nscf->total_requests);
			jwObj_int(&jwc, "BYTES_IN", nscf->bytes_in);
			jwObj_int(&jwc, "BYTES_OUT", nscf->bytes_out);
		jwEnd(&jwc );
	}
	jwEnd(&jwc );

	/* UPSTREAMS STATS */
	umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module); // get the upstream main conf
    uscfp = umcf->upstreams.elts; // all the upstream's configurations
	jwObj_array(&jwc, "UPSTREAMS" );
    for (int i = 0; i < umcf->upstreams.nelts; i++) {
		us = uscfp[i];
		peers = us->peer.data;
		upstream_name = &us->host;
		if(us->flags != 0){ // if has flags (we only want to show "declared" upstream and not the "auto-generated" proxy_pass's upstreams)
			jwArr_object(&jwc);
				jwObj_string(&jwc, "NAME", upstream_name->data, upstream_name->len);
				jwObj_array(&jwc, "PEERS" );
					if(us->servers){
						server = us->servers->elts;
						for (int j = 0; j < us->servers->nelts; j++) {
							jwArr_object(&jwc );
								jwObj_string(&jwc, "NAME", peers->peer[j].name.data, peers->peer[j].name.len);
								jwObj_int(&jwc, "CHECKED", peers->peer[j].checked);
								jwObj_int(&jwc, "FAILS", peers->peer[j].fails);
								jwObj_int(&jwc, "MAX_FAILS", peers->peer[j].max_fails);
								jwObj_int(&jwc, "WEIGHT", peers->peer[j].weight);
								jwObj_int(&jwc, "CONNECTIONS", peers->peer[j].conns);
							jwEnd(&jwc);
						}
					}
				jwEnd(&jwc );
			jwEnd(&jwc );
		}
	}
	jwEnd(&jwc );

	err= jwClose(&jwc );
	if(err != JWRITE_OK){
		ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "NEONGINX Error: %s at function call %d\n", jwErrorToString(err), jwErrorPos(&jwc ) );
		return ngx_http_neonginx_print(r, NEONGINX_API_V1_MSG_BUFFEROVERFLOW, NGX_HTTP_INTERNAL_SERVER_ERROR);
	}
	return ngx_http_neonginx_print(r, ngx_page, NGX_HTTP_OK);
}
