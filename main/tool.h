/*
    tool.h - WiFi Tools
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

#ifndef _TOOL_H  

#define _TOOL_H	1

    #ifdef __cplusplus 
    extern "C" {
    #endif

        #include "freertos/FreeRTOS.h"      // { bool } 
        #include "esp_wifi.h"               // { wifi_ap_record_t } 

        extern bool tool_perform_scan(const char *ssid, bool internal, void (*callback)(unsigned char *buffer, unsigned int len)) ;
        extern wifi_ap_record_t *tool_find_ftm_responder_ap(const char *ssid) ;
        extern unsigned int tool_mac_string_to_array(char *str,unsigned char *array) ;
        extern unsigned int tool_array_to_mac_string(char *str,unsigned char *array) ;    
        extern void tool_log(const char *tag, char *line, unsigned int type, void (*callback)(unsigned char *buffer, unsigned int len)) ;    

    #ifdef __cplusplus
    }
    #endif

#endif


