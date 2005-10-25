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

#ifndef _LV_ACTOR_H
#define _LV_ACTOR_H

#include <libvisual/lv_audio.h>
#include <libvisual/lv_video.h>
#include <libvisual/lv_palette.h>
#include <libvisual/lv_plugin.h>
#include <libvisual/lv_songinfo.h>
#include <libvisual/lv_event.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VISUAL_ACTOR(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisActor))

typedef struct _VisActor VisActor;

/**
 * The VisActor structure encapsulates the actor plugin and provides
 * abstract interfaces to the actor. The VisActor system 
 * it's methods are also capable of doing automatic size fitting 
 * and depth transformations, and it keeps track on songinfo and events.
 *
 * Members in the structure shouldn't be accessed directly but instead
 * it's adviced to use the methods provided.
 *
 * @see visual_actor_new
 */
struct _VisActor {
	VisObject	 object;		/**< The VisObject data. */

	VisPluginData	*plugin;		/**< Pointer to the plugin itself. */

	/* Video management and fake environments when needed */
	VisVideo	*video;			/**< Pointer to the target display video. 
						 * @see visual_actor_set_video */
	VisVideo	*transform;		/**< Private member which is used for depth transformation. */
	VisVideo	*fitting;		/**< Private member which is used to fit the plugin. */
	VisPalette	*ditherpal;		/**< Private member in which a palette is set when transforming
						 * depth from true color to indexed.
						 * @see visual_actor_get_palette */

	/* Songinfo management */
	VisSongInfo	 songcompare;		/**< Private member which is used to compare with new songinfo
						  * to check if a new song event should be emitted. */
};

/* prototypes */
VisPluginData *visual_actor_get_plugin (VisActor *actor);

VisList *visual_actor_get_list (void);
const char *visual_actor_get_next_by_name_gl (const char *name);
const char *visual_actor_get_prev_by_name_gl (const char *name);
const char *visual_actor_get_next_by_name_nogl (const char *name);
const char *visual_actor_get_prev_by_name_nogl (const char *name);
const char *visual_actor_get_next_by_name (const char *name);
const char *visual_actor_get_prev_by_name (const char *name);
int visual_actor_valid_by_name (const char *name);

VisActor *visual_actor_new (const char *actorname);

int visual_actor_realize (VisActor *actor);

VisSongInfo *visual_actor_get_songinfo (VisActor *actor);
VisPalette *visual_actor_get_palette (VisActor *actor);

int visual_actor_video_negotiate (VisActor *actor, int rundepth, int noevent, int forced);
int visual_actor_get_supported_depth (VisActor *actor);

int visual_actor_set_video (VisActor *actor, VisVideo *video);

int visual_actor_run (VisActor *actor, VisAudio *audio);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_ACTOR_H */
