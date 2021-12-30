/* 
 * fifo.c
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

static const char *TAG = "fifo";

typedef struct {
    unsigned char *buffer ;
    unsigned int pi,po ;
    unsigned int size ;
} fifo_type ;

void fifo_config(fifo_type *F,unsigned char * buffer,unsigned int size) ;
unsigned int fifo_put(fifo_type *F,unsigned char c) ;
unsigned int fifo_get(fifo_type *F,unsigned char *c) ;
unsigned int fifo_length(fifo_type *F) ;
void fifo_clear(fifo_type *F) ;


void fifo_config(fifo_type *F,unsigned char * buffer,unsigned int size)
{
    F->buffer = buffer ;
    F->size = size ;
    F->pi = 0 ;
    F->po = 0 ;
}

unsigned int fifo_put(fifo_type *F,unsigned char c)
{
    unsigned int ret = 0 ;
    unsigned int next ;
    

    next = (F->pi+1) % F->size ;
    if ( next != F->po)
    {
        F->buffer[F->pi] = c ;
        F->pi = next ;
        ret = 1 ;
    }

    return ret ;
}

unsigned int fifo_get(fifo_type *F,unsigned char *c)
{
    unsigned int ret = 0 ;
    unsigned int next ;

    if ( F->po != F->pi)
    {
        *c = F->buffer[F->po] ;
        next = (F->po+1) % F->size ;
        F->po = next ;
        ret = 1 ;
    }

    return ret ;    
}

unsigned int fifo_length(fifo_type *F)
{
    unsigned int ret = 0 ;

    if (F->pi >= F->po)
        ret = (F->pi - F->po) ;
    else
        ret =  (F->size + F->pi - F->po) ;

    return ret ;
}
 
void fifo_clear(fifo_type *F)
{
    F->pi = 0 ;
    F->po = 0 ;
}

