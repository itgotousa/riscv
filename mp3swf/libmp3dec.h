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
* File Name: libmp3dec.h							
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
#ifndef __LIBMP3DEC_H__
#define __LIBMP3DEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "typedef.h"

#define HMP3DEC void*

HMP3DEC	MP3_decode_init();
uint32_t MP3_GetFrameSize(uint32_t head);
int		MP3_GetAudioInfo(uint32_t* pnSampleRate, uint32_t *nChannels, uint32_t head);
int		MP3_decode_frame(HMP3DEC hDec, void *data, int *data_size, uint8_t *buf, int buf_size);
void	MP3_decode_close(HMP3DEC hDec);
				 
#ifdef __cplusplus
}
#endif
	

#endif


