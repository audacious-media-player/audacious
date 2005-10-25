/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "lv_common.h"
#include "lv_log.h"
#include "lv_endianess.h"
#include "lv_bmp.h"

#define BI_RGB	0
#define BI_RLE8	1
#define BI_RLE	2

/**
 * @defgroup VisBitmap VisBitmap
 * @{
 */

/**
 * Loads a BMP file into a VisVideo. The buffer will be located
 * for the VisVideo.
 *
 * The loader currently only supports uncompressed bitmaps in the form
 * of either 8 bits indexed or 24 bits RGB.
 *
 * Keep in mind that you need to free the palette by hand.
 * 
 * @param video Destination video where the bitmap should be loaded in.
 * @param filename The filename of the bitmap to be loaded.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_VIDEO_NULL, -VISUAL_ERROR_BMP_NOT_FOUND,
 * 	-VISUAL_ERROR_BMP_NO_BMP, -VISUAL_ERROR_BMP_NOT_SUPPORTED or -VISUAL_ERROR_BMP_CORRUPTED
 * 	on failure.
 */
int visual_bitmap_load (VisVideo *video, const char *filename)
{
	/* The win32 BMP header */
	char magic[2];
	uint32_t bf_size = 0;
	uint32_t bf_bits = 0;

	/* The win32 BITMAPINFOHEADER */
	int32_t bi_size = 0;
	int32_t bi_width = 0;
	int32_t bi_height = 0;
	int16_t bi_bitcount = 0;	
	uint32_t bi_compression;
	uint32_t bi_clrused;

	/* File read vars */
	int fd;

	/* Worker vars */
	uint8_t *data;
	int pad;
	int i;

	visual_log_return_val_if_fail (video != NULL, -VISUAL_ERROR_VIDEO_NULL);

	fd = open (filename, O_RDONLY);

	if (fd < 0) {
		visual_log (VISUAL_LOG_WARNING, "Bitmap file not found: %s", filename);

		return -VISUAL_ERROR_BMP_NOT_FOUND;
	}

	/* Read the magic string */
	read (fd, magic, 2);

	if (strncmp (magic, "BM", 2) != 0) {
		visual_log (VISUAL_LOG_WARNING, "Not a bitmap file"); 
	
		return -VISUAL_ERROR_BMP_NO_BMP;
	}

	/* Read the file size */
	read (fd, &bf_size, 4);
	bf_size = VISUAL_ENDIAN_LEI32 (bf_size);

	/* Skip past the reserved bits */
	lseek (fd, 4, SEEK_CUR);

	/* Read the offset bits */
	read (fd, &bf_bits, 4);
	bf_bits = VISUAL_ENDIAN_LEI32 (bf_bits);

	/* Read the info structure size */
	read (fd, &bi_size, 4);
	bi_size = VISUAL_ENDIAN_LEI32 (bi_size);

	if (bi_size == 12) {
		/* And read the width, height */
		read (fd, &bi_width, 2);
		read (fd, &bi_height, 2);
		bi_width = VISUAL_ENDIAN_LEI16 (bi_width);
		bi_height = VISUAL_ENDIAN_LEI16 (bi_height);

		/* Skip over the planet */
		lseek (fd, 2, SEEK_CUR);

		/* Read the bits per pixel */
		read (fd, &bi_bitcount, 2);
		bi_bitcount = VISUAL_ENDIAN_LEI16 (bi_bitcount);

		bi_compression = BI_RGB;
	} else {
		/* And read the width, height */
		read (fd, &bi_width, 4);
		read (fd, &bi_height, 4);
		bi_width = VISUAL_ENDIAN_LEI16 (bi_width);
		bi_height = VISUAL_ENDIAN_LEI16 (bi_height);

		/* Skip over the planet */
		lseek (fd, 2, SEEK_CUR);

		/* Read the bits per pixel */
		read (fd, &bi_bitcount, 2);
		bi_bitcount = VISUAL_ENDIAN_LEI16 (bi_bitcount);

		/* Read the compression flag */
		read (fd, &bi_compression, 4);
		bi_compression = VISUAL_ENDIAN_LEI32 (bi_compression);

		/* Skip over the nonsense we don't want to know */
		lseek (fd, 12, SEEK_CUR);

		/* Number of colors in palette */
		read (fd, &bi_clrused, 4);
		bi_clrused = VISUAL_ENDIAN_LEI32 (bi_clrused);

		/* Skip over the other nonsense */
		lseek (fd, 4, SEEK_CUR);
	}

	/* Check if we can handle it */
	if (bi_bitcount != 8 && bi_bitcount != 24) {
		visual_log (VISUAL_LOG_CRITICAL, "Only bitmaps with 8 bits or 24 bits per pixel are supported");
		
		return -VISUAL_ERROR_BMP_NOT_SUPPORTED;
	}

	/* We only handle uncompressed bitmaps */
	if (bi_compression != BI_RGB) {
		visual_log (VISUAL_LOG_CRITICAL, "Only uncompressed bitmaps are supported");

		return -VISUAL_ERROR_BMP_NOT_SUPPORTED;
	}

	/* Load the palette */
	if (bi_bitcount == 8) {
		if (bi_clrused == 0)
			bi_clrused = 256;

		if (video->pal == NULL)
			visual_object_unref (VISUAL_OBJECT (video->pal));
		
		video->pal = visual_palette_new (bi_clrused);

		if (bi_size == 12) {
			for (i = 0; i < bi_clrused; i++) {
				read (fd, &video->pal->colors[i].b, 1);
				read (fd, &video->pal->colors[i].g, 1);
				read (fd, &video->pal->colors[i].r, 1);
			}
		} else {
			for (i = 0; i < bi_clrused; i++) {
				read (fd, &video->pal->colors[i].b, 1);
				read (fd, &video->pal->colors[i].g, 1);
				read (fd, &video->pal->colors[i].r, 1);
				lseek (fd, 1, SEEK_CUR);
			}
		}
	}

	/* Make the target VisVideo ready for use */
	visual_video_set_depth (video, visual_video_depth_enum_from_value (bi_bitcount));
	visual_video_set_dimension (video, bi_width, bi_height);
	visual_video_allocate_buffer (video);

	/* Set to the beginning of image data, note that MickeySoft likes stuff upside down .. */
	lseek (fd, bf_bits, SEEK_SET);

	pad = ((video->pitch % 4) ? (4 - (video->pitch % 4)) : 0);

	data = video->pixels + (video->height * video->pitch);
	while (data > (uint8_t *) video->pixels) {
		data -= video->pitch;

		if (read (fd, data, video->pitch) != video->pitch) {
			visual_log (VISUAL_LOG_CRITICAL, "Bitmap data is not complete");
			
			visual_video_free_buffer (video);

			return -VISUAL_ERROR_BMP_CORRUPTED;
		}

#if !VISUAL_LITTLE_ENDIAN
		switch (bi_bitcount) {
			case 24: {
				int i, p;
				for (p=0, i=0; i<bi_width; i++){
#	if VISUAL_BIG_ENDIAN
					uint8_t c[2];

					c[0] = data[p];
					c[1] = data[p+2];

					data[p] = c[1];
					data[p+2] = c[0];

					p+=3;
#	else
#		error todo
#	endif
				}
				break;
			}
			default:
				visual_log (VISUAL_LOG_CRITICAL, "Internal error.");
		}
#endif

		if (pad != 0) {
			lseek (fd, 4, pad);
		}
	}

	close (fd);

	return VISUAL_OK;
}

/**
 * Loads a bitmap into a VisVideo and return this, so it's not needed to 
 * allocate a VisVideo before by hand.
 *
 * @see visual_bitmap_load
 *
 * @param filename The filename of the bitmap to be loaded.
 *
 * @return The VisVideo containing the bitmap or NULL on failure.
 */
VisVideo *visual_bitmap_load_new_video (const char *filename)
{
	VisVideo *video;

	video = visual_video_new ();
	
	if (visual_bitmap_load (video, filename) < 0) {
		visual_object_unref (VISUAL_OBJECT (video));

		return NULL;
	}

	return video;
}

/**
 * @}
 */

