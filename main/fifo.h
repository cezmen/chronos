/* 
 * File:   fifo.h
 * Author: cezarmenezes
 *
 * Created on November 27, 2018, 7:38 PM
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

#ifndef FIFO_H
#define	FIFO_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *buffer ;
    unsigned int pi,po ;
    unsigned int size ;
} fifo_type ;

extern void fifo_config(fifo_type *F,unsigned char * buffer,unsigned int size) ;
extern unsigned int fifo_put(fifo_type *F,unsigned char c) ;
extern unsigned int fifo_get(fifo_type *F,unsigned char *c) ;
extern unsigned int fifo_length(fifo_type *F) ;
extern void fifo_clear(fifo_type *F) ;


#ifdef	__cplusplus
}
#endif

#endif	/* FIFO_H */

