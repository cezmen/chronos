/*
    server.c - TCP/IP Server
*/

/*
 * Created on Thu Dec 30 2021
 *
 * Copyright (c) 2021 Cezar Menezes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Contact: cezar.menezes@live.com
 *
 */

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "lwip/sockets.h" 
#include <lwip/netdb.h>                 

#include "fifo.h"
#include "command.h"


#define PARAM_PORT                        CONFIG_ESP_PORT
#define PARAM_KEEPALIVE_IDLE              CONFIG_ESP_KEEPALIVE_IDLE
#define PARAM_KEEPALIVE_INTERVAL          CONFIG_ESP_KEEPALIVE_INTERVAL
#define PARAM_KEEPALIVE_COUNT             CONFIG_ESP_KEEPALIVE_COUNT 

#define SERVER_RX_BUFFER_LENGTH           1024
#define SERVER_TX_BUFFER_LENGTH           1024

#define FIFO_BUFFER_SIZE                  16384

// FUNCTION PROTOTYPES
static void  server_process_data(char * data, int len) ;
unsigned int server_get_byte(unsigned char *c) ;
unsigned int server_put_byte(unsigned char c) ;
void server_put_bytes(unsigned char *buffer, unsigned int len) ;
static void  server_transmit_tcp_data(const int sock, char * data, int len) ;
static void  server_receive_tcp_data(const int sock,void (*callback)(char * data, int len)) ;
static void  server_input_task(void *pvParameters) ;
static void  server_output_task(void *pvParameters) ;
void server_init(void) ;

static const char *TAG = "tcp server";

static int server_socket = -1 ; 

static fifo_type FIFO[2] ;  // incoming [index 0] and outgoing [index 1] FIFOs 
static unsigned char FIFO_BUFFER[2][FIFO_BUFFER_SIZE] ; 

static SemaphoreHandle_t server_mutex ;

//
// Process incoming TCP/IP data frame
//
// Incoming TCP/IP bytes (of a data frame) are injected in the incoming FIFO
//
static void server_process_data(char * data, int len) 
{
    unsigned int k ;

    if (len > 0)
    {
        data[len] = 0 ; // Null-terminate whatever is received and treat it like a string
        ESP_LOGI(TAG, "Received %d bytes: %s", len, data) ;

        for (k=0;k<len;k++)
        {
            fifo_put(&FIFO[0],(unsigned char) data[k]) ;
        }

        // PROCESS COMMANDS
        xSemaphoreTake(server_mutex,portMAX_DELAY) ;  
        command_processing() ;  
        xSemaphoreGive(server_mutex) ;                
    }
}

// 
// Get a single byte from the "incoming FIFO"
//
// Extract a single byte from the Incoming FIFO
//
unsigned int server_get_byte(unsigned char *c)
{
    unsigned int ret = 0 ;

    ret =  fifo_get(&FIFO[0],c) ;

    return ret ;
}

// 
// Put a single byte in the "outgoing FIFO"
//
unsigned int server_put_byte(unsigned char c)
{
    unsigned int ret = 0 ;

    ret = fifo_put(&FIFO[1],c) ;

    return ret ;    
}

// 
// Put a frame (multiple bytes) in the "outgoing FIFO"
//
void server_put_bytes(unsigned char *buffer, unsigned int len)
{
    unsigned int k ;

    for (k=0; k<len; k++) server_put_byte(buffer[k]) ;

}

// 
// TCP/IP transmission of a data frame
//
static void server_transmit_tcp_data(const int sock, char * data, int len)
{
    // send() can return less bytes than supplied length.
    // Walk-around for robust implementation.
    int to_write = len ;

    while (to_write > 0) 
    {
        int written = send(sock, data + (len - to_write), to_write, 0) ;
        if (written < 0) 
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno) ;
        }
        to_write -= written ;
    }
}

// 
// TCP/IP reception of a frame
//
// note: callback (if specified) is called for data frame processing
//
static void server_receive_tcp_data(const int sock,void (*callback)(char * data, int len)) 
{
    int len ;
    char rx_buffer[SERVER_RX_BUFFER_LENGTH] ;

    server_socket = sock ;

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0) ;
        if (len < 0) 
        {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno) ;
        } 
        else if (len == 0) 
        {
            ESP_LOGW(TAG, "Connection closed") ;
        } 
        else 
        {
            // PROCESS RECEIVED DATA
            callback(rx_buffer, len) ;
        }
    } while (len > 0) ;

    server_socket = -1 ;
}

//
// TCP/IP incoming data task
//
static void server_input_task(void *pvParameters)
{
    char addr_str[128] ;
    int  addr_family = (int)pvParameters ;
    int  ip_protocol = 0 ;
    int  keepAlive = 1 ;
    int  keepIdle = PARAM_KEEPALIVE_IDLE ;
    int  keepInterval = PARAM_KEEPALIVE_INTERVAL ;
    int  keepCount = PARAM_KEEPALIVE_COUNT ;
    struct sockaddr_storage dest_addr ;

    if (addr_family == AF_INET) 
    {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr ;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY) ;
        dest_addr_ip4->sin_family = AF_INET ;
        dest_addr_ip4->sin_port = htons(PARAM_PORT) ;
        ip_protocol = IPPROTO_IP ;
    }
#ifdef CONFIG_ESP_IPV6
    else if (addr_family == AF_INET6) 
    {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr ;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un)) ;
        dest_addr_ip6->sin6_family = AF_INET6 ;
        dest_addr_ip6->sin6_port = htons(PARAM_PORT) ;
        ip_protocol = IPPROTO_IPV6 ;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol) ;
    if (listen_sock < 0) 
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno) ;
        vTaskDelete(NULL) ;
        return ;
    }
    int opt = 1 ;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ;
#if defined(CONFIG_ESP_IPV4) && defined(CONFIG_ESP_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) ;
#endif

    ESP_LOGI(TAG, "Socket created") ;

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) ;
    if (err != 0) 
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno) ;
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family) ;
        goto CLEAN_UP ;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PARAM_PORT) ;

    err = listen(listen_sock, 1) ;
    if (err != 0) 
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno) ;
        goto CLEAN_UP ;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening") ;

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr) ;
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len) ;
        if (sock < 0) 
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno) ;
            break ;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int)) ;
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int)) ;
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int)) ;
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int)) ;
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) 
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1) ;
        }
#ifdef CONFIG_ESP_IPV6
        else if (source_addr.ss_family == PF_INET6) 
        {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1) ;
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str) ;

        // Inicialize Command Instance
        command_init() ;

        // Receive and Process TCP/IP incoming bytes
        server_receive_tcp_data(sock, server_process_data) ;

        // Terminate a TCP/IP socket connection
        shutdown(sock, 0) ;
        close(sock) ;
    }

CLEAN_UP:
    close(listen_sock) ;
    vTaskDelete(NULL) ;
}

//
// TCP/IP outgoing data task
//
static void server_output_task(void *pvParameters)
{
    unsigned int k, m, len, blocks, remainder ;
    char tx_buffer[SERVER_TX_BUFFER_LENGTH] ;    

    while(1)
    {
        xSemaphoreTake(server_mutex,portMAX_DELAY) ;
        len = fifo_length(&FIFO[1]) ;
        xSemaphoreGive(server_mutex) ;

        // FLUSH OUTPUT FIFO ( TCP/IP TRANSMISSION )
        if ( (len > 0) && (server_socket > 0) ) 
        {
            xSemaphoreTake(server_mutex,portMAX_DELAY) ;

            blocks = len / SERVER_TX_BUFFER_LENGTH ;    // number of full blocks
            remainder = len % SERVER_TX_BUFFER_LENGTH ; // size of remainder (final) block

            // TRANSMIT FULL BLOCKS
            for (k=0; k<blocks; k++)
            {
                for (m=0; m<SERVER_TX_BUFFER_LENGTH; m++)
                {
                    fifo_get(&FIFO[1], (unsigned char *) &tx_buffer[m]) ;
                }    
                server_transmit_tcp_data(server_socket, tx_buffer, SERVER_TX_BUFFER_LENGTH) ;
            }

            // TRANSMIT REMAINDER (FINAL) BLOCK
            if (remainder)
            {
                for (m=0; m<remainder; m++)
                {
                    fifo_get(&FIFO[1], (unsigned char *) &tx_buffer[m]) ;
                }    
                server_transmit_tcp_data(server_socket, tx_buffer, remainder) ;
            }

            xSemaphoreGive(server_mutex) ;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS) ;      
    }

    vTaskDelete(NULL) ;
}

//
// Server Initialization
//
void server_init(void)
{
    server_mutex = xSemaphoreCreateMutex() ;

    // CREATE FIFOS
    fifo_config( &FIFO[0], FIFO_BUFFER[0], FIFO_BUFFER_SIZE ) ; // INCOMING FIFO
    fifo_config( &FIFO[1], FIFO_BUFFER[1], FIFO_BUFFER_SIZE ) ; // OUTGOING FIFO

    // CREATE TCP/IP OUTPUT TASK
    xTaskCreate(server_output_task, "tcp_output", 8192, (void*) 0, 10, NULL) ;

    // CREATE TCP/IP INPUT TASK
    #ifdef CONFIG_ESP_IPV4
        xTaskCreate(server_input_task, "tcp_input", 8192, (void*)AF_INET, 5, NULL) ; 
    #endif

    #ifdef CONFIG_ESP_IPV6
        xTaskCreate(server_input_task, "tcp_input", 8192, (void*)AF_INET6, 5, NULL) ;
    #endif

}
