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
* File Name: mp3dec.h							
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
#ifndef __MP3DEC_H__
#define __MP3DEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bits.h"
	/* max frame size, in samples */
#define MPA_FRAME_SIZE 1152 
	
	/* max compressed frame size */
#define MPA_MAX_CODED_FRAME_SIZE 1792
	
#define MPA_MAX_CHANNELS 2
	
#define SBLIMIT 32 /* number of subbands */
	
#define MPA_STEREO  0
#define MPA_JSTEREO 1
#define MPA_DUAL    2
#define MPA_MONO    3

#define HEADER_SIZE 4
#define BACKSTEP_SIZE 512

#define FRAC_BITS   15   /* fractional bits for sb_samples and dct */
#define WFRAC_BITS  14   /* fractional bits for window */
	
#define FRAC_ONE    (1 << FRAC_BITS)

#define M_PI    3.14159265358979323846

#define int64_t_C(c)     (c ## L)
	
#define MULL(a,b) (((int64_t)(a) * (int64_t)(b)) >> FRAC_BITS)
#define MUL64(a,b) ((int64_t)(a) * (int64_t)(b))
#define FIX(a)   ((int)((a) * FRAC_ONE))
	/* WARNING: only correct for posititive numbers */
#define FIXR(a)   ((int)((a) * FRAC_ONE + 0.5))
#define FRAC_RND(a) (((a) + (FRAC_ONE/2)) >> FRAC_BITS)
	
typedef int32_t MPA_INT;

typedef struct tagAVCodecContext
{
	int frame_size;
	void *priv_data;
	int antialias_algo;
	int channels;
	int bit_rate;
	int sub_id;
	int sample_rate;
	int parse_only;
}AVCodecContext;


typedef struct MPADecodeContext {
    uint8_t inbuf1[2][MPA_MAX_CODED_FRAME_SIZE + BACKSTEP_SIZE];	/* input buffer */
    int inbuf_index;
    uint8_t *inbuf_ptr, *inbuf;
    int frame_size;
    int free_format_frame_size; /* frame size in case of free format
                                   (zero if currently unknown) */
    /* next header (used in free format parsing) */
    uint32_t free_format_next_header; 
    int error_protection;
    int layer;
    int sample_rate;
    int sample_rate_index; /* between 0 and 8 */
    int bit_rate;
    int old_frame_size;
    GetBitContext gb;
    int nb_channels;
    int mode;
    int mode_ext;
    int lsf;
    MPA_INT synth_buf[MPA_MAX_CHANNELS][512 * 2];
    int synth_buf_offset[MPA_MAX_CHANNELS];
    int32_t sb_samples[MPA_MAX_CHANNELS][36][SBLIMIT];
    int32_t mdct_buf[MPA_MAX_CHANNELS][SBLIMIT * 18]; /* previous samples, for layer 3 MDCT */
    void (*compute_antialias)(struct MPADecodeContext *s, struct GranuleDef *g);
} MPADecodeContext;	

/* layer 3 "granule" */
typedef struct GranuleDef {
    uint8_t scfsi;
    int part2_3_length;
    int big_values;
    int global_gain;
    int scalefac_compress;
    uint8_t block_type;
    uint8_t switch_point;
    int table_select[3];
    int subblock_gain[3];
    uint8_t scalefac_scale;
    uint8_t count1table_select;
    int region_size[3]; /* number of huffman codes in each region */
    int preflag;
    int short_start, long_end; /* long/short band indexes */
    uint8_t scale_factors[40];
    int32_t sb_hybrid[SBLIMIT * 18]; /* 576 samples */
} GranuleDef;


#define MODE_EXT_MS_STEREO 2
#define MODE_EXT_I_STEREO  1

/* layer 3 huffman tables */
typedef struct HuffTable 
{
    int xsize;
    const uint8_t *bits;
    const uint16_t *codes;
} HuffTable;

#ifdef __cplusplus
}
#endif


#endif

