/*
    ftm.h - Fine Time Measurement (FTM)
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


#ifndef _FTM_H  

#define _FTM_H	1

    #ifdef __cplusplus 
    extern "C" {
    #endif

        extern void ftm_event_handler(void *arg, esp_event_base_t event_base,
                                      int32_t event_id, void *event_data) ;
        extern void ftm_process_report(void) ;
        extern int  ftm_query_by_ssid(const char *ssid, unsigned int count, unsigned int burst_period,
                                      void (*callback)(unsigned char *buffer, unsigned int len)) ;

        extern int  ftm_query_by_mac(unsigned char *mac, unsigned int channel,
                                     unsigned int count, unsigned int burst_period,
                                     void (*callback)(unsigned char *buffer, unsigned int len)) ;                              
        extern void ftm_init(void) ;

    #ifdef __cplusplus
    }
    #endif

#endif

