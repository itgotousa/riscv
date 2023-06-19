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
* File Name: typedef.h							
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
#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

typedef signed char  int8_t;
typedef signed short int16_t;
typedef signed int   int32_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

typedef signed __int64   int64_t;
typedef unsigned __int64 uint64_t;

#define SWAP32(val) (uint32_t)((((uint32_t)(val)) & 0x000000FF)<<24|	\
					(((uint32_t)(val)) & 0x0000FF00)<<8 |	\
					(((uint32_t)(val)) & 0x00FF0000)>>8 |	\
					(((uint32_t)(val)) & 0xFF000000)>>24)	

#endif

