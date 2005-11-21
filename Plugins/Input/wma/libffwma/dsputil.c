/*
 * DSP utils
 * Copyright (c) 2000, 2001 Fabrice Bellard.
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This library is free software; you can redistribute it and/or
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
 * gmc & q-pel & 32/64 bit based MC by Michael Niedermayer <michaelni@gmx.at>
 */
 
/**
 * @file dsputil.c
 * DSP utils
 */
 
#include "avcodec.h"
#include "dsputil.h"
#include "simple_idct.h"

uint8_t cropTbl[256 + 2 * MAX_NEG_CROP];
uint32_t squareTbl[512];

const uint8_t ff_zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/* Specific zigzag scan for 248 idct. NOTE that unlike the
   specification, we interleave the fields */
const uint8_t ff_zigzag248_direct[64] = {
     0,  8,  1,  9, 16, 24,  2, 10,
    17, 25, 32, 40, 48, 56, 33, 41,
    18, 26,  3, 11,  4, 12, 19, 27,
    34, 42, 49, 57, 50, 58, 35, 43,
    20, 28,  5, 13,  6, 14, 21, 29,
    36, 44, 51, 59, 52, 60, 37, 45,
    22, 30,  7, 15, 23, 31, 38, 46,
    53, 61, 54, 62, 39, 47, 55, 63,
};

/* not permutated inverse zigzag_direct + 1 for MMX quantizer */
uint16_t __align8 inv_zigzag_direct16[64];

const uint8_t ff_alternate_horizontal_scan[64] = {
    0,  1,   2,  3,  8,  9, 16, 17, 
    10, 11,  4,  5,  6,  7, 15, 14,
    13, 12, 19, 18, 24, 25, 32, 33, 
    26, 27, 20, 21, 22, 23, 28, 29,
    30, 31, 34, 35, 40, 41, 48, 49, 
    42, 43, 36, 37, 38, 39, 44, 45,
    46, 47, 50, 51, 56, 57, 58, 59, 
    52, 53, 54, 55, 60, 61, 62, 63,
};

const uint8_t ff_alternate_vertical_scan[64] = {
    0,  8,  16, 24,  1,  9,  2, 10, 
    17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12, 
    19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14, 
    21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31, 
    38, 46, 54, 62, 39, 47, 55, 63,
};

/* a*inverse[b]>>32 == a/b for all 0<=a<=65536 && 2<=b<=255 */
const uint32_t inverse[256]={
         0, 4294967295U,2147483648U,1431655766, 1073741824,  858993460,  715827883,  613566757, 
 536870912,  477218589,  429496730,  390451573,  357913942,  330382100,  306783379,  286331154, 
 268435456,  252645136,  238609295,  226050911,  214748365,  204522253,  195225787,  186737709, 
 178956971,  171798692,  165191050,  159072863,  153391690,  148102321,  143165577,  138547333, 
 134217728,  130150525,  126322568,  122713352,  119304648,  116080198,  113025456,  110127367, 
 107374183,  104755300,  102261127,   99882961,   97612894,   95443718,   93368855,   91382283, 
  89478486,   87652394,   85899346,   84215046,   82595525,   81037119,   79536432,   78090315, 
  76695845,   75350304,   74051161,   72796056,   71582789,   70409300,   69273667,   68174085, 
  67108864,   66076420,   65075263,   64103990,   63161284,   62245903,   61356676,   60492498, 
  59652324,   58835169,   58040099,   57266231,   56512728,   55778797,   55063684,   54366675, 
  53687092,   53024288,   52377650,   51746594,   51130564,   50529028,   49941481,   49367441, 
  48806447,   48258060,   47721859,   47197443,   46684428,   46182445,   45691142,   45210183, 
  44739243,   44278014,   43826197,   43383509,   42949673,   42524429,   42107523,   41698712, 
  41297763,   40904451,   40518560,   40139882,   39768216,   39403370,   39045158,   38693400, 
  38347923,   38008561,   37675152,   37347542,   37025581,   36709123,   36398028,   36092163, 
  35791395,   35495598,   35204650,   34918434,   34636834,   34359739,   34087043,   33818641, 
  33554432,   33294321,   33038210,   32786010,   32537632,   32292988,   32051995,   31814573, 
  31580642,   31350127,   31122952,   30899046,   30678338,   30460761,   30246249,   30034737, 
  29826162,   29620465,   29417585,   29217465,   29020050,   28825284,   28633116,   28443493, 
  28256364,   28071682,   27889399,   27709467,   27531842,   27356480,   27183338,   27012373, 
  26843546,   26676816,   26512144,   26349493,   26188825,   26030105,   25873297,   25718368, 
  25565282,   25414008,   25264514,   25116768,   24970741,   24826401,   24683721,   24542671, 
  24403224,   24265352,   24129030,   23994231,   23860930,   23729102,   23598722,   23469767, 
  23342214,   23216040,   23091223,   22967740,   22845571,   22724695,   22605092,   22486740, 
  22369622,   22253717,   22139007,   22025474,   21913099,   21801865,   21691755,   21582751, 
  21474837,   21367997,   21262215,   21157475,   21053762,   20951060,   20849356,   20748635, 
  20648882,   20550083,   20452226,   20355296,   20259280,   20164166,   20069941,   19976593, 
  19884108,   19792477,   19701685,   19611723,   19522579,   19434242,   19346700,   19259944, 
  19173962,   19088744,   19004281,   18920561,   18837576,   18755316,   18673771,   18592933, 
  18512791,   18433337,   18354562,   18276457,   18199014,   18122225,   18046082,   17970575, 
  17895698,   17821442,   17747799,   17674763,   17602325,   17530479,   17459217,   17388532, 
  17318417,   17248865,   17179870,   17111424,   17043522,   16976156,   16909321,   16843010,
};

/* Input permutation for the simple_idct_mmx */
static const uint8_t simple_mmx_permutation[64]={
	0x00, 0x08, 0x04, 0x09, 0x01, 0x0C, 0x05, 0x0D, 
	0x10, 0x18, 0x14, 0x19, 0x11, 0x1C, 0x15, 0x1D, 
	0x20, 0x28, 0x24, 0x29, 0x21, 0x2C, 0x25, 0x2D, 
	0x12, 0x1A, 0x16, 0x1B, 0x13, 0x1E, 0x17, 0x1F, 
	0x02, 0x0A, 0x06, 0x0B, 0x03, 0x0E, 0x07, 0x0F, 
	0x30, 0x38, 0x34, 0x39, 0x31, 0x3C, 0x35, 0x3D, 
	0x22, 0x2A, 0x26, 0x2B, 0x23, 0x2E, 0x27, 0x2F, 
	0x32, 0x3A, 0x36, 0x3B, 0x33, 0x3E, 0x37, 0x3F,
};
#if 0
static int pix_sum_c(uint8_t * pix, int line_size)
{
    int s, i, j;

    s = 0;
    for (i = 0; i < 16; i++) {
	for (j = 0; j < 16; j += 8) {
	    s += pix[0];
	    s += pix[1];
	    s += pix[2];
	    s += pix[3];
	    s += pix[4];
	    s += pix[5];
	    s += pix[6];
	    s += pix[7];
	    pix += 8;
	}
	pix += line_size - 16;
    }
    return s;
}

static int pix_norm1_c(uint8_t * pix, int line_size)
{
    int s, i, j;
    uint32_t *sq = squareTbl + 256;

    s = 0;
    for (i = 0; i < 16; i++) {
	for (j = 0; j < 16; j += 8) {
#if 0
	    s += sq[pix[0]];
	    s += sq[pix[1]];
	    s += sq[pix[2]];
	    s += sq[pix[3]];
	    s += sq[pix[4]];
	    s += sq[pix[5]];
	    s += sq[pix[6]];
	    s += sq[pix[7]];
#else
#if LONG_MAX > 2147483647
	    register uint64_t x=*(uint64_t*)pix;
	    s += sq[x&0xff];
	    s += sq[(x>>8)&0xff];
	    s += sq[(x>>16)&0xff];
	    s += sq[(x>>24)&0xff];
            s += sq[(x>>32)&0xff];
            s += sq[(x>>40)&0xff];
            s += sq[(x>>48)&0xff];
            s += sq[(x>>56)&0xff];
#else
	    register uint32_t x=*(uint32_t*)pix;
	    s += sq[x&0xff];
	    s += sq[(x>>8)&0xff];
	    s += sq[(x>>16)&0xff];
	    s += sq[(x>>24)&0xff];
            x=*(uint32_t*)(pix+4);
            s += sq[x&0xff];
            s += sq[(x>>8)&0xff];
            s += sq[(x>>16)&0xff];
            s += sq[(x>>24)&0xff];
#endif
#endif
	    pix += 8;
	}
	pix += line_size - 16;
    }
    return s;
}

static void bswap_buf(uint32_t *dst, uint32_t *src, int w){
    int i;
    
    for(i=0; i+8<=w; i+=8){
        dst[i+0]= bswap_32(src[i+0]);
        dst[i+1]= bswap_32(src[i+1]);
        dst[i+2]= bswap_32(src[i+2]);
        dst[i+3]= bswap_32(src[i+3]);
        dst[i+4]= bswap_32(src[i+4]);
        dst[i+5]= bswap_32(src[i+5]);
        dst[i+6]= bswap_32(src[i+6]);
        dst[i+7]= bswap_32(src[i+7]);
    }
    for(;i<w; i++){
        dst[i+0]= bswap_32(src[i+0]);
    }
}

static int sse8_c(void *v, uint8_t * pix1, uint8_t * pix2, int line_size, int h)
{
    int s, i;
    uint32_t *sq = squareTbl + 256;

    s = 0;
    for (i = 0; i < h; i++) {
        s += sq[pix1[0] - pix2[0]];
        s += sq[pix1[1] - pix2[1]];
        s += sq[pix1[2] - pix2[2]];
        s += sq[pix1[3] - pix2[3]];
        s += sq[pix1[4] - pix2[4]];
        s += sq[pix1[5] - pix2[5]];
        s += sq[pix1[6] - pix2[6]];
        s += sq[pix1[7] - pix2[7]];
        pix1 += line_size;
        pix2 += line_size;
    }
    return s;
}

static int sse16_c(void *v, uint8_t *pix1, uint8_t *pix2, int line_size, int h)
{
    int s, i;
    uint32_t *sq = squareTbl + 256;

    s = 0;
    for (i = 0; i < h; i++) {
        s += sq[pix1[ 0] - pix2[ 0]];
        s += sq[pix1[ 1] - pix2[ 1]];
        s += sq[pix1[ 2] - pix2[ 2]];
        s += sq[pix1[ 3] - pix2[ 3]];
        s += sq[pix1[ 4] - pix2[ 4]];
        s += sq[pix1[ 5] - pix2[ 5]];
        s += sq[pix1[ 6] - pix2[ 6]];
        s += sq[pix1[ 7] - pix2[ 7]];
        s += sq[pix1[ 8] - pix2[ 8]];
        s += sq[pix1[ 9] - pix2[ 9]];
        s += sq[pix1[10] - pix2[10]];
        s += sq[pix1[11] - pix2[11]];
        s += sq[pix1[12] - pix2[12]];
        s += sq[pix1[13] - pix2[13]];
        s += sq[pix1[14] - pix2[14]];
        s += sq[pix1[15] - pix2[15]];

        pix1 += line_size;
        pix2 += line_size;
    }
    return s;
}

static void get_pixels_c(DCTELEM *restrict block, const uint8_t *pixels, int line_size)
{
    int i;

    /* read the pixels */
    for(i=0;i<8;i++) {
        block[0] = pixels[0];
        block[1] = pixels[1];
        block[2] = pixels[2];
        block[3] = pixels[3];
        block[4] = pixels[4];
        block[5] = pixels[5];
        block[6] = pixels[6];
        block[7] = pixels[7];
        pixels += line_size;
        block += 8;
    }
}

static void diff_pixels_c(DCTELEM *restrict block, const uint8_t *s1,
			  const uint8_t *s2, int stride){
    int i;

    /* read the pixels */
    for(i=0;i<8;i++) {
        block[0] = s1[0] - s2[0];
        block[1] = s1[1] - s2[1];
        block[2] = s1[2] - s2[2];
        block[3] = s1[3] - s2[3];
        block[4] = s1[4] - s2[4];
        block[5] = s1[5] - s2[5];
        block[6] = s1[6] - s2[6];
        block[7] = s1[7] - s2[7];
        s1 += stride;
        s2 += stride;
        block += 8;
    }
}


static void put_pixels_clamped_c(const DCTELEM *block, uint8_t *restrict pixels,
				 int line_size)
{
    int i;
    uint8_t *cm = cropTbl + MAX_NEG_CROP;
    
    /* read the pixels */
    for(i=0;i<8;i++) {
        pixels[0] = cm[block[0]];
        pixels[1] = cm[block[1]];
        pixels[2] = cm[block[2]];
        pixels[3] = cm[block[3]];
        pixels[4] = cm[block[4]];
        pixels[5] = cm[block[5]];
        pixels[6] = cm[block[6]];
        pixels[7] = cm[block[7]];

        pixels += line_size;
        block += 8;
    }
}

static void add_pixels_clamped_c(const DCTELEM *block, uint8_t *restrict pixels,
                          int line_size)
{
    int i;
    uint8_t *cm = cropTbl + MAX_NEG_CROP;
    
    /* read the pixels */
    for(i=0;i<8;i++) {
        pixels[0] = cm[pixels[0] + block[0]];
        pixels[1] = cm[pixels[1] + block[1]];
        pixels[2] = cm[pixels[2] + block[2]];
        pixels[3] = cm[pixels[3] + block[3]];
        pixels[4] = cm[pixels[4] + block[4]];
        pixels[5] = cm[pixels[5] + block[5]];
        pixels[6] = cm[pixels[6] + block[6]];
        pixels[7] = cm[pixels[7] + block[7]];
        pixels += line_size;
        block += 8;
    }
}
#endif
#if 0

#define PIXOP2(OPNAME, OP) \
static void OPNAME ## _pixels(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        OP(*((uint64_t*)block), LD64(pixels));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _no_rnd_pixels_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        const uint64_t a= LD64(pixels  );\
        const uint64_t b= LD64(pixels+1);\
        OP(*((uint64_t*)block), (a&b) + (((a^b)&0xFEFEFEFEFEFEFEFEULL)>>1));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _pixels_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        const uint64_t a= LD64(pixels  );\
        const uint64_t b= LD64(pixels+1);\
        OP(*((uint64_t*)block), (a|b) - (((a^b)&0xFEFEFEFEFEFEFEFEULL)>>1));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _no_rnd_pixels_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        const uint64_t a= LD64(pixels          );\
        const uint64_t b= LD64(pixels+line_size);\
        OP(*((uint64_t*)block), (a&b) + (((a^b)&0xFEFEFEFEFEFEFEFEULL)>>1));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _pixels_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int i;\
    for(i=0; i<h; i++){\
        const uint64_t a= LD64(pixels          );\
        const uint64_t b= LD64(pixels+line_size);\
        OP(*((uint64_t*)block), (a|b) - (((a^b)&0xFEFEFEFEFEFEFEFEULL)>>1));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
\
static void OPNAME ## _pixels_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
        int i;\
        const uint64_t a= LD64(pixels  );\
        const uint64_t b= LD64(pixels+1);\
        uint64_t l0=  (a&0x0303030303030303ULL)\
                    + (b&0x0303030303030303ULL)\
                    + 0x0202020202020202ULL;\
        uint64_t h0= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
                   + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
        uint64_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint64_t a= LD64(pixels  );\
            uint64_t b= LD64(pixels+1);\
            l1=  (a&0x0303030303030303ULL)\
               + (b&0x0303030303030303ULL);\
            h1= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
              + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
            OP(*((uint64_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0F0F0F0F0FULL));\
            pixels+=line_size;\
            block +=line_size;\
            a= LD64(pixels  );\
            b= LD64(pixels+1);\
            l0=  (a&0x0303030303030303ULL)\
               + (b&0x0303030303030303ULL)\
               + 0x0202020202020202ULL;\
            h0= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
              + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
            OP(*((uint64_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0F0F0F0F0FULL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
}\
\
static void OPNAME ## _no_rnd_pixels_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
        int i;\
        const uint64_t a= LD64(pixels  );\
        const uint64_t b= LD64(pixels+1);\
        uint64_t l0=  (a&0x0303030303030303ULL)\
                    + (b&0x0303030303030303ULL)\
                    + 0x0101010101010101ULL;\
        uint64_t h0= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
                   + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
        uint64_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint64_t a= LD64(pixels  );\
            uint64_t b= LD64(pixels+1);\
            l1=  (a&0x0303030303030303ULL)\
               + (b&0x0303030303030303ULL);\
            h1= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
              + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
            OP(*((uint64_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0F0F0F0F0FULL));\
            pixels+=line_size;\
            block +=line_size;\
            a= LD64(pixels  );\
            b= LD64(pixels+1);\
            l0=  (a&0x0303030303030303ULL)\
               + (b&0x0303030303030303ULL)\
               + 0x0101010101010101ULL;\
            h0= ((a&0xFCFCFCFCFCFCFCFCULL)>>2)\
              + ((b&0xFCFCFCFCFCFCFCFCULL)>>2);\
            OP(*((uint64_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0F0F0F0F0FULL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
}\
\
CALL_2X_PIXELS(OPNAME ## _pixels16_c    , OPNAME ## _pixels_c    , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_x2_c , OPNAME ## _pixels_x2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_y2_c , OPNAME ## _pixels_y2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_xy2_c, OPNAME ## _pixels_xy2_c, 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_x2_c , OPNAME ## _no_rnd_pixels_x2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_y2_c , OPNAME ## _no_rnd_pixels_y2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_xy2_c, OPNAME ## _no_rnd_pixels_xy2_c, 8)

#define op_avg(a, b) a = ( ((a)|(b)) - ((((a)^(b))&0xFEFEFEFEFEFEFEFEULL)>>1) )
#else // 64 bit variant

#define PIXOP2(OPNAME, OP) \
static void OPNAME ## _pixels2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    int i;\
    for(i=0; i<h; i++){\
        OP(*((uint16_t*)(block  )), LD16(pixels  ));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
static void OPNAME ## _pixels4_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    int i;\
    for(i=0; i<h; i++){\
        OP(*((uint32_t*)(block  )), LD32(pixels  ));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
static void OPNAME ## _pixels8_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    int i;\
    for(i=0; i<h; i++){\
        OP(*((uint32_t*)(block  )), LD32(pixels  ));\
        OP(*((uint32_t*)(block+4)), LD32(pixels+4));\
        pixels+=line_size;\
        block +=line_size;\
    }\
}\
static inline void OPNAME ## _no_rnd_pixels8_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels8_c(block, pixels, line_size, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a,b;\
        a= LD32(&src1[i*src_stride1  ]);\
        b= LD32(&src2[i*src_stride2  ]);\
        OP(*((uint32_t*)&dst[i*dst_stride  ]), no_rnd_avg32(a, b));\
        a= LD32(&src1[i*src_stride1+4]);\
        b= LD32(&src2[i*src_stride2+4]);\
        OP(*((uint32_t*)&dst[i*dst_stride+4]), no_rnd_avg32(a, b));\
    }\
}\
\
static inline void OPNAME ## _pixels8_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a,b;\
        a= LD32(&src1[i*src_stride1  ]);\
        b= LD32(&src2[i*src_stride2  ]);\
        OP(*((uint32_t*)&dst[i*dst_stride  ]), rnd_avg32(a, b));\
        a= LD32(&src1[i*src_stride1+4]);\
        b= LD32(&src2[i*src_stride2+4]);\
        OP(*((uint32_t*)&dst[i*dst_stride+4]), rnd_avg32(a, b));\
    }\
}\
\
static inline void OPNAME ## _pixels4_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a,b;\
        a= LD32(&src1[i*src_stride1  ]);\
        b= LD32(&src2[i*src_stride2  ]);\
        OP(*((uint32_t*)&dst[i*dst_stride  ]), rnd_avg32(a, b));\
    }\
}\
\
static inline void OPNAME ## _pixels2_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a,b;\
        a= LD16(&src1[i*src_stride1  ]);\
        b= LD16(&src2[i*src_stride2  ]);\
        OP(*((uint16_t*)&dst[i*dst_stride  ]), rnd_avg32(a, b));\
    }\
}\
\
static inline void OPNAME ## _pixels16_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    OPNAME ## _pixels8_l2(dst  , src1  , src2  , dst_stride, src_stride1, src_stride2, h);\
    OPNAME ## _pixels8_l2(dst+8, src1+8, src2+8, dst_stride, src_stride1, src_stride2, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels16_l2(uint8_t *dst, const uint8_t *src1, const uint8_t *src2, int dst_stride, \
                                                int src_stride1, int src_stride2, int h){\
    OPNAME ## _no_rnd_pixels8_l2(dst  , src1  , src2  , dst_stride, src_stride1, src_stride2, h);\
    OPNAME ## _no_rnd_pixels8_l2(dst+8, src1+8, src2+8, dst_stride, src_stride1, src_stride2, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _no_rnd_pixels8_l2(block, pixels, pixels+1, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels8_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels8_l2(block, pixels, pixels+1, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _no_rnd_pixels8_l2(block, pixels, pixels+line_size, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels8_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels8_l2(block, pixels, pixels+line_size, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels8_l4(uint8_t *dst, const uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4,\
                 int dst_stride, int src_stride1, int src_stride2,int src_stride3,int src_stride4, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a, b, c, d, l0, l1, h0, h1;\
        a= LD32(&src1[i*src_stride1]);\
        b= LD32(&src2[i*src_stride2]);\
        c= LD32(&src3[i*src_stride3]);\
        d= LD32(&src4[i*src_stride4]);\
        l0=  (a&0x03030303UL)\
           + (b&0x03030303UL)\
           + 0x02020202UL;\
        h0= ((a&0xFCFCFCFCUL)>>2)\
          + ((b&0xFCFCFCFCUL)>>2);\
        l1=  (c&0x03030303UL)\
           + (d&0x03030303UL);\
        h1= ((c&0xFCFCFCFCUL)>>2)\
          + ((d&0xFCFCFCFCUL)>>2);\
        OP(*((uint32_t*)&dst[i*dst_stride]), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
        a= LD32(&src1[i*src_stride1+4]);\
        b= LD32(&src2[i*src_stride2+4]);\
        c= LD32(&src3[i*src_stride3+4]);\
        d= LD32(&src4[i*src_stride4+4]);\
        l0=  (a&0x03030303UL)\
           + (b&0x03030303UL)\
           + 0x02020202UL;\
        h0= ((a&0xFCFCFCFCUL)>>2)\
          + ((b&0xFCFCFCFCUL)>>2);\
        l1=  (c&0x03030303UL)\
           + (d&0x03030303UL);\
        h1= ((c&0xFCFCFCFCUL)>>2)\
          + ((d&0xFCFCFCFCUL)>>2);\
        OP(*((uint32_t*)&dst[i*dst_stride+4]), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
    }\
}\
\
static inline void OPNAME ## _pixels4_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels4_l2(block, pixels, pixels+1, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels4_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels4_l2(block, pixels, pixels+line_size, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels2_x2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels2_l2(block, pixels, pixels+1, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _pixels2_y2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    OPNAME ## _pixels2_l2(block, pixels, pixels+line_size, line_size, line_size, line_size, h);\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_l4(uint8_t *dst, const uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4,\
                 int dst_stride, int src_stride1, int src_stride2,int src_stride3,int src_stride4, int h){\
    int i;\
    for(i=0; i<h; i++){\
        uint32_t a, b, c, d, l0, l1, h0, h1;\
        a= LD32(&src1[i*src_stride1]);\
        b= LD32(&src2[i*src_stride2]);\
        c= LD32(&src3[i*src_stride3]);\
        d= LD32(&src4[i*src_stride4]);\
        l0=  (a&0x03030303UL)\
           + (b&0x03030303UL)\
           + 0x01010101UL;\
        h0= ((a&0xFCFCFCFCUL)>>2)\
          + ((b&0xFCFCFCFCUL)>>2);\
        l1=  (c&0x03030303UL)\
           + (d&0x03030303UL);\
        h1= ((c&0xFCFCFCFCUL)>>2)\
          + ((d&0xFCFCFCFCUL)>>2);\
        OP(*((uint32_t*)&dst[i*dst_stride]), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
        a= LD32(&src1[i*src_stride1+4]);\
        b= LD32(&src2[i*src_stride2+4]);\
        c= LD32(&src3[i*src_stride3+4]);\
        d= LD32(&src4[i*src_stride4+4]);\
        l0=  (a&0x03030303UL)\
           + (b&0x03030303UL)\
           + 0x01010101UL;\
        h0= ((a&0xFCFCFCFCUL)>>2)\
          + ((b&0xFCFCFCFCUL)>>2);\
        l1=  (c&0x03030303UL)\
           + (d&0x03030303UL);\
        h1= ((c&0xFCFCFCFCUL)>>2)\
          + ((d&0xFCFCFCFCUL)>>2);\
        OP(*((uint32_t*)&dst[i*dst_stride+4]), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
    }\
}\
static inline void OPNAME ## _pixels16_l4(uint8_t *dst, const uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4,\
                 int dst_stride, int src_stride1, int src_stride2,int src_stride3,int src_stride4, int h){\
    OPNAME ## _pixels8_l4(dst  , src1  , src2  , src3  , src4  , dst_stride, src_stride1, src_stride2, src_stride3, src_stride4, h);\
    OPNAME ## _pixels8_l4(dst+8, src1+8, src2+8, src3+8, src4+8, dst_stride, src_stride1, src_stride2, src_stride3, src_stride4, h);\
}\
static inline void OPNAME ## _no_rnd_pixels16_l4(uint8_t *dst, const uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4,\
                 int dst_stride, int src_stride1, int src_stride2,int src_stride3,int src_stride4, int h){\
    OPNAME ## _no_rnd_pixels8_l4(dst  , src1  , src2  , src3  , src4  , dst_stride, src_stride1, src_stride2, src_stride3, src_stride4, h);\
    OPNAME ## _no_rnd_pixels8_l4(dst+8, src1+8, src2+8, src3+8, src4+8, dst_stride, src_stride1, src_stride2, src_stride3, src_stride4, h);\
}\
\
static inline void OPNAME ## _pixels2_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
        int i, a0, b0, a1, b1;\
        a0= pixels[0];\
        b0= pixels[1] + 2;\
        a0 += b0;\
        b0 += pixels[2];\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            a1= pixels[0];\
            b1= pixels[1];\
            a1 += b1;\
            b1 += pixels[2];\
\
            block[0]= (a1+a0)>>2; /* FIXME non put */\
            block[1]= (b1+b0)>>2;\
\
            pixels+=line_size;\
            block +=line_size;\
\
            a0= pixels[0];\
            b0= pixels[1] + 2;\
            a0 += b0;\
            b0 += pixels[2];\
\
            block[0]= (a1+a0)>>2;\
            block[1]= (b1+b0)>>2;\
            pixels+=line_size;\
            block +=line_size;\
        }\
}\
\
static inline void OPNAME ## _pixels4_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
        int i;\
        const uint32_t a= LD32(pixels  );\
        const uint32_t b= LD32(pixels+1);\
        uint32_t l0=  (a&0x03030303UL)\
                    + (b&0x03030303UL)\
                    + 0x02020202UL;\
        uint32_t h0= ((a&0xFCFCFCFCUL)>>2)\
                   + ((b&0xFCFCFCFCUL)>>2);\
        uint32_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint32_t a= LD32(pixels  );\
            uint32_t b= LD32(pixels+1);\
            l1=  (a&0x03030303UL)\
               + (b&0x03030303UL);\
            h1= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
            a= LD32(pixels  );\
            b= LD32(pixels+1);\
            l0=  (a&0x03030303UL)\
               + (b&0x03030303UL)\
               + 0x02020202UL;\
            h0= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
}\
\
static inline void OPNAME ## _pixels8_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int j;\
    for(j=0; j<2; j++){\
        int i;\
        const uint32_t a= LD32(pixels  );\
        const uint32_t b= LD32(pixels+1);\
        uint32_t l0=  (a&0x03030303UL)\
                    + (b&0x03030303UL)\
                    + 0x02020202UL;\
        uint32_t h0= ((a&0xFCFCFCFCUL)>>2)\
                   + ((b&0xFCFCFCFCUL)>>2);\
        uint32_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint32_t a= LD32(pixels  );\
            uint32_t b= LD32(pixels+1);\
            l1=  (a&0x03030303UL)\
               + (b&0x03030303UL);\
            h1= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
            a= LD32(pixels  );\
            b= LD32(pixels+1);\
            l0=  (a&0x03030303UL)\
               + (b&0x03030303UL)\
               + 0x02020202UL;\
            h0= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
        pixels+=4-line_size*(h+1);\
        block +=4-line_size*h;\
    }\
}\
\
static inline void OPNAME ## _no_rnd_pixels8_xy2_c(uint8_t *block, const uint8_t *pixels, int line_size, int h)\
{\
    int j;\
    for(j=0; j<2; j++){\
        int i;\
        const uint32_t a= LD32(pixels  );\
        const uint32_t b= LD32(pixels+1);\
        uint32_t l0=  (a&0x03030303UL)\
                    + (b&0x03030303UL)\
                    + 0x01010101UL;\
        uint32_t h0= ((a&0xFCFCFCFCUL)>>2)\
                   + ((b&0xFCFCFCFCUL)>>2);\
        uint32_t l1,h1;\
\
        pixels+=line_size;\
        for(i=0; i<h; i+=2){\
            uint32_t a= LD32(pixels  );\
            uint32_t b= LD32(pixels+1);\
            l1=  (a&0x03030303UL)\
               + (b&0x03030303UL);\
            h1= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
            a= LD32(pixels  );\
            b= LD32(pixels+1);\
            l0=  (a&0x03030303UL)\
               + (b&0x03030303UL)\
               + 0x01010101UL;\
            h0= ((a&0xFCFCFCFCUL)>>2)\
              + ((b&0xFCFCFCFCUL)>>2);\
            OP(*((uint32_t*)block), h0+h1+(((l0+l1)>>2)&0x0F0F0F0FUL));\
            pixels+=line_size;\
            block +=line_size;\
        }\
        pixels+=4-line_size*(h+1);\
        block +=4-line_size*h;\
    }\
}\
\
CALL_2X_PIXELS(OPNAME ## _pixels16_c  , OPNAME ## _pixels8_c  , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_x2_c , OPNAME ## _pixels8_x2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_y2_c , OPNAME ## _pixels8_y2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _pixels16_xy2_c, OPNAME ## _pixels8_xy2_c, 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_c  , OPNAME ## _pixels8_c         , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_x2_c , OPNAME ## _no_rnd_pixels8_x2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_y2_c , OPNAME ## _no_rnd_pixels8_y2_c , 8)\
CALL_2X_PIXELS(OPNAME ## _no_rnd_pixels16_xy2_c, OPNAME ## _no_rnd_pixels8_xy2_c, 8)\

#define op_avg(a, b) a = rnd_avg32(a, b)
#endif
#define op_put(a, b) a = b

//PIXOP2(avg, op_avg)
//PIXOP2(put, op_put)
#undef op_avg
#undef op_put

#define avg2(a,b) ((a+b+1)>>1)
#define avg4(a,b,c,d) ((a+b+c+d+2)>>2)

/* init static data */
void dsputil_static_init(void)
{
    int i;

    for(i=0;i<256;i++) cropTbl[i + MAX_NEG_CROP] = i;
    for(i=0;i<MAX_NEG_CROP;i++) {
        cropTbl[i] = 0;
        cropTbl[i + MAX_NEG_CROP + 256] = 255;
    }
    
    for(i=0;i<512;i++) {
        squareTbl[i] = (i - 256) * (i - 256);
    }
    
    for(i=0; i<64; i++) inv_zigzag_direct16[ff_zigzag_direct[i]]= i+1;
}
