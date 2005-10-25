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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lvconfig.h"
#include "lv_log.h"
#include "lv_list.h"
#include "lv_transform.h"
#include "lv_mem.h"

extern VisList *__lv_plugins_transform;

static int transform_dtor (VisObject *object);

static VisTransformPlugin *get_transform_plugin (VisTransform *transform);


static int transform_dtor (VisObject *object)
{
	VisTransform *transform = VISUAL_TRANSFORM (object);

	if (transform->plugin != NULL)
		visual_plugin_unload (transform->plugin);

	transform->plugin = NULL;

	return VISUAL_OK;
}

static VisTransformPlugin *get_transform_plugin (VisTransform *transform)
{
	VisTransformPlugin *transplugin;

	visual_log_return_val_if_fail (transform != NULL, NULL);
	visual_log_return_val_if_fail (transform->plugin != NULL, NULL);

	transplugin = VISUAL_PLUGIN_TRANSFORM (transform->plugin->info->plugin);

	return transplugin;
}

/**
 * @defgroup VisTransform VisTransform
 * @{
 */

/**
 * Gives the encapsulated VisPluginData from a VisTransform.
 *
 * @param transform Pointer of a VisTransform of which the VisPluginData needs to be returned.
 *
 * @return VisPluginData that is encapsulated in the VisTransform, possibly NULL.
 */
VisPluginData *visual_transform_get_plugin (VisTransform *transform)
{
	return transform->plugin;
}

/**
 * Gives a list of VisTransforms in the current plugin registry.
 *
 * @return An VisList containing the VisTransforms in the plugin registry.
 */
VisList *visual_transform_get_list ()
{
	return __lv_plugins_transform;
}

/**
 * Gives the next transform plugin based on the name of a plugin.
 *
 * @see visual_transform_get_prev_by_name
 * 
 * @param name The name of the current plugin, or NULL to get the first.
 *
 * @return The name of the next plugin within the list.
 */
const char *visual_transform_get_next_by_name (const char *name)
{
	return visual_plugin_get_next_by_name (visual_transform_get_list (), name);
}

/**
 * Gives the previous transform plugin based on the name of a plugin.
 *
 * @see visual_transform_get_next_by_name
 * 
 * @param name The name of the current plugin. or NULL to get the last.
 *
 * @return The name of the previous plugin within the list.
 */
const char *visual_transform_get_prev_by_name (const char *name)
{
	return visual_plugin_get_prev_by_name (visual_transform_get_list (), name);
}

/**
 * Checks if the transform plugin is in the registry, based on it's name.
 *
 * @param name The name of the plugin that needs to be checked.
 *
 * @return TRUE if found, else FALSE.
 */
int visual_transform_valid_by_name (const char *name)
{
	if (visual_plugin_find (visual_transform_get_list (), name) == NULL)
		return FALSE;
	else
		return TRUE;
}

/**
 * Creates a new transform from name, the plugin will be loaded but won't be realized.
 *
 * @param transformname
 * 	The name of the plugin to load, or NULL to simply allocate a new
 * 	transform. 
 *
 * @return A newly allocated VisTransform, optionally containing a loaded plugin. Or NULL on failure.
 */
VisTransform *visual_transform_new (const char *transformname)
{
	VisTransform *transform;
	VisPluginRef *ref;

	if (__lv_plugins_transform == NULL && transformname != NULL) {
		visual_log (VISUAL_LOG_CRITICAL, "the plugin list is NULL");
		return NULL;
	}
	
	transform = visual_mem_new0 (VisTransform, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (transform), TRUE, transform_dtor);

	if (transformname == NULL)
		return transform;

	ref = visual_plugin_find (__lv_plugins_transform, transformname);

	transform->plugin = visual_plugin_load (ref);

	return transform;
}

/**
 * Realize the VisTransform. This also calls the plugin init function.
 *
 * @param transform Pointer to a VisTransform that needs to be realized.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TRANSFORM_NULL, -VISUAL_ERROR_PLUGIN_NULL or
 *	error values returned by visual_plugin_realize () on failure.
 * 
 */
int visual_transform_realize (VisTransform *transform)
{
	visual_log_return_val_if_fail (transform != NULL, -VISUAL_ERROR_TRANSFORM_NULL);
	visual_log_return_val_if_fail (transform->plugin != NULL, -VISUAL_ERROR_PLUGIN_NULL);

	return visual_plugin_realize (transform->plugin);
}

/**
 * This function negotiates the VisTransform with it's target video that is set by visual_transform_set_video.
 * When needed it also sets up size fitting environment and depth transformation environment.
 *
 * @param transform Pointer to a VisTransform that needs negotiation.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TRANSFORM_NULL, -VISUAL_ERROR_PLUGIN_NULL, -VISUAL_ERROR_PLUGIN_REF_NULL
 * 	or -VISUAL_ERROR_TRANSFORM_NEGOTIATE on failure. 
 */ 
int visual_transform_video_negotiate (VisTransform *transform)
{
	int depthflag;

	visual_log_return_val_if_fail (transform != NULL, -VISUAL_ERROR_TRANSFORM_NULL);
	visual_log_return_val_if_fail (transform->plugin != NULL, -VISUAL_ERROR_PLUGIN_NULL);
	visual_log_return_val_if_fail (transform->plugin->ref != NULL, -VISUAL_ERROR_PLUGIN_REF_NULL);

	depthflag = visual_transform_get_supported_depth (transform);

	if (visual_video_depth_is_supported (depthflag, transform->video->depth) == FALSE)
		return -VISUAL_ERROR_TRANSFORM_NEGOTIATE;
	
	visual_event_queue_add_resize (&transform->plugin->eventqueue, transform->video,
			transform->video->width, transform->video->height);

	visual_plugin_events_pump (transform->plugin);

	return -VISUAL_OK;
}

/**
 * Gives the by the plugin natively supported depths
 *
 * @param transform Pointer to a VisTransform of which the supported depth of it's
 * 	  encapsulated plugin is requested.
 *
 * @return an OR value of the VISUAL_VIDEO_DEPTH_* values which can be checked against using AND on succes,
 * 	-VISUAL_ERROR_TRANSFORM_NULL, -VISUAL_ERROR_PLUGIN_NULL or -VISUAL_ERROR_TRANSFORM_PLUGIN_NULL on failure.
 */
int visual_transform_get_supported_depth (VisTransform *transform)
{
	VisTransformPlugin *transplugin;

	visual_log_return_val_if_fail (transform != NULL, -VISUAL_ERROR_TRANSFORM_NULL);
	visual_log_return_val_if_fail (transform->plugin != NULL, -VISUAL_ERROR_PLUGIN_NULL);

	transplugin = get_transform_plugin (transform);

	if (transplugin == NULL)
		return -VISUAL_ERROR_TRANSFORM_PLUGIN_NULL;

	return transplugin->depth;
}

/**
 * Used to connect the target display it's VisVideo structure to the VisTransform.
 *
 * Using the visual_video methods the screenbuffer, it's depth and dimension and optionally it's pitch
 * can be set so the transform plugins know about their graphical environment and have a place to draw.
 *
 * After this function it's most likely that visual_transform_video_negotiate needs to be called.
 *
 * @see visual_video_new
 * @see visual_transform_video_negotiate
 * 
 * @param transform Pointer to a VisTransform to which the VisVideo needs to be set.
 * @param video Pointer to a VisVideo which contains information about the target display and the pointer
 * 	  to it's screenbuffer.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TRANSFORM_NULL on failure.
 */
int visual_transform_set_video (VisTransform *transform, VisVideo *video)
{
	visual_log_return_val_if_fail (transform != NULL, -VISUAL_ERROR_TRANSFORM_NULL);

	transform->video = video;

	if (video != NULL)
		transform->pal = video->pal;
	else
		transform->pal = NULL;

	return VISUAL_OK;
}

/**
 * Used to override the palette that is extracted from the VisVideo that is given using
 * visual_transform_set_video. Always call this function after visual_transform_set_video is called.
 * 
 * @see visual_transform_set_video
 *
 * @param transform Pointer to a VisTransform to which the VisVideo needs to be set.
 * @param palette Pointer to the VisPalette which is used to override the palette in the VisTransform.
 * 
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TRANSFORM_NULL on failure.
 */ 
int visual_transform_set_palette (VisTransform *transform, VisPalette *palette)
{
	visual_log_return_val_if_fail (transform != NULL, -VISUAL_ERROR_TRANSFORM_NULL);

	transform->pal = palette;

	return VISUAL_OK;
}


/**
 * This is called to run a VisTransform.
 *
 * @see visual_transform_run_video
 * @see visual_transform_run_palette
 *
 * @param transform Pointer to a VisTransform that needs to be runned.
 * @param audio Pointer to a VisAudio that contains all the audio data.
 *
 * return VISUAL_OK on succes, -VISUAL_ERROR_TRANSFORM_NULL or error values returned by
 *	either visual_transform_run_video or visual_transform_run_palette on failure.
 */
int visual_transform_run (VisTransform *transform, VisAudio *audio)
{
	int ret;
	visual_log_return_val_if_fail (transform != NULL, -VISUAL_ERROR_TRANSFORM_NULL);

	if (transform->video != NULL) {
		if ((ret = visual_transform_run_video (transform, audio)) != VISUAL_OK)
			return ret;
	}
	
	if (transform->pal != NULL) {
		if ((ret = visual_transform_run_palette (transform, audio)) != VISUAL_OK)
			return ret;
	}

	return VISUAL_OK;
}

/**
 * This is called to run the video part of a VisTransform.
 *
 * @see visual_transform_run
 *
 * @param transform Pointer to a VisTransform that needs to be runned.
 * @param audio Pointer to a VisAudio that contains all the audio data.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TRANSFORM_NULL, -VISUAL_ERROR_TRANSFORM_VIDEO_NULL
 *	or -VISUAL_ERROR_TRANSFORM_PLUGIN_NULL on failure.
 */
int visual_transform_run_video (VisTransform *transform, VisAudio *audio)
{
	VisTransformPlugin *transplugin;
	VisPluginData *plugin;

	visual_log_return_val_if_fail (transform != NULL, -VISUAL_ERROR_TRANSFORM_NULL);
	visual_log_return_val_if_fail (transform->video != NULL, -VISUAL_ERROR_TRANSFORM_VIDEO_NULL);

	transplugin = get_transform_plugin (transform);
	plugin = visual_transform_get_plugin (transform);

	if (transplugin == NULL) {
		visual_log (VISUAL_LOG_CRITICAL,
			"The given transform does not reference any transform plugin");

		return -VISUAL_ERROR_TRANSFORM_PLUGIN_NULL;
	}

	visual_plugin_events_pump (plugin);

	transplugin->video (plugin, transform->video, audio);

	return VISUAL_OK;
}

/**
 * This is called to run the palette part of a VisTransform.
 *
 * @see visual_transform_run
 *
 * @param transform Pointer to a VisTransform that needs to be runned.
 * @param audio Pointer to a VisAudio that contains all the audio data.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TRANSFORM_NULL, -VISUAL_ERROR_TRANSFORM_PALETTE_NULL
 *	or -VISUAL_ERROR_TRANSFORM_PLUGIN_NULL on failure.
 */
int visual_transform_run_palette (VisTransform *transform, VisAudio *audio)
{
	VisTransformPlugin *transplugin;
	VisPluginData *plugin;

	visual_log_return_val_if_fail (transform != NULL, -VISUAL_ERROR_TRANSFORM_NULL);
	visual_log_return_val_if_fail (transform->pal != NULL, -VISUAL_ERROR_TRANSFORM_PALETTE_NULL);

	transplugin = get_transform_plugin (transform);
	plugin = visual_transform_get_plugin (transform);

	if (transplugin == NULL) {
		visual_log (VISUAL_LOG_CRITICAL,
			"The given transform does not reference any transform plugin");

		return -VISUAL_ERROR_TRANSFORM_PLUGIN_NULL;
	}

	visual_plugin_events_pump (plugin);

	transplugin->palette (plugin, transform->pal, audio);

	return VISUAL_OK;
}

/**
 * @}
 */

