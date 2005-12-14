/* 

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    output.h

*/

/* Data format encoding bits */

#define PE_MONO 	0x01  /* versus stereo */
#define PE_SIGNED	0x02  /* versus unsigned */
#define PE_16BIT 	0x04  /* versus 8-bit */

/* Conversion functions -- These overwrite the sint32 data in *lp with
   data in another format */

/* 8-bit signed and unsigned*/
extern void s32tos8(void *dp, sint32 *lp, sint32 c);
extern void s32tou8(void *dp, sint32 *lp, sint32 c);

/* 16-bit */
extern void s32tos16(void *dp, sint32 *lp, sint32 c);
extern void s32tou16(void *dp, sint32 *lp, sint32 c);

/* byte-exchanged 16-bit */
extern void s32tos16x(void *dp, sint32 *lp, sint32 c);
extern void s32tou16x(void *dp, sint32 *lp, sint32 c);

/* little-endian and big-endian specific */
#ifdef LITTLE_ENDIAN
#define s32tou16l s32tou16
#define s32tou16b s32tou16x
#define s32tos16l s32tos16
#define s32tos16b s32tos16x
#else
#define s32tou16l s32tou16x
#define s32tou16b s32tou16
#define s32tos16l s32tos16x
#define s32tos16b s32tos16
#endif
