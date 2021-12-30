/*
    parser.c - Command Parser
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
#include "esp_log.h"
#include "cJSON.h"
#include "tool.h"

static const char *TAG = "parser";

// FUNCTION PROTOTYPES
unsigned int parser_ftm_by_ssid(unsigned char *string, char *ssid, unsigned int *count, unsigned int *burst_period) ;
unsigned int parser_ftm_by_mac(unsigned char *string, unsigned char *mac, unsigned int *channel, unsigned int *count, unsigned int *burst_period) ;
unsigned int parser_scan(unsigned char *string, char *ssid) ;

//
// Parse and detect "FTM by SSID" command
//
unsigned int parser_ftm_by_ssid(unsigned char *string, char *ssid, unsigned int *count, unsigned int *burst_period)
{
    unsigned int ret = 0 ;
	cJSON *root , *parameters ;

    if (string) 
    {
        root = cJSON_Parse((const char *) string);        

        if (cJSON_GetObjectItem(root, "function")) 
        {
            char *function = cJSON_GetObjectItem(root,"function")->valuestring ;

            if (!strcmp(function,"ftm"))
            {
                if ( (parameters = cJSON_GetObjectItem(root, "parameters"))  ) 
                {
                    if (ssid && cJSON_GetObjectItem(parameters, "ssid")) 
                    {
                        char *s ;
                        if ( (s = cJSON_GetObjectItem(parameters,"ssid")->valuestring) )
                        {
                            strncpy(ssid,s,127) ;                            
                            ret = 1 ;
                            ESP_LOGI(TAG, "ftm function (by ssid)");            
                        }
                    }

                    if (count)
                    {
                        if (cJSON_GetObjectItem(parameters, "count")) 
                        {
                            *count = cJSON_GetObjectItem(parameters,"count")->valueint;
                        }
                        else
                        {
                            *count = 8 ;
                        }
                    }

                    if (burst_period)
                    {
                        if (cJSON_GetObjectItem(parameters, "burst"))
                        {
                            *burst_period = cJSON_GetObjectItem(parameters,"burst")->valueint;
                        }
                        else
                        {
                            *burst_period = 4 ;
                        }
                    }
                }
            }
        }
	    cJSON_Delete(root);
    }
    return ret ;
}

//
// Parse and detect "FTM by MAC address" command
//
unsigned int parser_ftm_by_mac(unsigned char *string, unsigned char *mac, unsigned int *channel, unsigned int *count, unsigned int *burst_period)
{
    unsigned int ret = 0 ;
	cJSON *root , *parameters ;

    if (string) 
    {
        root = cJSON_Parse((const char *) string);        

        if (cJSON_GetObjectItem(root, "function")) 
        {
            char *function = cJSON_GetObjectItem(root,"function")->valuestring ;

            if (!strcmp(function,"ftm"))
            {
                if ( (parameters = cJSON_GetObjectItem(root, "parameters"))  ) 
                {
                    if ( (mac && cJSON_GetObjectItem(parameters, "mac")) && (channel && cJSON_GetObjectItem(parameters, "channel")) ) 
                    {
                        char *s ;
                        s = cJSON_GetObjectItem(parameters,"mac")->valuestring ;
                        if (tool_mac_string_to_array(s, mac))
                        {
                            *channel = cJSON_GetObjectItem(parameters,"channel")->valueint ;
                            ret = 1 ;
                            ESP_LOGI(TAG, "ftm function (by mac)") ;
                        }
                    }

                    if (count)
                    {
                        if (cJSON_GetObjectItem(parameters, "count")) 
                        {
                            *count = cJSON_GetObjectItem(parameters,"count")->valueint;
                        }
                        else
                        {
                            *count = 8 ;
                        }
                    }

                    if (burst_period)
                    {
                        if (cJSON_GetObjectItem(parameters, "burst"))
                        {
                            *burst_period = cJSON_GetObjectItem(parameters,"burst")->valueint;
                        }
                        else
                        {
                            *burst_period = 4 ;
                        }
                    }
                }
            }
        }
	    cJSON_Delete(root);
    }
    return ret ;
}


//
// Parse and detect "WiFi scanning" command
//
unsigned int parser_scan(unsigned char *string, char *ssid)
{
    unsigned int ret = 0 ;
	cJSON *root , *parameters ;

    if (string) 
    {
        root = cJSON_Parse((const char *) string);        

        if (cJSON_GetObjectItem(root, "function")) 
        {
            char *function = cJSON_GetObjectItem(root,"function")->valuestring ;

            if (!strcmp(function,"scan"))
            {
                if (ssid)
                {
                    strcpy(ssid,"?") ;
                    if ( (parameters = cJSON_GetObjectItem(root, "parameters"))  ) 
                    {
                        if (cJSON_GetObjectItem(parameters, "ssid"))
                        {
                            char *s ;
                            if ( (s = cJSON_GetObjectItem(parameters,"ssid")->valuestring) )
                            {
                                strncpy(ssid,s,127) ;
                            }
                        }
                    }
                }
                ret = 1 ;
                ESP_LOGI(TAG, "scan function") ;
            }
        }
	    cJSON_Delete(root);        
    }
    return ret ;
}
