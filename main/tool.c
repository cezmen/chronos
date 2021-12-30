/*
    tool.c - WiFi Tools 
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

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"

#define TOOL_LINE_BUFFER_LENGTH       1024

static uint16_t g_scan_ap_num;
static wifi_ap_record_t *g_ap_list_buffer;

static const char *TAG = "tool";

// FUNCTION PROTOTYPES
bool tool_perform_scan(const char *ssid, bool internal, void (*callback)(unsigned char *buffer, unsigned int len)) ;
wifi_ap_record_t *tool_find_ftm_responder_ap(const char *ssid) ;
unsigned int tool_mac_string_to_array(char *str,unsigned char *array) ;
unsigned int tool_array_to_mac_string(char *str,unsigned char *array) ;
void tool_log(const char *tag, char *line, unsigned int type, void (*callback)(unsigned char *buffer, unsigned int len)) ;

//
// Perform WiFi Scanning 
//
// ssid == 0 : scan all available AP stations
// ssid != 0 : scan specific SSID string (pointed by <ssid>)
//
bool tool_perform_scan(const char *ssid, bool internal, void (*callback)(unsigned char *buffer, unsigned int len))
{
    wifi_scan_config_t scan_config = { 0 } ;
    scan_config.ssid = (uint8_t *) ssid ;
    uint8_t i;
    char line[TOOL_LINE_BUFFER_LENGTH] ;    

    ESP_ERROR_CHECK( esp_wifi_scan_start(&scan_config, true) ) ;

    esp_wifi_scan_get_ap_num(&g_scan_ap_num) ;

    if (g_scan_ap_num == 0) 
    {
        sprintf(line, "No matching AP found") ;
        tool_log(TAG, line, 0, callback) ;        
        return false ;
    }

    if (g_ap_list_buffer) 
    {
        free(g_ap_list_buffer) ;
    }

    g_ap_list_buffer = malloc(g_scan_ap_num * sizeof(wifi_ap_record_t)) ;

    if (g_ap_list_buffer == NULL) 
    {
        sprintf(line, "Failed to malloc buffer to print scan results") ;
        tool_log(TAG, line, 1, callback) ;            
        return false ;
    }

    // [ SCAN REPORT TITLE ]
    sprintf(line, "Scan Report:") ;
    tool_log(TAG, line, 0, callback) ;    

    // [ SCAN REPORT ROWS ]
    if (esp_wifi_scan_get_ap_records(&g_scan_ap_num, (wifi_ap_record_t *)g_ap_list_buffer) == ESP_OK) 
    {
        if (!internal) 
        {
            for (i = 0; i < g_scan_ap_num; i++) 
            {
                char mac_string[32] ;

                tool_array_to_mac_string(mac_string, g_ap_list_buffer[i].bssid)  ;

                sprintf(line, "[%s][rssi %d][ch %d][mac %s]%s", 
                                g_ap_list_buffer[i].ssid, 
                                g_ap_list_buffer[i].rssi, 
                                g_ap_list_buffer[i].primary,                                 
                                mac_string,
                                g_ap_list_buffer[i].ftm_responder ? "[FTM]" : "") ;
                tool_log(TAG, line, 0, callback) ;                                    
            }
        }
    }

    sprintf(line,"%s","sta scan done") ;
    tool_log(TAG, line, 0, callback) ;        


    return true ;
}

//
// Return the Description (wifi_ap_record_t) of a WiFi AP
// ( which contains bssid[], ssid[], primary channel, secondary channel), etc )
//
wifi_ap_record_t *tool_find_ftm_responder_ap(const char *ssid)
{
    bool retry_scan = false ;
    uint8_t i ;

    if (!ssid)
        return NULL ;

retry:
    if (!g_ap_list_buffer || (g_scan_ap_num == 0)) 
    {
        ESP_LOGI(TAG, "Scanning for %s", ssid) ;
        if (false == tool_perform_scan(ssid, true, 0)) 
        {
            return NULL ;
        }
    }

    for (i = 0; i < g_scan_ap_num; i++) 
    {
        if (strcmp((const char *)g_ap_list_buffer[i].ssid, ssid) == 0)
            return &g_ap_list_buffer[i] ;
    }

    if (!retry_scan) 
    {
        retry_scan = true ;
        if (g_ap_list_buffer) 
        {
            free(g_ap_list_buffer) ;
            g_ap_list_buffer = NULL ;
        }
        goto retry ;
    }

    ESP_LOGI(TAG, "No matching AP found") ;

    return NULL ;
}

//
// Convert MAC string to byte array (6 bytes)
//

unsigned int tool_mac_string_to_array(char *str,unsigned char *array) 
{
    unsigned int m , k ;
    unsigned int a[6] ;

    if (str && array)
    {
        m = sscanf(str,"%02x:%02x:%02x:%02x:%02x:%02x",
                        &a[0],&a[1],&a[2],&a[3],&a[4],&a[5]) ;
        if (m == 6)
        {
            for(k=0; k<6; k++)
            {
                array[k] = (unsigned char) a[k] ;                
            }            
            return 1 ;
        }                    
    }        
    return 0 ;
}

//
// Convert byte array (6 bytes) to MAC string (19 bytes)
//
unsigned int tool_array_to_mac_string(char *str,unsigned char *array) 
{
    if (str && array)
    {
        sprintf(str,"%02x:%02x:%02x:%02x:%02x:%02x",
                    array[0],array[1],array[2],array[3],array[4],array[5]) ;
        return 1 ;
    }                    
    return 0 ;
}

//
// Print Log Messages to both Console and TCP/IP socket (through callback function)
//
// type==0 : Information Log , type==1 : Error Log
//
void tool_log(const char *tag, char *line, unsigned int type, void (*callback)(unsigned char *buffer, unsigned int len))
{
    // Conventional ESP LOG
    if (tag && line)
    {
        switch(type)
        {
            case 0 : ESP_LOGI(tag,"%s",line) ;      // Information logging
                    break ;
            case 1 : ESP_LOGE(tag,"%s",line) ;      // Error logging
                    break ;
        }
    }    

    // TCP/IP socket CALLBACK
    if (callback)
    {
        strcat(line,"\n") ;
        callback((unsigned char *)line,strlen(line)) ;
    }    
}
