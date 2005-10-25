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

#ifndef _LV_TRANSFORM_H
#define _LV_TRANSFORM_H

#include <libvisual/lv_audio.h>
#include <libvisual/lv_video.h>
#include <libvisual/lv_palette.h>
#include <libvisual/lv_plugin.h>
#include <libvisual/lv_songinfo.h>
#include <libvisual/lv_event.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VISUAL_TRANSFORM(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisTransform))

typedef struct _VisTransform VisTransform;

/**
 * The VisTransform structure encapsulates the transform plugin and provides
 * abstract interfaces to the transform. 
 *
 * Members in the structure shouldn't be accessed directly but instead
 * it's adviced to use the methods provided.
 *
 * @see visual_transform_new
 */
struct _VisTransform {
	VisObject	 object;		/**< The VisObject data. */

	VisPluginData	*plugin;		/**< Pointer to the plugin itself. */

	/* Video management and fake environments when needed */
	VisVideo	*video;			/**< Pointer to the target display video. 
						 * @see visual_transform_set_video */
	VisPalette	*pal;			/**< Pointer to the VisPalette that is to be transformed.
						 * @see visual_transform_set_palette */
};

/* prototypes */
VisPluginData *visual_transform_get_plugin (VisTransform *transform);

VisList *visual_transform_get_list (void);
const char *visual_transform_get_next_by_name (const char *name);
const char *visual_transform_get_prev_by_name (const char *name);
int visual_transform_valid_by_name (const char *name);

VisTransform *visual_transform_new (const char *transformname);

int visual_transform_realize (VisTransform *transform);

int visual_transform_video_negotiate (VisTransform *transform);
int visual_transform_get_supported_depth (VisTransform *transform);

int visual_transform_set_video (VisTransform *transform, VisVideo *video);
int visual_transform_set_palette (VisTransform *transform, VisPalette *palette);

int visual_transform_run (VisTransform *transform, VisAudio *audio);
int visual_transform_run_video (VisTransform *transform, VisAudio *audio);
int visual_transform_run_palette (VisTransform *transform, VisAudio *audio);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_TRANSFORM_H */
