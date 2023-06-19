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
* File Name: bits.h							
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
#ifndef __BITS_H__
#define __BITS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "typedef.h"

#define bswap_16(x) (((x) & 0x00ff) << 8 | ((x) & 0xff00) >> 8)
	
#define be2me_16(x) bswap_16(x)

#   define MIN_CACHE_BITS 25

int unaligned32_be(const void *v);
		
#   define OPEN_READER(name, gb)\
	int name##_index= (gb)->index;\
	int name##_cache= 0;\
	
#   define CLOSE_READER(name, gb)\
	(gb)->index= name##_index;\
	
#   define UPDATE_CACHE(name, gb)\
	name##_cache= unaligned32_be( ((uint8_t *)(gb)->buffer)+(name##_index>>3) ) << (name##_index&0x07);\
	
#   define SKIP_CACHE(name, gb, num)\
	name##_cache <<= (num);\
	
	// FIXME name?
#   define SKIP_COUNTER(name, gb, num)\
	name##_index += (num);\
	
	//  av_log(NULL, 0, "SKIP_BITS-1\n"); 
#define SKIP_BITS(name, gb, num)\
	{\
		SKIP_CACHE(name, gb, num)\
		SKIP_COUNTER(name, gb, num)\
	}\
	
#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#define LAST_SKIP_CACHE(name, gb, num) ;

#define NEG_SSR32(a,s) ((( int32_t)(a))>>(32-(s)))
#define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))
	
#define SHOW_UBITS(name, gb, num)\
	NEG_USR32(name##_cache, num)
	
#define SHOW_SBITS(name, gb, num)\
	NEG_SSR32(name##_cache, num)
	
#define GET_CACHE(name, gb)\
	((uint32_t)name##_cache)
	

	/* buffer, buffer_end and size_in_bits must be present and used by every reader */
typedef struct GetBitContext 
{
	const uint8_t *buffer, *buffer_end;
	int index;
	int size_in_bits;
} GetBitContext;
	

void init_get_bits(GetBitContext *s, const uint8_t *buffer, int bit_size);
unsigned int get_bits(GetBitContext *s, int n);
void skip_bits(GetBitContext *s, int n);
int get_bits_count(GetBitContext *s);
unsigned int get_bits1(GetBitContext *s);


void *av_mallocz(unsigned int size);
void *av_malloc(unsigned int size);
void *av_mallocz_static(unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
