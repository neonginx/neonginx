static ngx_int_t ngx_http_neonginx_print(ngx_http_request_t *r, char *string, ngx_uint_t status){
    ////////////
  	// HEADER //
  	////////////

	  // content type header
    r->headers_out.content_type.len = sizeof("application/json")-1;
    r->headers_out.content_type.data = (u_char *) "application/json";
	  // status code (HTTP_CODE) 200
    r->headers_out.status = status;
    // header della content_length
    r->headers_out.content_length_n = strlen(string);

    ngx_http_send_header(r); // invia gli header

  	//////////
  	// BODY //
  	//////////
    ngx_buf_t *b;
    ngx_chain_t out;

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    out.buf = b;
    out.next = NULL;

    b->pos = string;
    b->last = string + strlen(string);
    b->memory = 1;
    b->last_buf = 1;

    ngx_http_output_filter(r, &out);
    ngx_http_finalize_request(r, 0);
    return NGX_DONE;
}


// Function to swap to chars, just for more clean code
void swap(char *x, char *y) {
  char t = *x; *x = *y; *y = t;
}

// function to implement reverse() function in C
char* reverse(char *buffer, int i, int j){
  while (i < j){
    swap(&buffer[i++], &buffer[j--]);
  }
  return buffer;
}

// Iterative function to implement itoa() function in C
char* itoa(int value, char* buffer, int base){
  // invalid input
  if (base < 2 || base > 32)
    return buffer;
  int n = abs(value);
  int i = 0;
  while (n){
    int r = n % base;
    if (r >= 10)
      buffer[i++] = 65 + (r - 10);
    else
      buffer[i++] = 48 + r;

    n = n / base;
  }

  // if number is 0
  if (i == 0)
    buffer[i++] = '0';

  if (value < 0 && base == 10)
    buffer[i++] = '-';

  buffer[i] = '\0'; // null terminate string
  // reverse the string and return it
  return reverse(buffer, 0, i - 1);
}

// Hash that will be used for the user authentication (DJB2 Algorithm)
const unsigned long hash_djb2(unsigned char *str){
	unsigned long hash = 5381;
	int c;
	while (c = *str++){
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
	return hash;
}

// Function to get a random number, giving max/min, just for more clean code
const unsigned int randomNumber(unsigned int min, unsigned int max){
  return (rand() % (max + 1 - min)) + min;
}

// Function to get a random string, buffer will be allocated in the given pool
const char *generateRandomString(ngx_pool_t *pool, unsigned int length){
  char characters[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRST"
                      "UVWXYZ!$()[]-_?@#*+";
  size_t characters_len = (sizeof(characters)/sizeof(char))-1;
  char *randomString = ngx_pcalloc(pool, (length+1)*sizeof(char));
  for(int i = 0; i < length; i++){
    randomString[i] = characters[randomNumber(0, characters_len-1)];
  }
  return randomString;
}

// Function to validate session token
const unsigned int validateSession(char *str, char *salt, ngx_pool_t *pool){
  unsigned long input_hash, hash;
  unsigned long input_timestamp;
  char *tmp, *input_timestamp_str = ngx_pcalloc(pool, sizeof(char)*NGX_INT_T_LEN + 1);
  sscanf(str, "%lu:%lu", &input_hash, &input_timestamp);
  tmp = ngx_pcalloc(pool, sizeof(char)*(NGX_INT_T_LEN + NEONGINX_SALT_LEN + 1));
  itoa(input_timestamp, input_timestamp_str, 10);
  strcat(tmp, input_timestamp_str); // Concat the timestamp
  strcat(tmp, salt);
  hash = hash_djb2(tmp);
  if(hash == input_hash){
    if(time(NULL) - input_timestamp < NEONGINX_SESSION_MAXAGE){ // If D < MAXAGE
      return 1;
    }
  }
  return 0;

}

// Function to generate session token
const char *generateSession(char *salt, ngx_pool_t *pool){
  char *tmp, *input_timestamp_str = ngx_pcalloc(pool, sizeof(char)*NGX_INT_T_LEN + 1);
  unsigned long timestamp = time(NULL);
  unsigned long hash;
  tmp = ngx_pcalloc(pool, sizeof(char)*(NGX_INT_T_LEN + NEONGINX_SALT_LEN) + 1);
  itoa(timestamp, input_timestamp_str, 10);
  strcat(tmp, input_timestamp_str); // Concat the timestamp
  strcat(tmp, salt);
  hash = hash_djb2(tmp);
  ngx_pfree(pool, tmp);
  tmp = ngx_pcalloc(pool, sizeof(char)*(NGX_INT_T_LEN + sizeof(":") + NGX_INT_T_LEN + 1));
  sprintf(tmp, "%lu:%lu", hash, timestamp);
  return tmp;
}
