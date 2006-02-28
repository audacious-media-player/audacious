/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define NOT_NATIVE_ENDIAN						\
		  ((IS_BIG_ENDIAN &&					\
		   (output.format.sun == AUDIO_ENCODING_SLINEAR_LE ||	\
		    output.format.sun == AUDIO_ENCODING_ULINEAR_LE))||	\
		  (!IS_BIG_ENDIAN &&					\
		   (output.format.sun == AUDIO_ENCODING_SLINEAR_BE ||	\
		    output.format.sun == AUDIO_ENCODING_ULINEAR_BE)))

#define RESAMPLE_STEREO(sample_type)				\
do {								\
	const int shift = sizeof (sample_type);			\
        int i, in_samples, out_samples, x, delta;		\
	sample_type *inptr = (sample_type *)ob, *outptr;	\
	guint nlen = (((length >> shift) * espeed) / speed);	\
								\
	if (nlen == 0)						\
		break;						\
	nlen <<= shift;						\
	if (NOT_NATIVE_ENDIAN)					\
		sun_bswap16(ob, length);			\
	if (nlen > nbuffer_size)				\
	{							\
		nbuffer = g_realloc(nbuffer, nlen);		\
		nbuffer_size = nlen;				\
	}							\
	outptr = (sample_type *)nbuffer;			\
	in_samples = length >> shift;				\
        out_samples = nlen >> shift;				\
	delta = (in_samples << 12) / out_samples;		\
	for (x = 0, i = 0; i < out_samples; i++)		\
	{							\
		int x1, frac;					\
		x1 = (x >> 12) << 12;				\
		frac = x - x1;					\
		*outptr++ =					\
			(sample_type)				\
			((inptr[(x1 >> 12) << 1] *		\
			  ((1<<12) - frac) +			\
			  inptr[((x1 >> 12) + 1) << 1] *	\
			  frac) >> 12);				\
		*outptr++ =					\
			(sample_type)				\
			((inptr[((x1 >> 12) << 1) + 1] *	\
			  ((1<<12) - frac) +			\
			  inptr[(((x1 >> 12) + 1) << 1) + 1] *	\
			  frac) >> 12);				\
		x += delta;					\
	}							\
	if (NOT_NATIVE_ENDIAN)					\
		sun_bswap16(nbuffer, nlen);			\
	w = write_all(audio.fd, nbuffer, nlen);			\
} while (0)

#define RESAMPLE_MONO(sample_type)				\
do {								\
	const int shift = sizeof (sample_type) - 1;		\
        int i, x, delta, in_samples, out_samples;		\
	sample_type *inptr = (sample_type *)ob, *outptr;	\
	guint nlen = (((length >> shift) * espeed) / speed);	\
								\
	if (nlen == 0)						\
		break;						\
	nlen <<= shift;						\
	if (NOT_NATIVE_ENDIAN)					\
		sun_bswap16(ob, length);			\
	if (nlen > nbuffer_size)				\
	{							\
		nbuffer = g_realloc(nbuffer, nlen);		\
		nbuffer_size = nlen;				\
	}							\
	outptr = (sample_type *)nbuffer;			\
	in_samples = length >> shift;				\
        out_samples = nlen >> shift;				\
	delta = ((length >> shift) << 12) / out_samples;	\
	for (x = 0, i = 0; i < out_samples; i++)		\
	{							\
		int x1, frac;					\
		x1 = (x >> 12) << 12;				\
		frac = x - x1;					\
		*outptr++ =					\
			(sample_type)				\
			((inptr[x1 >> 12] * ((1<<12) - frac) +	\
			  inptr[(x1 >> 12) + 1] * frac) >> 12);	\
		x += delta;					\
	}							\
	if (NOT_NATIVE_ENDIAN)					\
		sun_bswap16(nbuffer, nlen);			\
	w = write_all(audio.fd, nbuffer, nlen);			\
} while (0)

