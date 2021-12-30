/*
    command.c - Process Commands
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

#define COMMAND_TCP_ECHO_ENABLED    0

extern void json_test(void) ;

#include <string.h>
#include "server.h"
#include "tool.h"
#include "ftm.h"
#include "parser.h"

#define COMMAND_BUFFER_LENGTH   4096


static struct {
    unsigned int index  ;    
    unsigned char buffer[COMMAND_BUFFER_LENGTH] ;
} command_control = { .index = 0 } ;

void command_processing(void) ;
static void command_parsing(void) ;
void command_init(void) ;

//
// Process incoming data frame ( commands )
//
void command_processing(void) 
{
    unsigned char c ;

    while ( (command_control.index < COMMAND_BUFFER_LENGTH-1) && server_get_byte(&c) ) 
    {
        switch(c)
        {
            case 0x0A : // ignore LF ( line feed )
            case 0x0D : // ignore CR ( carriage return )
                        break ;

            case ';'  : // semicolon denotes the completion of a command                  
                        // PARSE FULL COMMAND
                        command_parsing() ;
                        // RESET INDEX
                        command_control.index = 0 ;
                        break ;

            default : 
                        // store a new character (c) in the buffer
                        command_control.buffer[command_control.index++] = c ;
                        break ;
        }
    }
}

//
// Parse and execute individual commands
//
static void command_parsing(void)
{
    char ssid[128] ;
    unsigned char mac[6] ;
    unsigned int  channel ;
    unsigned int  count ;
    unsigned int  burst_period ;
            
    // INSERT A NULL TERMINATION
    command_control.buffer[command_control.index++] = 0 ;   

    //
    // PARSE REMOTE COMMANDS
    //
    if (parser_ftm_by_ssid(command_control.buffer, ssid, &count, &burst_period))                // FTM COMMAND (BY SSID)
    {
        ftm_query_by_ssid(ssid, count, burst_period, server_put_bytes) ;
    }
    else if (parser_ftm_by_mac(command_control.buffer, mac, &channel, &count, &burst_period))   // FTM COMMAND (BY MAC)
    {
        ftm_query_by_mac(mac, channel, count, burst_period, server_put_bytes) ;            
    }
    else if (parser_scan(command_control.buffer, ssid))                                         // SCAN COMMAND
    {
        if (!strcmp(ssid,"?"))
        {
            // SCAN ALL SSIDs
            tool_perform_scan(0, false, server_put_bytes) ;                                     
        }
        else
        {
            // SCAN SPECIFIC SSID
            tool_perform_scan(ssid, false, server_put_bytes) ;                                  
        }
    }
 

    // COPY INPUT BYTES TO OUTPUT
    #if (COMMAND_TCP_ECHO_ENABLED)
        for(int k=0; k<command_control.index; k++)
        {
            server_put_byte(command_control.buffer[k]) ;    
        }
    #endif
}

//
// Initialize command instance
//
void command_init(void) 
{
    command_control.index = 0 ;
}