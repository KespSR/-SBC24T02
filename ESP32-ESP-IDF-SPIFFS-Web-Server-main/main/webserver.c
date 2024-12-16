#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include <esp_http_server.h>
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "mbedtls/md.h"
#include "connect_wifi.h"
#include "thingsboard_send.h"

#include "esp_tls_crypto.h"

static const char *TAG = "WEBSERVER"; // TAG for debug
int door_state = 0;
char door_status[32]="unknown";
#define INDEX_HTML_PATH "/spiffs/index.html"
#define ADD_USER_HTML_PATH "/spiffs/add_user.html"
#define ALLOWED_MACS_PATH "/spiffs/macs.txt"
#define CREDENTIALS_PATH "/spiffs/creds.txt"
#define MAX_USERS 5
#define PASS_LENGTH 32
char index_html[4096];
char add_user_html[2048];
char response_data[4096];
int registered_users=0;
unsigned char pass_list[MAX_USERS][PASS_LENGTH];






static unsigned char * cypher(char *payload,unsigned char hmacResult[]){  
	char *key = "AWiseManFear";
  
  
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
 
  const size_t payloadLength = strlen(payload);
  const size_t keyLength = strlen(key);            
 
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, keyLength);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  ESP_LOGI(TAG,"%u", (unsigned int)hmacResult);
  return hmacResult;
  }










typedef esp_err_t (*page_handler_t)(httpd_req_t *req);
typedef struct {
    char    *username;
    char    *password;
	page_handler_t authorized_func;
	int user_id;
} basic_auth_info_t;

#define HTTPD_401      "401 UNAUTHORIZED"           /*!< HTTP Response 401 */


esp_err_t send_web_page(httpd_req_t *req)
{
    int response;
    sprintf(response_data, index_html, door_status);
    response = httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return response;
}
static char *http_auth_basic(const char *username, const char *password)
{
	ESP_LOGI(TAG, "name %c", *username);
    size_t out;
    char *user_info = NULL;
    char *digest = NULL;
    size_t n = 0;
    int rc = asprintf(&user_info, "%s:%s", username, password);
    if (rc < 0) {
        ESP_LOGE(TAG, "asprintf() returned: %d", rc);
        return NULL;
    }

    if (!user_info) {
        ESP_LOGE(TAG, "No enough memory for user information");
        return NULL;
    }
    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

    /* 6: The length of the "Basic " string
     * n: Number of bytes for a base64 encode format
     * 1: Number of bytes for a reserved which be used to fill zero
    */
    digest = calloc(1, 6 + n + 1);
    if (digest) {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, &out, (const unsigned char *)user_info, strlen(user_info));
    }
    free(user_info);
    return digest;
}

/* An HTTP GET handler */
static esp_err_t basic_auth_get_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    basic_auth_info_t *basic_auth_info =req->user_ctx ;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1) {
        buf = calloc(1, buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "No enough memory for basic authorization");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Authorization: %s", buf);
        } else {
            ESP_LOGE(TAG, "No auth value received");
        }
		bool authed=false;
		unsigned char auth_credentials[PASS_LENGTH] ;
		cypher(http_auth_basic((char*)pass_list[0], (char*)pass_list[1]),auth_credentials);
		unsigned char user_creds[PASS_LENGTH] ;
		cypher(buf,user_creds);
		for (int i=0;i<MAX_USERS;i++){
			memcpy(auth_credentials , pass_list[i],PASS_LENGTH);
			if (!buf) {
				ESP_LOGE(TAG, "No enough memory for basic authorization credentials");
				free(buf);
				return ESP_ERR_NO_MEM;
			}
			//ESP_LOGE(TAG, "%s    %s         %d",auth_credentials, user_creds,memcmp(auth_credentials, user_creds, PASS_LENGTH));
			if (memcmp(auth_credentials, user_creds, PASS_LENGTH)==0){
				authed=true;
				basic_auth_info->user_id=i;
				i=MAX_USERS+1;
			}
		}
        if (!authed) {
            ESP_LOGE(TAG, "Not authenticated");
            httpd_resp_set_status(req, HTTPD_401);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
            httpd_resp_send(req, NULL, 0);
        } else {
            ESP_LOGI(TAG, "Authenticated!");
            char *basic_auth_resp = NULL;
            httpd_resp_set_status(req, HTTPD_200);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            int rc = asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", basic_auth_info->username);
            if (rc < 0) {
                ESP_LOGE(TAG, "asprintf() returned: %d", rc);
               // free(auth_credentials);
                return ESP_FAIL;
            }
            if (!basic_auth_resp) {
                ESP_LOGE(TAG, "No enough memory for basic authorization response");
                //free(auth_credentials);
                free(buf);
                return ESP_ERR_NO_MEM;
            }
			httpd_resp_set_type(req, "text/html");
			//send_web_page(req);
			if (basic_auth_info->authorized_func) {
				(basic_auth_info->authorized_func)(req);
			} else {
				ESP_LOGE(TAG, "Authorized function not set");
}
			//(basic_auth_info->authorized_func)(req);
            //httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
            free(basic_auth_resp);
        }
        free(buf);
    } else {
        ESP_LOGE(TAG, "No auth header received");
        httpd_resp_set_status(req, HTTPD_401);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
        httpd_resp_send(req, NULL, 0);
    }
    return ESP_OK;
}


static void httpd_register_basic_auth(httpd_handle_t server,char* uri_,page_handler_t authed_func)
{
	ESP_LOGI(TAG, "Security added");
    basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
	httpd_uri_t basic_auth = {
		.uri       = uri_,
		.method    = HTTP_GET,
		.handler   = basic_auth_get_handler,
	};
    if (basic_auth_info) {
        basic_auth_info->username = "c";
        basic_auth_info->password = "d";
		basic_auth_info->authorized_func=authed_func;
		basic_auth_info->user_id=-1;
        basic_auth.user_ctx = basic_auth_info;
        httpd_register_uri_handler(server, &basic_auth);
    }
	//free(basic_auth_info);
}

static void initi_web_page_buffer(void)
{
    memset((void *)index_html, 0, sizeof(index_html));
    struct stat st;
    if (stat(INDEX_HTML_PATH, &st))
    {
        ESP_LOGE(TAG, "index.html not found");
        return;
    }

    FILE *fp = fopen(INDEX_HTML_PATH, "r");
    if (fread(index_html, st.st_size, 1, fp) == 0)
    {
        ESP_LOGE(TAG, "fread failed");
    }
    fclose(fp);
}

static void load_from_memory(char* buffer, char* path)
{
    memset((void *)buffer, 0, sizeof(buffer));
    struct stat st;
    if (stat(path, &st))
    {
        ESP_LOGE(TAG, "%s not found",path);
        return;
    }

    FILE *fp = fopen(path, "r");
    if (fread(buffer, st.st_size, 1, fp) == 0)
    {
        ESP_LOGE(TAG, "fread failed");
    }
    fclose(fp);
}
void show_user(unsigned char user[]){
    for (int i = 0; i < PASS_LENGTH; i++) {
        printf("%02x ", user[i]);  // Print each byte in hexadecimal
    }
	printf("\n");
}
void show_users(void){
	ESP_LOGI(TAG,"NUMBER OF USERS:  %d",registered_users);
	for (int i=0;i<registered_users;i++){
		show_user(pass_list[i]);
	}
}

static void load_userSet(void)
{
    struct stat st;
    if (stat(CREDENTIALS_PATH, &st))
    {
        ESP_LOGE(TAG, "pass file not found");
        return;
    }

    FILE *fp = fopen(CREDENTIALS_PATH, "rb");
	    // Ensure the file was opened successfully
    if (!fp){
        printf("Error opening password file.\n");
        return;
    }
	unsigned char *pass_list_buf = (unsigned char *)malloc(PASS_LENGTH);
    if (pass_list_buf == NULL) {
        perror("Memory allocation failed");
        fclose(fp);
        return;
    }

	//pread(int fildes, void *buf, size_t nbyte, off_t offset); [Option End]
	size_t bytesRead;
	int round=0;//fscanf(fp,"%s",pass_list_buf)
	registered_users=0;
	while (round<MAX_USERS && (bytesRead = fread(pass_list_buf, sizeof(unsigned char), PASS_LENGTH, fp)) > 0){
      // Null-terminate the buffer
		memcpy(pass_list[round],pass_list_buf,PASS_LENGTH) ;
		show_user(pass_list_buf);
		round++;
		registered_users++;
    }
	if (registered_users==0){
		ESP_LOGE(TAG,"No users found, setting default");
		cypher(http_auth_basic("admin","admin"),pass_list_buf);
		memcpy(pass_list[0],pass_list_buf,PASS_LENGTH);//----------------------------------------------
		ESP_LOGE(TAG,"user is admin admin");
		registered_users=1;
	}
	free(pass_list_buf);
	round=registered_users;
	show_users();
    fclose(fp);
}
static void save_userSet(void){
	struct stat st;
    if (stat(CREDENTIALS_PATH, &st))
    {
        ESP_LOGE(TAG, "pass file not found");
        return;
    }

    FILE *fp = fopen(CREDENTIALS_PATH, "wb");
	    // Ensure the file was opened successfully
    if (!fp){
        printf("Error opening password file.\n");
        return;
    }
	int round=0;
	show_users();
	size_t bytesW;
	ESP_LOGI(TAG,"Saving File");
	while (round<registered_users){
		bytesW=fwrite(pass_list[round],sizeof(unsigned char),PASS_LENGTH,fp);
		//fwrite(data, sizeof(unsigned char), sizeof(data) / sizeof(unsigned char), file);
		//bytesW=fwrite("\n", sizeof(char), 1, fp);
		round++;
		ESP_LOGI(TAG,"Saved %d bytes",bytesW);
    }
    fclose(fp);
	ESP_LOGI(TAG,"File saved");
	load_userSet();
	show_users();
}
void update_users(char user[],char pass[]){//PASS_LENGTH chars for user, PASS_LENGTH chars for password
	if (registered_users>=MAX_USERS){
		ESP_LOGE(TAG,"MAX USERS REACHED");
	}
	else{
		unsigned char pass_list_buf[PASS_LENGTH];
		cypher(http_auth_basic(pass,user),pass_list_buf);
		memcpy(pass_list[registered_users],pass_list_buf,PASS_LENGTH);//_----------------------------------------------
		//memcpy(pass_list[registered_users],pass,PASS_LENGTH);
		registered_users++;
		save_userSet();
	}
}
static void spiffs_stuff(void){
	    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
	initi_web_page_buffer();
	load_userSet();
	load_from_memory(add_user_html,ADD_USER_HTML_PATH);
}

esp_err_t get_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}

esp_err_t doorO_handler(httpd_req_t *req)
{
	basic_auth_info_t *basic_auth_info =req->user_ctx ;
	ESP_LOGI(TAG,"User %d Activated:",basic_auth_info->user_id);
	send_to_thingsboard(basic_auth_info->user_id);
	door_state=1;
    ESP_LOGI(TAG, "opening door\n");
    return send_web_page(req);
}

esp_err_t doorC_handler(httpd_req_t *req)
{
	door_state=0;
    ESP_LOGI(TAG, "closing door\n");
    return send_web_page(req);
}
esp_err_t add_user_page_load(httpd_req_t *req)
{
    return httpd_resp_send(req, add_user_html, HTTPD_RESP_USE_STRLEN);
}
esp_err_t add_user(httpd_req_t *req)
{
    char*  buf = malloc(req->content_len + 1);
	size_t off = 0;
	while (off < req->content_len) {
		/* Read data received in the request */
		int ret = httpd_req_recv(req, buf + off, req->content_len - off);
		if (ret <= 0) {
			if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
				httpd_resp_send_408(req);
			}
			free (buf);
			return ESP_FAIL;
		}
		off += ret;
		ESP_LOGI(TAG, "root_post_handler recv length %d", ret);
	}
	buf[off] = '\0';
	ESP_LOGI(TAG, "root_post_handler buf=[%s]", buf);
	cJSON *root = cJSON_Parse(buf);
	free(buf);
	cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "user");
    cJSON *user = cJSON_GetObjectItemCaseSensitive(root, "password");

	ESP_LOGI(TAG, "Name: %s", user->valuestring);
	ESP_LOGI(TAG, "password: %s", password->valuestring);
    // Check if the extracted values are valid and print them
    if (cJSON_IsString(user) && (user->valuestring != NULL)) {
        ESP_LOGI(TAG, "Name: %s", user->valuestring);
    }

    if (cJSON_IsString(password) && (password->valuestring != NULL)) {
        ESP_LOGI(TAG, "password: %s", password->valuestring);
    }
	update_users(user->valuestring,password->valuestring);
    return httpd_resp_send(req, add_user_html, HTTPD_RESP_USE_STRLEN);
}
httpd_uri_t basic_post = {
		.uri       = "/",
		.method    = HTTP_POST,
		.handler   = add_user,
	};


httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.uri_match_fn = httpd_uri_match_wildcard;
	//config.max_uri_handlers=5;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
		httpd_register_uri_handler(server, &basic_post);
		//httpd_register_basic_auth(server);
		httpd_register_basic_auth(server,"/doorO",doorO_handler);
		httpd_register_basic_auth(server,"/",get_req_handler);
		httpd_register_basic_auth(server,"/index.html",get_req_handler);
		httpd_register_basic_auth(server,"/doorC",doorC_handler);
		httpd_register_basic_auth(server,"/add_user.html",add_user_page_load);
        /*httpd_register_uri_handler(server, &uri_get,get_req_handler);
        httpd_register_uri_handler(server, &uri_open,doorO_handler);
        httpd_register_uri_handler(server, &uri_close,doorC_handler);*/
		
		
		
    }

    return server;
}
//add struct
void set_door_status(char * new_status){strcpy(door_status,new_status);}
void set_door_state(int new_state){door_state=new_state;}
int get_door_state(){return door_state;}
void webserver_init()
{
	//ESP_ERROR_CHECK(nvs_flash_erase());
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    connect_wifi();
   thingboard_init();
    if (wifi_connect_status)
    {
        door_state = 0;
        ESP_LOGI(TAG, "Door Control SPIFFS Web Server is running ... ...\n");
        spiffs_stuff();
        setup_server();
		//init_dns_server();
    }
}

