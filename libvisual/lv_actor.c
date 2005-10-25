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
#include "lv_actor.h"
#include "lv_mem.h"

extern VisList *__lv_plugins_actor;

static int actor_dtor (VisObject *object);

static VisActorPlugin *get_actor_plugin (VisActor *actor);
static int negotiate_video_with_unsupported_depth (VisActor *actor, int rundepth, int noevent, int forced);
static int negotiate_video (VisActor *actor, int noevent);


static int actor_dtor (VisObject *object)
{
	VisActor *actor = VISUAL_ACTOR (object);

	if (actor->plugin != NULL)
		visual_plugin_unload (actor->plugin);

	if (actor->transform != NULL)
		visual_object_unref (VISUAL_OBJECT (actor->transform));

	if (actor->fitting != NULL)
		visual_object_unref (VISUAL_OBJECT (actor->fitting));

	visual_object_unref (VISUAL_OBJECT (&actor->songcompare));

	actor->plugin = NULL;
	actor->transform = NULL;
	actor->fitting = NULL;

	return VISUAL_OK;
}

static VisActorPlugin *get_actor_plugin (VisActor *actor)
{
	VisActorPlugin *actplugin;

	visual_log_return_val_if_fail (actor != NULL, NULL);
	visual_log_return_val_if_fail (actor->plugin != NULL, NULL);

	actplugin = VISUAL_PLUGIN_ACTOR (actor->plugin->info->plugin);

	return actplugin;
}

/**
 * @defgroup VisActor VisActor
 * @{
 */

/**
 * Gives the encapsulated VisPluginData from a VisActor.
 *
 * @param actor Pointer of a VisActor of which the VisPluginData needs to be returned.
 *
 * @return VisPluginData that is encapsulated in the VisActor, possibly NULL.
 */
VisPluginData *visual_actor_get_plugin (VisActor *actor)
{
	return actor->plugin;
}

/**
 * Gives a list of VisActors in the current plugin registry.
 *
 * @return An VisList containing the VisActors in the plugin registry.
 */
VisList *visual_actor_get_list ()
{
	return __lv_plugins_actor;
}

/** @todo find a way to NOT load, unload the plugins */

/**
 * Gives the next actor plugin based on the name of a plugin but skips non
 * GL plugins.
 *
 * @see visual_actor_get_prev_by_name_gl
 *
 * @param name The name of the current plugin or NULL to get the first.
 *
 * @return The name of the next plugin within the list that is a GL plugin.
 */
const char *visual_actor_get_next_by_name_gl (const char *name)
{
	const char *next = name;
	VisPluginData *plugin;
	VisPluginRef *ref;
	VisActorPlugin *actplugin;
	int gl;

	do {
		next = visual_plugin_get_next_by_name (visual_actor_get_list (), next);

		if (next == NULL)
			return NULL;
		
		ref = visual_plugin_find (__lv_plugins_actor, next);
		plugin = visual_plugin_load (ref);

		actplugin = VISUAL_PLUGIN_ACTOR (plugin->info->plugin);

		if ((actplugin->depth & VISUAL_VIDEO_DEPTH_GL) > 0)
			gl = TRUE;
		else
			gl = FALSE;
	
		visual_plugin_unload (plugin);

	} while (gl == FALSE);

	return next;
}

/**
 * Gives the previous actor plugin based on the name of a plugin but skips non
 * GL plugins.
 *
 * @see visual_actor_get_next_by_name_gl
 *
 * @param name The name of the current plugin or NULL to get the last.
 *
 * @return The name of the previous plugin within the list that is a GL plugin.
 */
const char *visual_actor_get_prev_by_name_gl (const char *name)
{
	const char *prev = name;
	VisPluginData *plugin;
	VisPluginRef *ref;
	VisActorPlugin *actplugin;
	int gl;

	do {
		prev = visual_plugin_get_prev_by_name (visual_actor_get_list (), prev);

		if (prev == NULL)
			return NULL;
		
		ref = visual_plugin_find (__lv_plugins_actor, prev);
		plugin = visual_plugin_load (ref);
		actplugin = VISUAL_PLUGIN_ACTOR (plugin->info->plugin);

		if ((actplugin->depth & VISUAL_VIDEO_DEPTH_GL) > 0)
			gl = TRUE;
		else
			gl = FALSE;
	
		visual_plugin_unload (plugin);

	} while (gl == FALSE);

	return prev;
}

/**
 * Gives the next actor plugin based on the name of a plugin but skips
 * GL plugins.
 *
 * @see visual_actor_get_prev_by_name_nogl
 *
 * @param name The name of the current plugin or NULL to get the first.
 *
 * @return The name of the next plugin within the list that is not a GL plugin.
 */
const char *visual_actor_get_next_by_name_nogl (const char *name)
{
	const char *next = name;
	VisPluginData *plugin;
	VisPluginRef *ref;
	VisActorPlugin *actplugin;
	int gl;

	do {
		next = visual_plugin_get_next_by_name (visual_actor_get_list (), next);

		if (next == NULL)
			return NULL;
		
		ref = visual_plugin_find (__lv_plugins_actor, next);
		plugin = visual_plugin_load (ref);
		actplugin = VISUAL_PLUGIN_ACTOR (plugin->info->plugin);

		if ((actplugin->depth & VISUAL_VIDEO_DEPTH_GL) > 0)
			gl = TRUE;
		else
			gl = FALSE;
	
		visual_plugin_unload (plugin);

	} while (gl == TRUE);

	return next;
}

/**
 * Gives the previous actor plugin based on the name of a plugin but skips
 * GL plugins.
 *
 * @see visual_actor_get_next_by_name_nogl
 *
 * @param name The name of the current plugin or NULL to get the last.
 *
 * @return The name of the previous plugin within the list that is not a GL plugin.
 */
const char *visual_actor_get_prev_by_name_nogl (const char *name)
{
	const char *prev = name;
	VisPluginData *plugin;
	VisPluginRef *ref;
	VisActorPlugin *actplugin;
	int gl;

	do {
		prev = visual_plugin_get_prev_by_name (visual_actor_get_list (), prev);

		if (prev == NULL)
			return NULL;
		
		ref = visual_plugin_find (__lv_plugins_actor, prev);
		plugin = visual_plugin_load (ref);
		actplugin = VISUAL_PLUGIN_ACTOR (plugin->info->plugin);

		if ((actplugin->depth & VISUAL_VIDEO_DEPTH_GL) > 0)
			gl = TRUE;
		else
			gl = FALSE;
	
		visual_plugin_unload (plugin);

	} while (gl == TRUE);

	return prev;
}

/**
 * Gives the next actor plugin based on the name of a plugin.
 *
 * @see visual_actor_get_prev_by_name
 * 
 * @param name The name of the current plugin, or NULL to get the first.
 *
 * @return The name of the next plugin within the list.
 */
const char *visual_actor_get_next_by_name (const char *name)
{
	return visual_plugin_get_next_by_name (visual_actor_get_list (), name);
}

/**
 * Gives the previous actor plugin based on the name of a plugin.
 *
 * @see visual_actor_get_next_by_name
 * 
 * @param name The name of the current plugin. or NULL to get the last.
 *
 * @return The name of the previous plugin within the list.
 */
const char *visual_actor_get_prev_by_name (const char *name)
{
	return visual_plugin_get_prev_by_name (visual_actor_get_list (), name);
}

/**
 * Checks if the actor plugin is in the registry, based on it's name.
 *
 * @param name The name of the plugin that needs to be checked.
 *
 * @return TRUE if found, else FALSE.
 */
int visual_actor_valid_by_name (const char *name)
{
	if (visual_plugin_find (visual_actor_get_list (), name) == NULL)
		return FALSE;
	else
		return TRUE;
}

/**
 * Creates a new actor from name, the plugin will be loaded but won't be realized.
 *
 * @param actorname
 * 	The name of the plugin to load, or NULL to simply allocate a new
 * 	actor. 
 *
 * @return A newly allocated VisActor, optionally containing a loaded plugin. Or NULL on failure.
 */
VisActor *visual_actor_new (const char *actorname)
{
	VisActor *actor;
	VisPluginRef *ref;

	if (__lv_plugins_actor == NULL && actorname != NULL) {
		visual_log (VISUAL_LOG_CRITICAL, "the plugin list is NULL");
		return NULL;
	}
	
	actor = visual_mem_new0 (VisActor, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (actor), TRUE, actor_dtor);

	if (actorname == NULL)
		return actor;

	ref = visual_plugin_find (__lv_plugins_actor, actorname);

	actor->plugin = visual_plugin_load (ref);

	return actor;
}

/**
 * Realize the VisActor. This also calls the plugin init function.
 *
 * @param actor Pointer to a VisActor that needs to be realized.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_ACTOR_NULL, -VISUAL_ERROR_PLUGIN_NULL or
 *	error values returned by visual_plugin_realize () on failure.
 * 
 */
int visual_actor_realize (VisActor *actor)
{
	visual_log_return_val_if_fail (actor != NULL, -VISUAL_ERROR_ACTOR_NULL);
	visual_log_return_val_if_fail (actor->plugin != NULL, -VISUAL_ERROR_PLUGIN_NULL);

	return visual_plugin_realize (actor->plugin);
}

/**
 * Gives a pointer to the song info data within the VisActor. This song info data can be used
 * to set name, artist and even coverart which can be used by the plugins and the framework itself.
 *
 * @see VisSongInfo
 *
 * @param actor Pointer to a VisActor of which the song info is needed.
 *
 * @return Pointer to the song info structure on succes or NULL on failure.
 */
VisSongInfo *visual_actor_get_songinfo (VisActor *actor)
{
	VisActorPlugin *actplugin;

	visual_log_return_val_if_fail (actor != NULL, NULL);

	actplugin = get_actor_plugin (actor);
	visual_log_return_val_if_fail (actplugin != NULL, NULL);

	return &actplugin->songinfo;
}

/**
 * Gives a pointer to the palette within the VisActor. This can be needed to set a palette on the target
 * display when it's in index mode.
 *
 * @see VisPalette
 *
 * @param actor Pointer to a VisActor of which the palette is needed.
 *
 * @return Pointer to the palette structure on succes or NULL on failure. Also it's possible that NULL
 * is returned when the plugin is running in a full color mode or openGL. The returned palette is
 * read only.
 */
VisPalette *visual_actor_get_palette (VisActor *actor)
{
	VisActorPlugin *actplugin;

	visual_log_return_val_if_fail (actor != NULL, NULL);

	actplugin = get_actor_plugin (actor);
	
	if (actplugin == NULL) {
		visual_log (VISUAL_LOG_CRITICAL,
			"The given actor does not reference any actor plugin");
		return NULL;
	}

	if (actor->transform != NULL &&
		actor->video->depth == VISUAL_VIDEO_DEPTH_8BIT) {
		
		return actor->ditherpal;

	} else {
		return actplugin->palette (visual_actor_get_plugin (actor));
	}

	return NULL;
}

/**
 * This function negotiates the VisActor with it's target video that is set by visual_actor_set_video.
 * When needed it also sets up size fitting environment and depth transformation environment.
 *
 * The function has a few extra arguments that are mainly to be used from within internal code.
 *
 * This function needs to be called everytime there is a change within either the size or depth of
 * the target video. 
 *
 * The main method of calling this function is: "visual_actor_video_negotiate (actor, 0, FALSE, FALSE)"
 * 
 * @see visual_actor_set_video
 *
 * @param actor Pointer to a VisActor that needs negotiation.
 * @param rundepth An depth in the form of the VISUAL_VIDEO_DEPTH_* style when a depth is forced.
 * 	  This could be needed when for example a plugin has both a 8 bits and a 32 bits display method
 * 	  but while the target video is in 32 bits you still want to run the plugin in 8 bits. If this
 * 	  is desired the "forced" argument also needs to be set on TRUE.
 * @param noevent When set on TRUE this does only renegotiate depth transformation environments. For example
 * 	  when the target display was running in 32 bits and switched to 8 bits while the plugin was already
 * 	  in 8 bits it doesn't need an events, which possibly reinitializes the plugin.
 * @param forced This should be set if the rundepth argument is set, so it forces the plugin in a certain
 * 	  depth.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_ACTOR_NULL, -VISUAL_ERROR_PLUGIN_NULL, -VISUAL_ERROR_PLUGIN_REF_NULL,
 * 	-VISUAL_ERROR_ACTOR_VIDEO_NULL or -VISUAL_ERROR_ACTOR_GL_NEGOTIATE on failure. 
 */ 
int visual_actor_video_negotiate (VisActor *actor, int rundepth, int noevent, int forced)
{
	int depthflag;

	visual_log_return_val_if_fail (actor != NULL, -VISUAL_ERROR_ACTOR_NULL);
	visual_log_return_val_if_fail (actor->plugin != NULL, -VISUAL_ERROR_PLUGIN_NULL);
	visual_log_return_val_if_fail (actor->plugin->ref != NULL, -VISUAL_ERROR_PLUGIN_REF_NULL);
	visual_log_return_val_if_fail (actor->video != NULL, -VISUAL_ERROR_ACTOR_VIDEO_NULL);

	if (actor->transform != NULL) {
		visual_object_unref (VISUAL_OBJECT (actor->transform));

		actor->transform = NULL;
	}
	
	if (actor->fitting != NULL) {
		visual_object_unref (VISUAL_OBJECT (actor->fitting));
		
		actor->fitting = NULL;
	}

	if (actor->ditherpal != NULL) {
		visual_object_unref (VISUAL_OBJECT (actor->ditherpal));
		
		actor->ditherpal = NULL;
	}

	depthflag = visual_actor_get_supported_depth (actor);

	visual_log (VISUAL_LOG_INFO, "negotiating plugin %s", actor->plugin->info->name);

	/* Set up depth transformation enviroment */
	if (visual_video_depth_is_supported (depthflag, actor->video->depth) != TRUE ||
			(forced == TRUE && actor->video->depth != rundepth))
		/* When the depth is not supported, or if we only switch the depth and not
		 * the size */
		return negotiate_video_with_unsupported_depth (actor, rundepth, noevent, forced);
	else
		return negotiate_video (actor, noevent);

	return -VISUAL_ERROR_IMPOSSIBLE;
}

static int negotiate_video_with_unsupported_depth (VisActor *actor, int rundepth, int noevent, int forced)
{
	VisActorPlugin *actplugin = get_actor_plugin (actor);
	int depthflag = visual_actor_get_supported_depth (actor);
	
	/* Depth transform enviroment, it automaticly
	 * fits size because it can use the pitch from
	 * the dest video context */
	actor->transform = visual_video_new ();

	visual_log (VISUAL_LOG_INFO, "run depth %d forced %d\n", rundepth, forced);

	if (forced == TRUE)
		visual_video_set_depth (actor->transform, rundepth);
	else
		visual_video_set_depth (actor->transform,
				visual_video_depth_get_highest_nogl (depthflag));

	visual_log (VISUAL_LOG_INFO, "transpitch1 %d depth %d bpp %d", actor->transform->pitch, actor->transform->depth,
			actor->transform->bpp);
	/* If there is only GL (which gets returned by highest nogl if
	 * nothing else is there, stop here */
	if (actor->transform->depth == VISUAL_VIDEO_DEPTH_GL)
		return -VISUAL_ERROR_ACTOR_GL_NEGOTIATE;

	visual_video_set_dimension (actor->transform, actor->video->width, actor->video->height);
	visual_log (VISUAL_LOG_INFO, "transpitch2 %d %d", actor->transform->width, actor->transform->pitch);

	actplugin->requisition (visual_actor_get_plugin (actor), &actor->transform->width, &actor->transform->height);
	visual_log (VISUAL_LOG_INFO, "transpitch3 %d", actor->transform->pitch);

	if (noevent == FALSE) {
		visual_event_queue_add_resize (&actor->plugin->eventqueue, actor->transform,
				actor->transform->width, actor->transform->height);
		visual_plugin_events_pump (actor->plugin);
	} else {
		/* Normally a visual_video_set_dimension get's called within the
		 * event handler, but we won't come there right now so we've
		 * got to set the pitch ourself */
		visual_video_set_dimension (actor->transform,
				actor->transform->width, actor->transform->height);
	}

	visual_log (VISUAL_LOG_INFO, "rundepth: %d transpitch %d\n", rundepth, actor->transform->pitch);
	visual_video_allocate_buffer (actor->transform);

	if (actor->video->depth == VISUAL_VIDEO_DEPTH_8BIT)
		actor->ditherpal = visual_palette_new (256);

	return VISUAL_OK;
}

static int negotiate_video (VisActor *actor, int noevent)
{
	VisActorPlugin *actplugin = get_actor_plugin (actor);
	int tmpwidth, tmpheight, tmppitch;

	tmpwidth = actor->video->width;
	tmpheight = actor->video->height;
	tmppitch = actor->video->pitch;

	/* Pump the resize events and handle all the pending events */
	actplugin->requisition (visual_actor_get_plugin (actor), &actor->video->width, &actor->video->height);

	if (noevent == FALSE) {
		visual_event_queue_add_resize (&actor->plugin->eventqueue, actor->video,
				actor->video->width, actor->video->height);

		visual_plugin_events_pump (actor->plugin);
	}

	/* Size fitting enviroment */
	if (tmpwidth != actor->video->width || tmpheight != actor->video->height) {
		actor->fitting = visual_video_new_with_buffer (actor->video->width,
				actor->video->height, actor->video->depth);

		visual_video_set_dimension (actor->video, tmpwidth, tmpheight);
	}

	/* Set the pitch seen this is the framebuffer context */
	visual_video_set_pitch (actor->video, tmppitch);

	return VISUAL_OK;
}

/**
 * Gives the by the plugin natively supported depths
 *
 * @param actor Pointer to a VisActor of which the supported depth of it's
 * 	  encapsulated plugin is requested.
 *
 * @return an OR value of the VISUAL_VIDEO_DEPTH_* values which can be checked against using AND on succes,
 * 	-VISUAL_ERROR_ACTOR_NULL, -VISUAL_ERROR_PLUGIN_NULL or -VISUAL_ERROR_ACTOR_PLUGIN_NULL on failure.
 */
int visual_actor_get_supported_depth (VisActor *actor)
{
	VisActorPlugin *actplugin;

	visual_log_return_val_if_fail (actor != NULL, -VISUAL_ERROR_ACTOR_NULL);
	visual_log_return_val_if_fail (actor->plugin != NULL, -VISUAL_ERROR_PLUGIN_NULL);

	actplugin = get_actor_plugin (actor);

	if (actplugin == NULL)
		return -VISUAL_ERROR_ACTOR_PLUGIN_NULL;

	return actplugin->depth;
}

/**
 * Used to connect the target display it's VisVideo structure to the VisActor.
 *
 * Using the visual_video methods the screenbuffer, it's depth and dimension and optionally it's pitch
 * can be set so the actor plugins know about their graphical environment and have a place to draw.
 *
 * After this function it's most likely that visual_actor_video_negotiate needs to be called.
 *
 * @see visual_video_new
 * @see visual_actor_video_negotiate
 * 
 * @param actor Pointer to a VisActor to which the VisVideo needs to be set.
 * @param video Pointer to a VisVideo which contains information about the target display and the pointer
 * 	  to it's screenbuffer.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_ACTOR_NULL on failure.
 */
int visual_actor_set_video (VisActor *actor, VisVideo *video)
{
	visual_log_return_val_if_fail (actor != NULL, -VISUAL_ERROR_ACTOR_NULL);

	actor->video = video;

	return VISUAL_OK;
}

/**
 * This is called to run a VisActor. It also pump it's events when needed, checks for new song events and also does the fitting 
 * and depth transformation actions when needed.
 *
 * Every run cycle one frame is created, so this function needs to be used in the main draw loop of the application.
 *
 * @param actor Pointer to a VisActor that needs to be runned.
 * @param audio Pointer to a VisAudio that contains all the audio data.
 *
 * return VISUAL_OK on succes, -VISUAL_ERROR_ACTOR_NULL, -VISUAL_ERROR_ACTOR_VIDEO_NULL, -VISUAL_ERROR_NULL or
 * 	-VISUAL_ERROR_ACTOR_PLUGIN_NULL on failure.
 */
int visual_actor_run (VisActor *actor, VisAudio *audio)
{
	VisActorPlugin *actplugin;
	VisPluginData *plugin;
	VisVideo *video;
	VisVideo *transform;
	VisVideo *fitting;

	/* We don't check for video, because we don't always need a video */
	/*
	 * Really? take a look at visual_video_set_palette bellow
	 */
	visual_log_return_val_if_fail (actor != NULL, -VISUAL_ERROR_ACTOR_NULL);
	visual_log_return_val_if_fail (actor->video != NULL, -VISUAL_ERROR_ACTOR_VIDEO_NULL);
	visual_log_return_val_if_fail (audio != NULL, -VISUAL_ERROR_NULL);

	actplugin = get_actor_plugin (actor);
	plugin = visual_actor_get_plugin (actor);

	if (actplugin == NULL) {
		visual_log (VISUAL_LOG_CRITICAL,
			"The given actor does not reference any actor plugin");

		return -VISUAL_ERROR_ACTOR_PLUGIN_NULL;
	}

	/* Songinfo handling */
	if (visual_songinfo_compare (&actor->songcompare, &actplugin->songinfo) == FALSE) {
		visual_songinfo_mark (&actplugin->songinfo);
		
		visual_event_queue_add_newsong (
			visual_plugin_get_eventqueue (plugin),
			&actplugin->songinfo);

		visual_songinfo_free_strings (&actor->songcompare);
		visual_songinfo_copy (&actor->songcompare, &actplugin->songinfo);
	}

	video = actor->video;
	transform = actor->transform;
	fitting = actor->fitting;

	/*
	 * This needs to happen before palette, render stuff, always, period.
	 * Also internal vars can be initialized when params have been set in init on the param
	 * events in the event loop.
	 */
	visual_plugin_events_pump (actor->plugin);
	
	visual_video_set_palette (video, visual_actor_get_palette (actor));
	
	/* Set the palette to the target video */
	video->pal = visual_actor_get_palette (actor);

	/* Yeah some transformation magic is going on here when needed */
	if (transform != NULL && (transform->depth != video->depth)) {
		actplugin->render (plugin, transform, audio);

		if (transform->depth == VISUAL_VIDEO_DEPTH_8BIT) {
			visual_video_set_palette (transform, visual_actor_get_palette (actor));
			visual_video_depth_transform (video, transform);
		} else {
			visual_video_set_palette (transform, actor->ditherpal);
			visual_video_depth_transform (video, transform);
		}
	} else {
		if (fitting != NULL && (fitting->width != video->width || fitting->height != video->height)) {
			actplugin->render (plugin, fitting, audio);
			visual_video_blit_overlay (video, fitting, 0, 0, FALSE);
		} else {
			actplugin->render (plugin, video, audio);
		}
	}

	return VISUAL_OK;
}

/**
 * @}
 */

