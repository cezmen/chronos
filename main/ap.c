/*
    ap.c - Access Point
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "ftm.h"


/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define PARAM_WIFI_SSID           CONFIG_ESP_WIFI_SSID
#define PARAM_WIFI_PASS           CONFIG_ESP_WIFI_PASSWORD
#define PARAM_WIFI_CHANNEL        CONFIG_ESP_WIFI_CHANNEL
#define PARAM_MAX_STA_CONN        CONFIG_ESP_MAX_STA_CONN
#define PARAM_INTERFACE_IP        CONFIG_ESP_INTERFACE_IP
#define PARAM_INTERFACE_GW        CONFIG_ESP_INTERFACE_GW
#define PARAM_INTERFACE_NETMASK   CONFIG_ESP_INTERFACE_NETMASK

static const char *TAG = "wifi softAP";


/* FUNCTION PROTOTYPES */
static void ap_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) ;
static void ap_create_and_setup_interface(void) ;
void ap_init(void) ;

//
// Access Point Event Handler
//
static void ap_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data ;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid) ;
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data ;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid) ;

    }     
    else 
    {
        // FTM EVENT HANDLER
        ftm_event_handler(arg, event_base, event_id, event_data) ; 
    }
}

//
// Create and Setup IP addresses and network mask
//
static void ap_create_and_setup_interface(void)
{
    esp_netif_t* wifiAP = esp_netif_create_default_wifi_ap() ;

    esp_netif_ip_info_t ipInfo;

    esp_netif_str_to_ip4(PARAM_INTERFACE_IP, &ipInfo.ip) ;                 // IP4_ADDR(&ipInfo.ip, 192,168,4,1);
    esp_netif_str_to_ip4(PARAM_INTERFACE_GW, &ipInfo.gw) ;                 // IP4_ADDR(&ipInfo.gw, 192,168,4,1);
    esp_netif_str_to_ip4(PARAM_INTERFACE_NETMASK, &ipInfo.netmask) ;       // IP4_ADDR(&ipInfo.netmask, 255,255,255,0);

	esp_netif_dhcps_stop(wifiAP);
	esp_netif_set_ip_info(wifiAP, &ipInfo);
	esp_netif_dhcps_start(wifiAP);
}

//
// Access Point Initialization
//
void ap_init(void)
{
    // Initialize the underlying TCP/IP stack.
    ESP_ERROR_CHECK(esp_netif_init()) ;
    // Cretate the Default Event Loop (a special type of loop used for system events)
    ESP_ERROR_CHECK(esp_event_loop_create_default()) ;
    // Setup IP addresses and netmask
    ap_create_and_setup_interface() ;

    // Initialize WiFi, allocate resource for WiFi driver, start WiFi task
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT() ;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)) ;

    // Initialize FTM module
    ftm_init() ;

    // Event Handler registration
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &ap_event_handler,
                                                        NULL,
                                                        NULL)) ;

    // Prepare configuration data for ESP32 AP or STA.
    wifi_config_t wifi_config = 
    {
        .ap = {
            .ssid = PARAM_WIFI_SSID,
            .ssid_len = strlen(PARAM_WIFI_SSID),
            .channel = PARAM_WIFI_CHANNEL,
            .password = PARAM_WIFI_PASS,
            .max_connection = PARAM_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .ftm_responder = true
        },
    };

    if (strlen(PARAM_WIFI_PASS) == 0) 
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN ;
    }


    // Set the WiFi operating mode ( the Chronos Utility works in "WiFi station + soft-AP mode" )
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA)) ;  
    // Set the bandwidth of ESP32 specified interface
    // If the device is used in some special environment, e.g. there are too many other Wi-Fi devices around the ESP32 device, 
    // the performance of HT40 may be degraded. So if the applications need to support same or similar scenarios, it’s recommended 
    // that the bandwidth is always configured to HT20.
    esp_wifi_set_bandwidth(ESP_IF_WIFI_STA, WIFI_BW_HT20) ;
    esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_BW_HT20) ;
    // Set the configuration of the ESP32 STA or AP
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config)) ;
    // Start WiFi according to current configuration
    ESP_ERROR_CHECK(esp_wifi_start()) ;

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             PARAM_WIFI_SSID, PARAM_WIFI_PASS, PARAM_WIFI_CHANNEL) ;
}
