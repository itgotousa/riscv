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
* File Name: vlc.c							
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
#include "vlc.h"

int alloc_table(VLC *vlc, int size)
{
    int index;
    index = vlc->table_size;
    vlc->table_size += size;
    if (vlc->table_size > vlc->table_allocated) 
	{
        vlc->table_allocated += (1 << vlc->bits);
        vlc->table = realloc(vlc->table,
			sizeof(VLC_TYPE) * 2 * vlc->table_allocated);
        if (!vlc->table)
            return -1;
    }
    return index;
}

#define GET_DATA(v, table, i, wrap, size) \
{\
    const uint8_t *ptr = (const uint8_t *)table + i * wrap;\
    switch(size) {\
    case 1:\
	v = *(const uint8_t *)ptr;\
	break;\
    case 2:\
	v = *(const uint16_t *)ptr;\
	break;\
    default:\
	v = *(const uint32_t *)ptr;\
	break;\
	}\
}

int build_table(VLC *vlc, int table_nb_bits,
                       int nb_codes,
                       const void *bits, int bits_wrap, int bits_size,
                       const void *codes, int codes_wrap, int codes_size,
                       uint32_t code_prefix, int n_prefix)
{
    int i, j, k, n, table_size, table_index, nb, n1, index;
    uint32_t code;
    VLC_TYPE (*table)[2];
	
    table_size = 1 << table_nb_bits;
    table_index = alloc_table(vlc, table_size);
    if (table_index < 0)
        return -1;
    table = &vlc->table[table_index];
	
    for(i=0;i<table_size;i++) {
        table[i][1] = 0; //bits
        table[i][0] = -1; //codes
    }
	
    /* first pass: map codes and compute auxillary table sizes */
    for(i=0;i<nb_codes;i++) {
        GET_DATA(n, bits, i, bits_wrap, bits_size);
        GET_DATA(code, codes, i, codes_wrap, codes_size);
        /* we accept tables with holes */
        if (n <= 0)
            continue;
        /* if code matches the prefix, it is in the table */
        n -= n_prefix;
        if (n > 0 && (code >> n) == code_prefix) {
            if (n <= table_nb_bits) {
                /* no need to add another table */
                j = (code << (table_nb_bits - n)) & (table_size - 1);
                nb = 1 << (table_nb_bits - n);
                for(k=0;k<nb;k++) {
                    if (table[j][1] /*bits*/ != 0) 
					{
						printf("Panic\n");
						return 0;
                    }
                    table[j][1] = n; //bits
                    table[j][0] = i; //code
                    j++;
                }
            } else {
                n -= table_nb_bits;
                j = (code >> n) & ((1 << table_nb_bits) - 1);
                /* compute table size */
                n1 = -table[j][1]; //bits
                if (n > n1)
                    n1 = n;
                table[j][1] = -n1; //bits
            }
        }
    }
	
    /* second pass : fill auxillary tables recursively */
    for(i=0;i<table_size;i++) 
	{
        n = table[i][1]; //bits
        if (n < 0) {
            n = -n;
            if (n > table_nb_bits) {
                n = table_nb_bits;
                table[i][1] = -n; //bits
            }
            index = build_table(vlc, n, nb_codes,
				bits, bits_wrap, bits_size,
				codes, codes_wrap, codes_size,
				(code_prefix << table_nb_bits) | i,
				n_prefix + table_nb_bits);
            if (index < 0)
                return -1;
            /* note: realloc has been done, so reload tables */
            table = &vlc->table[table_index];
            table[i][0] = index; //code
        }
    }
    return table_index;
}

int init_vlc(VLC *vlc, int nb_bits, int nb_codes,
             const void *bits, int bits_wrap, int bits_size,
             const void *codes, int codes_wrap, int codes_size)
{
    vlc->bits = nb_bits;
    vlc->table = NULL;
    vlc->table_allocated = 0;
    vlc->table_size = 0;
	
    if (build_table(vlc, nb_bits, nb_codes,
		bits, bits_wrap, bits_size,
		codes, codes_wrap, codes_size,
		0, 0) < 0) 
	{
        free(vlc->table);
        return -1;
    }
    return 0;
}

#define GET_VLC(code, name, gb, table, bits, max_depth)\
{\
    int n, index, nb_bits;\
	\
    index= SHOW_UBITS(name, gb, bits);\
    code = table[index][0];\
    n    = table[index][1];\
	\
    if(max_depth > 1 && n < 0){\
	LAST_SKIP_BITS(name, gb, bits)\
	UPDATE_CACHE(name, gb)\
	\
	nb_bits = -n;\
	\
	index= SHOW_UBITS(name, gb, nb_bits) + code;\
	code = table[index][0];\
	n    = table[index][1];\
	if(max_depth > 2 && n < 0){\
	LAST_SKIP_BITS(name, gb, nb_bits)\
	UPDATE_CACHE(name, gb)\
	\
	nb_bits = -n;\
	\
	index= SHOW_UBITS(name, gb, nb_bits) + code;\
	code = table[index][0];\
	n    = table[index][1];\
	}\
    }\
    SKIP_BITS(name, gb, n)\
}

int get_vlc(GetBitContext *s, VLC *vlc)
{
    int code;
    VLC_TYPE (*table)[2]= vlc->table;
    
    OPEN_READER(re, s)
	UPDATE_CACHE(re, s)
		
	GET_VLC(code, re, s, table, vlc->bits, 3)    
		
	CLOSE_READER(re, s)
	return code;
}

void free_vlc(VLC *vlc)
{
	if(vlc->table)
		free(vlc->table);
}

