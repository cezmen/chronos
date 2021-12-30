/*
    ftm.c - Fine Time Measurement (FTM)
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

#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "tool.h"
#include "server.h"

#define FTM_LINE_BUFFER_LENGTH       1024
#define FTM_LOG_BUFFER_LENGTH        400

static const char *TAG = "ftm" ;

static uint32_t g_rtt_est, g_dist_est ;
wifi_ftm_report_entry_t *g_ftm_report ;
uint8_t g_ftm_report_num_entries ;
static EventGroupHandle_t ftm_event_group ;
const int FTM_REPORT_BIT  = BIT0 ;
const int FTM_FAILURE_BIT = BIT1 ;

static void (*ftm_callback)(unsigned char *buffer, unsigned int len) ;

const int g_report_lvl =
        #ifdef CONFIG_ESP_FTM_REPORT_SHOW_DIAG
            BIT0 |
        #endif
        #ifdef CONFIG_ESP_FTM_REPORT_SHOW_RTT
            BIT1 |
        #endif
        #ifdef CONFIG_ESP_FTM_REPORT_SHOW_T1T2T3T4
            BIT2 |
        #endif
        #ifdef CONFIG_ESP_FTM_REPORT_SHOW_RSSI
            BIT3 |
        #endif
        0 ;


// FUNCTION PROTOTYPES
void ftm_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) ;
void ftm_process_report(void) ;     

int  ftm_query_by_ssid(const char *ssid, unsigned int count, unsigned int burst_period,
                       void (*callback)(unsigned char *buffer, unsigned int len)) ;

int  ftm_query_by_mac(unsigned char *mac, unsigned int channel,
                      unsigned int count, unsigned int burst_period,
                      void (*callback)(unsigned char *buffer, unsigned int len)) ;

void ftm_init(void) ;

//
// FTM Event Handler
//
void ftm_event_handler(void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_FTM_REPORT) 
    {
        wifi_event_ftm_report_t *event = (wifi_event_ftm_report_t *) event_data ;

        if (event->status == FTM_STATUS_SUCCESS) 
        {
            g_rtt_est = event->rtt_est ;                                // Estimated Round-Trip-Time with peer in Nano-Seconds
            g_dist_est = event->dist_est ;                              // Estimated one-way distance in Centi-Meters
            g_ftm_report = event->ftm_report_data ;                     // Pointer to FTM Report with multiple entries, should be freed after use
            g_ftm_report_num_entries = event->ftm_report_num_entries ;  // Number of entries in the FTM Report data
            xEventGroupSetBits(ftm_event_group, FTM_REPORT_BIT) ;
        } 
        else 
        {
            ESP_LOGI(TAG, "FTM procedure with Peer("MACSTR") failed! (Status - %d)",
                     MAC2STR(event->peer_mac), event->status) ;
            xEventGroupSetBits(ftm_event_group, FTM_FAILURE_BIT) ;
        }        
    }
}    


//
// Process a successful FTM report
//
void ftm_process_report(void)
{
    int i;
    char *log = malloc(FTM_LOG_BUFFER_LENGTH) ;

    if (!g_report_lvl)
        return ;

    if (!log) 
    {
        ESP_LOGE(TAG, "Failed to alloc buffer for FTM report") ;
        return ;
    }

    bzero(log, FTM_LOG_BUFFER_LENGTH) ;

    // [ FTM REPORT TITLE ]
    sprintf(log, "FTM Report:") ;    
    tool_log(TAG, log, 0, ftm_callback) ;

    // [ FTM REPORT HEADER ]
    sprintf(log, "|%s%s%s%s", 
                 g_report_lvl & BIT0 ? " Diag |":"", 
                 g_report_lvl & BIT1 ? "   RTT   |":"",
                 g_report_lvl & BIT2 ? "       T1       |       T2       |       T3       |       T4       |":"",
                 g_report_lvl & BIT3 ? "  RSSI  |":"") ;
    tool_log(TAG, log, 0, ftm_callback) ;

    // [ FTM REPORT ROWS ]
    for (i = 0; i < g_ftm_report_num_entries; i++) 
    {
        char *log_ptr = log ;

        bzero(log, FTM_LOG_BUFFER_LENGTH) ;

        log_ptr += sprintf(log_ptr,"|") ;

        if (g_report_lvl & BIT0) 
        {
            log_ptr += sprintf(log_ptr, "%6d|", g_ftm_report[i].dlog_token) ;   // Dialog Token
        }
        if (g_report_lvl & BIT1) 
        {
            log_ptr += sprintf(log_ptr, "%7u  |", g_ftm_report[i].rtt) ;        // RTT
        }
        if (g_report_lvl & BIT2) 
        {
            log_ptr += sprintf(log_ptr, "%14llu  |%14llu  |%14llu  |%14llu  |", g_ftm_report[i].t1,
                                        g_ftm_report[i].t2, g_ftm_report[i].t3, g_ftm_report[i].t4) ;
        }
        if (g_report_lvl & BIT3) 
        {
            log_ptr += sprintf(log_ptr, "%6d  |", g_ftm_report[i].rssi) ;
        }

        tool_log(TAG, log, 0, ftm_callback) ;

    }
    free(log) ;
}

//
// Execute a FTM query by SSID
//
int ftm_query_by_ssid(const char *ssid, unsigned int count, unsigned int burst_period,
                      void (*callback)(unsigned char *buffer, unsigned int len))
{
    wifi_ap_record_t *ap_record ;
   
    ap_record = tool_find_ftm_responder_ap(ssid) ;

    if (ap_record) 
    {
        return ftm_query_by_mac(ap_record->bssid, ap_record->primary, 
                                count, burst_period, 
                                callback) ;        
    } 
    else 
    {
        return 0 ;
    }
}

//
// Execute a FTM query by MAC address ( and channel )
//
int ftm_query_by_mac(unsigned char *mac, unsigned int channel,
                     unsigned int count, unsigned int burst_period,
                     void (*callback)(unsigned char *buffer, unsigned int len))
{
    EventBits_t bits ;
    const TickType_t xMaxTicksToWait = 30000 / portTICK_PERIOD_MS ;      // 30 seconds of maximum waiting before Timoeout

    wifi_ftm_initiator_cfg_t ftmi_cfg = {
        .frm_count = 32,
        .burst_period = 2,
    } ;

    char line[FTM_LINE_BUFFER_LENGTH] ;
   
    ftm_callback = callback ;

    // MAC ADDRESS
    memcpy(ftmi_cfg.resp_mac, mac, 6) ;
    ftmi_cfg.channel = channel ;

    // COUNT
    if ( count != 0 && count != 8 && count != 16 &&
         count != 24 && count != 32 && count != 64 )
    {
        sprintf(line,"Invalid Frame Count! Valid options are 0/8/16/24/32/64") ;
        tool_log(TAG, line, 1, ftm_callback) ;        
        return 0 ;
    }
    else
    {
        ftmi_cfg.frm_count = count ;
    }

    // BURST PERIOD
    if ( (burst_period >= 2) && (burst_period < 256) )
    {
        ftmi_cfg.burst_period = burst_period ;
    } 
    else 
    {
        sprintf(line,"Invalid Burst Period! Valid range is 2-255") ;
        tool_log(TAG, line, 1, ftm_callback) ;        
        return 0 ;
    }

    // START FTM QUERY 
    sprintf(line,"Requesting FTM session with Frm Count - %d, Burst Period - %dmSec (0: No Preference)",
                 ftmi_cfg.frm_count, ftmi_cfg.burst_period*100) ;
    tool_log(TAG, line, 0, ftm_callback) ;                 


    if (ESP_OK != esp_wifi_ftm_initiate_session(&ftmi_cfg)) 
    {
        sprintf(line,"Failed to start FTM session") ;
        tool_log(TAG, line, 1, ftm_callback) ;        
        return 0 ;
    }

    bits = xEventGroupWaitBits(ftm_event_group, FTM_REPORT_BIT | FTM_FAILURE_BIT,
                               pdFALSE, pdFALSE, xMaxTicksToWait) ;

    /* Processing data from FTM session */
    if (bits & FTM_REPORT_BIT) 
    {
        ftm_process_report() ;
        free(g_ftm_report) ;
        g_ftm_report = NULL ;
        g_ftm_report_num_entries = 0 ;
        sprintf(line,"Estimated RTT - %d nSec, Estimated Distance - %d.%02d meters",
                    g_rtt_est, g_dist_est / 100, g_dist_est % 100) ;
        tool_log(TAG, line, 0, ftm_callback) ;                    

        xEventGroupClearBits(ftm_event_group, FTM_REPORT_BIT) ;
        return 1 ;
    } 
    else 
    {
        /* Failure case */
        if (bits & FTM_FAILURE_BIT)
        {
            sprintf(line,"FTM Failure") ;
        }
        else
        {
            sprintf(line,"FTM Timeout") ;            
        }    
        tool_log(TAG, line, 0, ftm_callback) ;        
    }

    return 0 ;
}


//
// FTM Initialization
//
void ftm_init(void)
{
    ftm_event_group = xEventGroupCreate() ; 
    ftm_callback = 0 ;
}

