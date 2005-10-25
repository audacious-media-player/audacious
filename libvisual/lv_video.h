/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *	    Duilio J. Protti <dprotti@users.sourceforge.net>
 *	    Chong Kai Xiong <descender@phreaker.net>
 *	    Jean-Christophe Hoelt <jeko@ios-software.com>
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

#ifndef _LV_VIDEO_H
#define _LV_VIDEO_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_palette.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VISUAL_VIDEO(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisVideo))

/* NOTE: The depth find helper code in lv_actor depends on an arrangment from low to high */

/**
 * Video flags, for internal use only.
 */
typedef enum {
	VISUAL_VIDEO_FLAG_NONE			= 0,	/**< No flags. */
	VISUAL_VIDEO_FLAG_ALLOCATED_BUFFER	= 1,	/**< Libvisual allocated this buffer. */
	VISUAL_VIDEO_FLAG_EXTERNAL_BUFFER	= 2,	/**< External allocated buffer. */
} VisVideoFlags;

/**
 * Enumerate that defines video depths for use within plugins, libvisual functions, etc.
 */
typedef enum {
	VISUAL_VIDEO_DEPTH_NONE		= 0,	/**< No video surface flag. */
	VISUAL_VIDEO_DEPTH_8BIT		= 1,	/**< 8 bits indexed surface flag. */
	VISUAL_VIDEO_DEPTH_16BIT	= 2,	/**< 16 bits 5-6-5 surface flag. */
	VISUAL_VIDEO_DEPTH_24BIT	= 4,	/**< 24 bits surface flag. */
	VISUAL_VIDEO_DEPTH_32BIT	= 8,	/**< 32 bits surface flag. */
	VISUAL_VIDEO_DEPTH_GL		= 16,	/**< openGL surface flag. */
	VISUAL_VIDEO_DEPTH_ENDLIST	= 32,	/**< Used to mark the end of the depth list. */
	VISUAL_VIDEO_DEPTH_ERROR	= -1,	/**< Used when there is an error. */
	VISUAL_VIDEO_DEPTH_ALL		= VISUAL_VIDEO_DEPTH_8BIT |  \
					  VISUAL_VIDEO_DEPTH_16BIT | \
					  VISUAL_VIDEO_DEPTH_24BIT | \
					  VISUAL_VIDEO_DEPTH_32BIT | \
					  VISUAL_VIDEO_DEPTH_GL /**< All graphical depths. */
} VisVideoDepth;

/**
 * Enumerate that defines the different methods of scaling within VisVideo.
 */
typedef enum {
	VISUAL_VIDEO_SCALE_NEAREST  = 0,    /**< Nearest neighbour. */
	VISUAL_VIDEO_SCALE_BILINEAR = 1	    /**< Bilinearly interpolated. */
} VisVideoScaleMethod;


typedef struct _VisVideo VisVideo;

/**
 * Data structure that contains all the information about a screen surface.
 * Contains all the information regarding a screen surface like the current depth it's in,
 * width, height, bpp, the size in bytes it's pixel buffer is and the screen pitch.
 *
 * It also contains a pointer to the pixels and an optional pointer to the palette.
 *
 * Elements within the structure should be set using the VisVideo system it's methods.
 */
struct _VisVideo {
	VisObject	 object;	/**< The VisObject data. */

	VisVideoDepth	 depth;		/**< Surface it's depth. */
	int		 width;		/**< Surface it's width. */
	int		 height;	/**< Surface it's height. */
	int		 bpp;		/**< Surface it's bytes per pixel. */
	int		 size;		/**< Surface it's screen buffer size in bytes. */
	int		 pitch;		/**< Surface it's pitch value. Value contains
					  * the number of bytes per line. */
	void		*pixels;	/**< Pointer to the pixels. */
	void		**pixel_rows;	/**< Pixel row start pointer table. */
	VisPalette	*pal;		/**< Optional pointer to the palette. */

	VisVideoFlags	flags;		/**< Private field */
};

/* prototypes */
VisVideo *visual_video_new (void);
VisVideo *visual_video_new_with_buffer (int width, int height, VisVideoDepth depth);
int visual_video_free_buffer (VisVideo *video);
int visual_video_allocate_buffer (VisVideo *video);
int visual_video_have_allocated_buffer (const VisVideo *video);
int visual_video_clone (VisVideo *dest, const VisVideo *src);
int visual_video_compare (const VisVideo *src1, const VisVideo *src2);

int visual_video_set_palette (VisVideo *video, VisPalette *pal);
int visual_video_set_buffer (VisVideo *video, void *buffer);
int visual_video_set_dimension (VisVideo *video, int width, int height);
int visual_video_set_pitch (VisVideo *video, int pitch);
int visual_video_set_depth (VisVideo *video, VisVideoDepth depth);

int visual_video_depth_is_supported (int depthflag, VisVideoDepth depth);
VisVideoDepth visual_video_depth_get_next (int depthflag, VisVideoDepth depth);
VisVideoDepth visual_video_depth_get_prev (int depthflag, VisVideoDepth depth);
VisVideoDepth visual_video_depth_get_lowest (int depthflag);
VisVideoDepth visual_video_depth_get_highest (int depthflag);
VisVideoDepth visual_video_depth_get_highest_nogl (int depthflag);
int visual_video_depth_is_sane (VisVideoDepth depth);
int visual_video_depth_value_from_enum (VisVideoDepth depth);
VisVideoDepth visual_video_depth_enum_from_value (int depthvalue);

int visual_video_bpp_from_depth (VisVideoDepth depth);

int visual_video_blit_overlay (VisVideo *dest, const VisVideo *src, int x, int y, int alpha);

int visual_video_alpha_color (VisVideo *video, uint8_t r, uint8_t g, uint8_t b, uint8_t density);
int visual_video_alpha_fill (VisVideo *video, uint8_t density);

int visual_video_color_bgr_to_rgb (VisVideo *dest, const VisVideo *src);

int visual_video_depth_transform (VisVideo *viddest, const VisVideo *vidsrc);
int visual_video_depth_transform_to_buffer (uint8_t *dest, const VisVideo *video,
		VisPalette *pal, VisVideoDepth destdepth, int pitch);

int visual_video_scale (VisVideo *dest, const VisVideo *src, VisVideoScaleMethod scale_method);

/* Optimized versions of performance sensitive routines */
/* mmx from lv_video_mmx.c */ /* FIXME can we do this nicer ? */
int _lv_blit_overlay_alpha32_mmx (VisVideo *dest, const VisVideo *src, int x, int y);
int _lv_scale_bilinear_32_mmx (VisVideo *dest, const VisVideo *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_VIDEO_H */
