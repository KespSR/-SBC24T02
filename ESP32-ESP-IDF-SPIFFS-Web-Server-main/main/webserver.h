#ifndef WEBSERVER_H
#define WEBSERVER_H

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
#include "esp_tls_crypto.h"

typedef esp_err_t (*page_handler_t)(httpd_req_t *req);
typedef struct {
    char    *username;
    char    *password;
	page_handler_t authorized_func;
} basic_auth_info_t;
// Funciones de servidor web
esp_err_t send_web_page(httpd_req_t *req);
esp_err_t get_req_handler(httpd_req_t *req);
esp_err_t doorO_handler(httpd_req_t *req);
esp_err_t doorC_handler(httpd_req_t *req);
esp_err_t add_user_page_load(httpd_req_t *req);
esp_err_t add_user(httpd_req_t *req);

// Funciones de autenticación
static char *http_auth_basic(const char *username, const char *password);
esp_err_t basic_auth_get_handler(httpd_req_t *req);
void httpd_register_basic_auth(httpd_handle_t server, char* uri_, page_handler_t authed_func);
// Funciones de manipulación de usuarios
void update_users(char user[], char pass[]);

// Funciones de almacenamiento en SPIFFS
static void spiffs_stuff(void);
static void load_from_memory(char* buffer, char* path);
static void load_userSet(void);
static void save_userSet(void);

// Funciones de configuración del servidor
httpd_handle_t setup_server(void);

// Funciones de gestión de la puerta
void set_door_status(char * new_status);
void set_door_state(int new_state);
int get_door_state();

// Funciones de inicialización
void webserver_init(void);

// Funciones auxiliares
void show_user(unsigned char user[]);
void show_users(void);

#endif // WEBSERVER_H
