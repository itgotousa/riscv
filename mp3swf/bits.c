/*
* This source code is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*       
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* File Name: bits.c							
*
* Reference:
*
* Author: Li Feng,  fli_linux@yahoo.com.cn                                                 
*
* Description:
*
* 	
* 
* History:
* 02/23/2005  Li Feng    Created
*  
*
*CodeReview Log:
* 
*/
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "bits.h"

int unaligned32_be(const void *v)
{
	return SWAP32( (*(uint32_t*)(v))); //original
}

void init_get_bits(GetBitContext *s,
								 const uint8_t *buffer, int bit_size)
{
    const int buffer_size= (bit_size+7)>>3;
	
    s->buffer= buffer;
    s->size_in_bits= bit_size;
    s->buffer_end= buffer + buffer_size;
    s->index=0;
    {
        OPEN_READER(re, s)
		UPDATE_CACHE(re, s)
		UPDATE_CACHE(re, s)
		CLOSE_READER(re, s)
    }
}

void skip_bits(GetBitContext *s, int n)
{
	//Note gcc seems to optimize this to s->index+=n for the ALT_READER :))
    OPEN_READER(re, s)
	UPDATE_CACHE(re, s)
	LAST_SKIP_BITS(re, s, n)
	CLOSE_READER(re, s)
}


int get_bits_count(GetBitContext *s)
{
	return s->index;
}

unsigned int get_bits(GetBitContext *s, int n){
    register int tmp;
    OPEN_READER(re, s)
	UPDATE_CACHE(re, s)
	tmp= SHOW_UBITS(re, s, n);
    LAST_SKIP_BITS(re, s, n)
	CLOSE_READER(re, s)
	return tmp;
}

unsigned int get_bits1(GetBitContext *s)
{
    int index= s->index;
    uint8_t result= s->buffer[ index>>3 ];
    result<<= (index&0x07);
    result>>= 8 - 1;
    index++;
    s->index= index;
    return result;
}


void av_free(void *ptr)
{
    /* XXX: this test should not be needed on most libcs */
    if (ptr)
        free(ptr);
}


void *av_mallocz(unsigned int size)
{
    void *ptr;
    
    ptr = av_malloc(size);
    if (!ptr)
        return NULL;
    memset(ptr, 0, size);
    return ptr;
}

void *av_malloc(unsigned int size)
{
    void *ptr;
    ptr = malloc(size);
    return ptr;
}


void *av_fast_realloc(void *ptr, unsigned int *size, unsigned int min_size)
{
    if(min_size < *size) 
        return ptr;
    
    *size= 17*min_size/16 + 32;
	
    return realloc(ptr, *size);
}

static unsigned int last_static = 0;
static unsigned int allocated_static = 0;
static void** array_static = NULL;

void *av_mallocz_static(unsigned int size)
{
    void *ptr = av_mallocz(size);
	
    if(ptr)
	{ 
        array_static =av_fast_realloc(array_static, &allocated_static, sizeof(void*)*(last_static+1));
        array_static[last_static++] = ptr;
    }
	
    return ptr;
}

/**
 * free all static arrays and reset pointers to 0.
 */
/*void av_free_static(void)
{
    while(last_static){
        av_freep(&array_static[--last_static]);
    }
    av_freep(&array_static);
}*/

