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

/*
 * lvconfig.h must be included in order to show correct log messages
 */
#include "lvconfig.h"
#include "lv_log.h"

#include "lv_list.h"
#include "lv_input.h"
#include "lv_actor.h"
#include "lv_bin.h"
#include "lv_bin.h"

/* WARNING: Utterly shit ahead, i've screwed up on this and i need to
 * rewrite it. And i can't say i feel like it at the moment so be
 * patient :) */

static int bin_dtor (VisObject *object);

static void fix_depth_with_bin (VisBin *bin, VisVideo *video, int depth);
static int bin_get_depth_using_preferred (VisBin *bin, int depthflag);

static int bin_dtor (VisObject *object)
{
	VisBin *bin = VISUAL_BIN (object);

	visual_log_return_val_if_fail (bin != NULL, -1);

	if (bin->actor != NULL)
		visual_object_unref (VISUAL_OBJECT (bin->actor));

	if (bin->input != NULL)
		visual_object_unref (VISUAL_OBJECT (bin->input));

	if (bin->morph != NULL)
		visual_object_unref (VISUAL_OBJECT (bin->morph));

	if (bin->actmorphmanaged == TRUE) {
		if (bin->actmorph != NULL)
			visual_object_unref (VISUAL_OBJECT (bin->actmorph));

		if (bin->actmorphvideo != NULL)
			visual_object_unref (VISUAL_OBJECT (bin->actmorphvideo));
	}

	if (bin->privvid != NULL)
		visual_object_unref (VISUAL_OBJECT (bin->privvid));

	bin->actor = NULL;
	bin->input = NULL;
	bin->morph = NULL;
	bin->actmorph = NULL;
	bin->actmorphvideo = NULL;
	bin->privvid = NULL;

	return VISUAL_OK;
}

static void fix_depth_with_bin (VisBin *bin, VisVideo *video, int depth)
{
	/* Is supported within bin natively */
	if ((bin->depthflag & depth) > 0) {
		visual_video_set_depth (video, depth);
	} else {
		/* Not supported by the bin, taking the highest depth from the bin */
		visual_video_set_depth (video,
				visual_video_depth_get_highest_nogl (bin->depthflag));
	}
}

static int bin_get_depth_using_preferred (VisBin *bin, int depthflag)
{
	if (bin->depthpreferred == VISUAL_BIN_DEPTH_LOWEST)
		return visual_video_depth_get_lowest (depthflag);
	else
		return visual_video_depth_get_highest (depthflag);
}

/**
 * @defgroup VisBin VisBin
 * @{
 */

VisBin *visual_bin_new ()
{
	VisBin *bin;

	bin = visual_mem_new0 (VisBin, 1);

	/* VisObject stuff.. */
	visual_object_initialize (VISUAL_OBJECT (bin), TRUE, bin_dtor);

	bin->morphautomatic = TRUE;

	bin->morphmode = VISUAL_MORPH_MODE_TIME;
	visual_time_set (&bin->morphtime, 4, 0);
	
	bin->depthpreferred = VISUAL_BIN_DEPTH_HIGHEST;

	return bin;
}

int visual_bin_realize (VisBin *bin)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	if (bin->actor != NULL)
		visual_actor_realize (bin->actor);

	if (bin->input != NULL)
		visual_input_realize (bin->input);

	if (bin->morph != NULL)
		visual_morph_realize (bin->morph);

	return 0;
}

int visual_bin_set_actor (VisBin *bin, VisActor *actor)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->actor = actor;

	bin->managed = FALSE;

	return 0;
}

VisActor *visual_bin_get_actor (VisBin *bin)
{
	visual_log_return_val_if_fail (bin != NULL, NULL);

	return bin->actor;
}

int visual_bin_set_input (VisBin *bin, VisInput *input)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->input = input;

	bin->inputmanaged = FALSE;

	return 0;
}

VisInput *visual_bin_get_input (VisBin *bin)
{
	visual_log_return_val_if_fail (bin != NULL, NULL);

	return bin->input;
}

int visual_bin_set_morph (VisBin *bin, VisMorph *morph)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->morph = morph;

	bin->morphmanaged = FALSE;

	return 0;
}

int visual_bin_set_morph_by_name (VisBin *bin, char *morphname)
{
	VisMorph *morph;
	int depthflag;

	visual_log_return_val_if_fail (bin != NULL, -1);

	if (bin->morph != NULL)
		visual_object_unref (VISUAL_OBJECT (bin->morph));

	morph = visual_morph_new (morphname);

	bin->morph = morph;
	bin->morphmanaged = TRUE;
	
	visual_log_return_val_if_fail (morph->plugin != NULL, -1);

	depthflag = visual_morph_get_supported_depth (morph);

	if (visual_video_depth_is_supported (depthflag, bin->actvideo->depth) <= 0) {
		visual_object_unref (VISUAL_OBJECT (morph));
		bin->morph = NULL;

		return -2;
	}
	
	return 0;
}

VisMorph *visual_bin_get_morph (VisBin *bin)
{
	visual_log_return_val_if_fail (bin != NULL, NULL);

	return bin->morph;
}

int visual_bin_connect (VisBin *bin, VisActor *actor, VisInput *input)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	visual_bin_set_actor (bin, actor);
	visual_bin_set_input (bin, input);

	return 0;
}

int visual_bin_connect_by_names (VisBin *bin, char *actname, char *inname)
{
	VisActor *actor;
	VisInput *input;
	int depthflag;
	int depth;

	visual_log_return_val_if_fail (bin != NULL, -1);

	/* Create the actor */
	actor = visual_actor_new (actname);
	visual_log_return_val_if_fail (actor != NULL, -1);

	/* Check and set required depth */
	depthflag = visual_actor_get_supported_depth (actor);

	/* GL plugin, and ONLY a GL plugin */
	if (depthflag == VISUAL_VIDEO_DEPTH_GL)
		visual_bin_set_depth (bin, VISUAL_VIDEO_DEPTH_GL);
	else {
		depth = bin_get_depth_using_preferred (bin, depthflag);

		/* Is supported within bin natively */
		if ((bin->depthflag & depth) > 0) {
			visual_bin_set_depth (bin, depth);
		} else {
			/* Not supported by the bin, taking the highest depth from the bin */
			visual_bin_set_depth (bin,
				visual_video_depth_get_highest_nogl (bin->depthflag));
		}
	}

	/* Initialize the managed depth */
	bin->depthforcedmain = bin->depth;

	/* Create the input */
	input = visual_input_new (inname);
	visual_log_return_val_if_fail (input != NULL, -1);

	/* Connect */
	visual_bin_connect (bin, actor, input);

	bin->managed = TRUE;
	bin->inputmanaged = TRUE;

	return 0;
}

int visual_bin_sync (VisBin *bin, int noevent)
{
	VisVideo *video;
	VisVideo *actvideo;

	visual_log_return_val_if_fail (bin != NULL, -1);

	visual_log (VISUAL_LOG_DEBUG, "starting sync");

	/* Sync the actor regarding morph */
	if (bin->morphing == TRUE && bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			bin->actvideo->depth != VISUAL_VIDEO_DEPTH_GL &&
			bin->depthfromGL != TRUE) {
		visual_morph_set_video (bin->morph, bin->actvideo);
	
		video = bin->privvid;
		if (video == NULL) {
			visual_log (VISUAL_LOG_DEBUG, "Private video data NULL");
			return -1;
		}

		visual_video_free_buffer (video);
		visual_video_clone (video, bin->actvideo);

		visual_log (VISUAL_LOG_DEBUG, "pitches actvideo %d, new video %d", bin->actvideo->pitch, video->pitch);

		visual_log (VISUAL_LOG_DEBUG, "phase1 bin->privvid %p", bin->privvid);
		if (bin->actmorph->video->depth == VISUAL_VIDEO_DEPTH_GL) {
			visual_video_set_buffer (video, NULL);
			video = bin->actvideo;
		} else
			visual_video_allocate_buffer (video);
		
		visual_log (VISUAL_LOG_DEBUG, "phase2");
	} else {
		video = bin->actvideo;
		if (video == NULL) {
			visual_log (VISUAL_LOG_DEBUG, "Actor video is NULL");
			return -1;
		}

		visual_log (VISUAL_LOG_DEBUG, "setting new video from actvideo %d %d", video->depth, video->bpp);
	}

	/* Main actor */
//	visual_actor_realize (bin->actor);
	visual_actor_set_video (bin->actor, video);

	visual_log (VISUAL_LOG_DEBUG, "one last video pitch check %d depth old %d forcedmain %d noevent %d",
			video->pitch, bin->depthold,
			bin->depthforcedmain, noevent);

	if (bin->managed == TRUE) {
		if (bin->depthold == VISUAL_VIDEO_DEPTH_GL)
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, noevent, TRUE);
	} else {
		if (bin->depthold == VISUAL_VIDEO_DEPTH_GL)
			visual_actor_video_negotiate (bin->actor, 0, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actor, 0, noevent, FALSE);
	}
	
	visual_log (VISUAL_LOG_DEBUG, "pitch after main actor negotiate %d", video->pitch);

	/* Morphing actor */
	if (bin->actmorphmanaged == TRUE && bin->morphing == TRUE &&
			bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH) {

		actvideo = bin->actmorphvideo;
		if (actvideo == NULL) {
			visual_log (VISUAL_LOG_DEBUG, "Morph video is NULL");
			return -1;
		}

		visual_video_free_buffer (actvideo);

		visual_video_clone (actvideo, video);

		if (bin->actor->video->depth != VISUAL_VIDEO_DEPTH_GL)
			visual_video_allocate_buffer (actvideo);

		visual_actor_realize (bin->actmorph);

		visual_log (VISUAL_LOG_DEBUG, "phase3 pitch of real framebuffer %d", bin->actvideo->pitch);
		if (bin->actmorphmanaged == TRUE)
			visual_actor_video_negotiate (bin->actmorph, bin->depthforced, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actmorph, 0, FALSE, FALSE);
	}

	visual_log (VISUAL_LOG_DEBUG, "end sync function");

	return 0;
}

int visual_bin_set_video (VisBin *bin, VisVideo *video)
{
	visual_log_return_val_if_fail (bin != NULL, -1);
	
	bin->actvideo = video;

	return 0;
}

int visual_bin_set_supported_depth (VisBin *bin, int depthflag)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->depthflag = depthflag;

	return 0;
}

int visual_bin_set_preferred_depth (VisBin *bin, VisBinDepth depthpreferred)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->depthpreferred = depthpreferred;

	return 0;
}

int visual_bin_set_depth (VisBin *bin, int depth)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->depthold = bin->depth;

	if (visual_video_depth_is_supported (bin->depthflag, depth) != TRUE)
		return -2;

	visual_log (VISUAL_LOG_DEBUG, "old: %d new: %d", bin->depth, depth);
	if (bin->depth != depth)
		bin->depthchanged = TRUE;

	if (bin->depth == VISUAL_VIDEO_DEPTH_GL && bin->depthchanged == TRUE)
		bin->depthfromGL = TRUE;
	else
		bin->depthfromGL = FALSE;

	bin->depth = depth;

	visual_video_set_depth (bin->actvideo, depth);

	return 0;
}

int visual_bin_get_depth (VisBin *bin)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	return bin->depth;
}

int visual_bin_depth_changed (VisBin *bin)
{
	visual_log_return_val_if_fail (bin != NULL, -1);
	
	if (bin->depthchanged == FALSE)
		return FALSE;

	bin->depthchanged = FALSE;

	return TRUE;
}

VisPalette *visual_bin_get_palette (VisBin *bin)
{
	visual_log_return_val_if_fail (bin != NULL, NULL);

	if (bin->morphing == TRUE)
		return visual_morph_get_palette (bin->morph);
	else
		return visual_actor_get_palette (bin->actor);
}

int visual_bin_switch_actor_by_name (VisBin *bin, char *actname)
{
	VisActor *actor;
	VisVideo *video;
	int depthflag;
	int depth;

	visual_log_return_val_if_fail (bin != NULL, -1);
	visual_log_return_val_if_fail (actname != NULL, -1);

	visual_log (VISUAL_LOG_DEBUG, "switching to a new actor: %s, old actor: %s", actname, bin->actor->plugin->info->name);

	/* Destroy if there already is a managed one */
	if (bin->actmorphmanaged == TRUE) {
		if (bin->actmorph != NULL) {
			visual_object_unref (VISUAL_OBJECT (bin->actmorph));

			if (bin->actmorphvideo != NULL)
				visual_object_unref (VISUAL_OBJECT (bin->actmorphvideo));
		}
	}

	/* Create a new managed actor */
	actor = visual_actor_new (actname);
	visual_log_return_val_if_fail (actor != NULL, -1);

	video = visual_video_new ();

	visual_video_clone (video, bin->actvideo);

	depthflag = visual_actor_get_supported_depth (actor);
	if (visual_video_depth_is_supported (depthflag, VISUAL_VIDEO_DEPTH_GL) == TRUE) {
		visual_log (VISUAL_LOG_INFO, "Switching to Gl mode");

		bin->depthforced = VISUAL_VIDEO_DEPTH_GL;
		bin->depthforcedmain = VISUAL_VIDEO_DEPTH_GL;
	
		visual_video_set_depth (video, VISUAL_VIDEO_DEPTH_GL);

		visual_bin_set_depth (bin, VISUAL_VIDEO_DEPTH_GL);
		bin->depthchanged = TRUE;

	} else {
		visual_log (VISUAL_LOG_INFO, "Switching away from Gl mode -- or non Gl switch");

		
		/* Switching from GL */
		depth = bin_get_depth_using_preferred (bin, depthflag);

		fix_depth_with_bin (bin, video, depth);

		visual_log (VISUAL_LOG_DEBUG, "after depth fixating");
		
		/* After a depth change, the pitch value needs an update from the client
		 * if it's different from width * bpp, after a visual_bin_sync
		 * the issues are fixed again */
		visual_log (VISUAL_LOG_INFO, "video depth (from fixate): %d", video->depth);

		/* FIXME check if there are any unneeded depth transform environments and drop these */
		visual_log (VISUAL_LOG_DEBUG, "checking if we need to drop something: depthforcedmain: %d actvideo->depth %d",
				bin->depthforcedmain, bin->actvideo->depth);

		/* Drop a transformation environment when not needed */
		if (bin->depthforcedmain != bin->actvideo->depth) {
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, TRUE, TRUE);
			visual_log (VISUAL_LOG_DEBUG, "[[[[optionally a bogus transform environment, dropping]]]]\n");
		}

		if (bin->actvideo->depth > video->depth
				&& bin->actvideo->depth != VISUAL_VIDEO_DEPTH_GL
				&& bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH) {

			visual_log (VISUAL_LOG_INFO, "old depth is higher, video depth %d, depth %d, bin depth %d", video->depth, depth,
					bin->depth);
			
			bin->depthforced = depth;
			bin->depthforcedmain = bin->depth;
			
			visual_bin_set_depth (bin, bin->actvideo->depth);

			visual_video_set_depth (video, bin->actvideo->depth);

		} else if (bin->actvideo->depth != VISUAL_VIDEO_DEPTH_GL) {

			visual_log (VISUAL_LOG_INFO, "new depth is higher, or equal: video depth %d, depth %d bin depth %d", video->depth, depth,
					bin->depth);

			visual_log (VISUAL_LOG_DEBUG, "depths i can locate: actvideo: %d   bin: %d   bin-old: %d", bin->actvideo->depth,
					bin->depth, bin->depthold);
			
			bin->depthforced = video->depth;
			bin->depthforcedmain = bin->depth;

			visual_log (VISUAL_LOG_DEBUG, "depthforcedmain in switch by name: %d", bin->depthforcedmain);
			visual_log (VISUAL_LOG_DEBUG, "visual_bin_set_depth %d", video->depth);
			visual_bin_set_depth (bin, video->depth);

		} else {
			/* Don't force ourself into a GL depth, seen we do a direct
			 * switch in the run */
			bin->depthforced = video->depth;
			bin->depthforcedmain = video->depth;
			
			visual_log (VISUAL_LOG_INFO, "Switching from Gl TO framebuffer for real, framebuffer depth: %d", video->depth);
		}

		visual_log (VISUAL_LOG_INFO, "Target depth selected: %d", depth);
		visual_video_set_dimension (video, video->width, video->height);

		visual_log (VISUAL_LOG_INFO, "Switch to new pitch: %d", bin->actvideo->pitch);
		if (bin->actvideo->depth != VISUAL_VIDEO_DEPTH_GL)
			visual_video_set_pitch (video, bin->actvideo->pitch);
		
		visual_log (VISUAL_LOG_DEBUG, "before allocating buffer");
		visual_video_allocate_buffer (video);
		visual_log (VISUAL_LOG_DEBUG, "after allocating buffer");
	}

	visual_log (VISUAL_LOG_INFO, "video pitch of that what connects to the new actor %d", video->pitch);
	visual_actor_set_video (actor, video);

	bin->actmorphvideo = video;
	bin->actmorphmanaged = TRUE;

	visual_log (VISUAL_LOG_INFO, "switching... ******************************************");
	visual_bin_switch_actor (bin, actor);

	visual_log (VISUAL_LOG_INFO, "end switch actor by name function ******************");
	return 0;
}

int visual_bin_switch_actor (VisBin *bin, VisActor *actor)
{
	VisVideo *privvid;

	visual_log_return_val_if_fail (bin != NULL, -1);
	visual_log_return_val_if_fail (actor != NULL, -1);

	/* Set the new actor */
	bin->actmorph = actor;

	visual_log (VISUAL_LOG_DEBUG, "entering...");
	
	/* Free the private video */
	if (bin->privvid != NULL) {
		visual_object_unref (VISUAL_OBJECT (bin->privvid));
		
		bin->privvid = NULL;
	}

	visual_log (VISUAL_LOG_INFO, "depth of the main actor: %d", bin->actor->video->depth);

	/* Starting the morph, but first check if we don't have anything todo with openGL */
	if (bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			bin->actor->video->depth != VISUAL_VIDEO_DEPTH_GL &&
			bin->actmorph->video->depth != VISUAL_VIDEO_DEPTH_GL &&
			bin->depthfromGL != TRUE) {

		if (bin->morph != NULL && bin->morph->plugin != NULL) {
			visual_morph_set_rate (bin->morph, 0);
		
			visual_morph_set_video (bin->morph, bin->actvideo);

			if (bin->morphautomatic == TRUE)
				visual_morph_set_mode (bin->morph, bin->morphmode);
			else
				visual_morph_set_mode (bin->morph, VISUAL_MORPH_MODE_SET);
			
			visual_morph_set_time (bin->morph, &bin->morphtime);
			visual_morph_set_steps (bin->morph, bin->morphsteps);
		}

		bin->morphrate = 0;
		bin->morphstepsdone = 0;

		visual_log (VISUAL_LOG_DEBUG, "phase 1");
		/* Allocate a private video for the main actor, so the morph
		 * can draw to the framebuffer */
		privvid = visual_video_new ();

		visual_log (VISUAL_LOG_DEBUG, "actvideo->depth %d actmorph->video->depth %d",
				bin->actvideo->depth, bin->actmorph->video->depth);

		visual_log (VISUAL_LOG_DEBUG, "phase 2");
		visual_video_clone (privvid, bin->actvideo);
		visual_log (VISUAL_LOG_DEBUG, "phase 3 pitch privvid %d actvideo %d", privvid->pitch, bin->actvideo->pitch);

		visual_video_allocate_buffer (privvid);

		visual_log (VISUAL_LOG_DEBUG, "phase 4");
		/* Initial privvid initialize */
	
		visual_log (VISUAL_LOG_DEBUG, "actmorph->video->depth %d %p", bin->actmorph->video->depth,
				bin->actvideo->pixels);
		
		if (bin->actvideo->pixels != NULL && privvid->pixels != NULL)
			visual_mem_copy (privvid->pixels, bin->actvideo->pixels, privvid->size);
		else if (privvid->pixels != NULL)
			memset (privvid->pixels, 0, privvid->size);

		visual_actor_set_video (bin->actor, privvid);
		bin->privvid = privvid;
	} else {
		visual_log (VISUAL_LOG_DEBUG, "Pointer actvideo->pixels %p", bin->actvideo->pixels);
		if (bin->actor->video->depth != VISUAL_VIDEO_DEPTH_GL &&
				bin->actvideo->pixels != NULL) {
			memset (bin->actvideo->pixels, 0, bin->actvideo->size);
		}
	}

	visual_log (VISUAL_LOG_DEBUG, "Leaving, actor->video->depth: %d actmorph->video->depth: %d",
			bin->actor->video->depth, bin->actmorph->video->depth);

	bin->morphing = TRUE;

	return 0;
}

int visual_bin_switch_finalize (VisBin *bin)
{
	int depthflag;

	visual_log_return_val_if_fail (bin != NULL, -1);

	visual_log (VISUAL_LOG_DEBUG, "Entering...");
	if (bin->managed == TRUE)
		visual_object_unref (VISUAL_OBJECT (bin->actor));

	/* Copy over the depth to be sure, and for GL plugins */
//	bin->actvideo->depth = bin->actmorphvideo->depth;
//	visual_video_set_depth (bin->actvideo, bin->actmorphvideo->depth);

	if (bin->actmorphmanaged == TRUE) {
		visual_object_unref (VISUAL_OBJECT (bin->actmorphvideo));

		bin->actmorphvideo = NULL;
	}

	if (bin->privvid != NULL) {
		visual_object_unref (VISUAL_OBJECT (bin->privvid));
		
		bin->privvid = NULL;
	}

	bin->actor = bin->actmorph;
	bin->actmorph = NULL;
	
	visual_actor_set_video (bin->actor, bin->actvideo);
	
	bin->morphing = FALSE;

	if (bin->morphmanaged == TRUE) {
		visual_object_unref (VISUAL_OBJECT (bin->morph));
		bin->morph = NULL;
	}

	visual_log (VISUAL_LOG_DEBUG, " - in finalize - fscking depth from actvideo: %d %d", bin->actvideo->depth, bin->actvideo->bpp);

	
//	visual_bin_set_depth (bin, bin->actvideo->depth);

	depthflag = visual_actor_get_supported_depth (bin->actor);
	fix_depth_with_bin (bin, bin->actvideo, bin_get_depth_using_preferred (bin, depthflag));
	visual_bin_set_depth (bin, bin->actvideo->depth);

	bin->depthforcedmain = bin->actvideo->depth;
	visual_log (VISUAL_LOG_DEBUG, "bin->depthforcedmain in finalize %d", bin->depthforcedmain);

	// FIXME replace with a depth fixer
	if (bin->depthchanged == TRUE) {
		visual_log (VISUAL_LOG_INFO, "negotiate without event");
		visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, TRUE, TRUE);
		visual_log (VISUAL_LOG_INFO, "end negotiate without event");
	//	visual_bin_sync (bin);
	}

	visual_log (VISUAL_LOG_DEBUG, "Leaving...");

	return 0;
}

int visual_bin_switch_set_style (VisBin *bin, VisBinSwitchStyle style)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->morphstyle = style;

	return 0;
}

int visual_bin_switch_set_steps (VisBin *bin, int steps)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->morphsteps = steps;

	return 0;
}

int visual_bin_switch_set_automatic (VisBin *bin, int automatic)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->morphautomatic = automatic;

	return 0;
}

int visual_bin_switch_set_rate (VisBin *bin, float rate)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->morphrate = rate;

	return 0;
}

int visual_bin_switch_set_mode (VisBin *bin, VisMorphMode mode)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	bin->morphmode = mode;

	return 0;
}

int visual_bin_switch_set_time (VisBin *bin, long sec, long usec)
{
	visual_log_return_val_if_fail (bin != NULL, -1);

	visual_time_set (&bin->morphtime, sec, usec);

	return 0;
}


int visual_bin_run (VisBin *bin)
{
	visual_log_return_val_if_fail (bin != NULL, -1);
	visual_log_return_val_if_fail (bin->actor != NULL, -1);
	visual_log_return_val_if_fail (bin->input != NULL, -1);

	visual_input_run (bin->input);

	/* If we have a direct switch, do this BEFORE we run the actor,
	 * else we can get into trouble especially with GL, also when
	 * switching away from a GL plugin this is needed */
	if (bin->morphing == TRUE) {
		/* We realize here, because it doesn't realize
		 * on switch, the reason for this is so that after a
		 * switch call, especially in a managed bin the
		 * depth can be requested and set, this is important
		 * for openGL plugins, the realize method checks
		 * for double realize itself so we don't have
		 * to check this, it's a bit hacky */
		visual_log_return_val_if_fail (bin->actmorph != NULL, -1);
		visual_log_return_val_if_fail (bin->actmorph->plugin != NULL, -1);
 		if (bin->actmorph->plugin->realized == FALSE) {
			visual_actor_realize (bin->actmorph);
			
			if (bin->actmorphmanaged == TRUE)
				visual_actor_video_negotiate (bin->actmorph, bin->depthforced, FALSE, TRUE);
			else
				visual_actor_video_negotiate (bin->actmorph, 0, FALSE, FALSE);
		}

		/* When we've got multiple switch events without a sync we need
		 * to realize the main actor as well */
		visual_log_return_val_if_fail (bin->actor->plugin != NULL, -1);
		if (bin->actor->plugin->realized == FALSE) {
			visual_actor_realize (bin->actor);
			
			if (bin->managed == TRUE)
				visual_actor_video_negotiate (bin->actor, bin->depthforced, FALSE, TRUE);
			else
				visual_actor_video_negotiate (bin->actor, 0, FALSE, FALSE);
		}

		/* When the style is DIRECT or the context is GL we shouldn't try
		 * to morph and instead finalize at once */
		visual_log_return_val_if_fail (bin->actor->video != NULL, -1);
		if (bin->morphstyle == VISUAL_SWITCH_STYLE_DIRECT ||
			bin->actor->video->depth == VISUAL_VIDEO_DEPTH_GL) {
		
			visual_bin_switch_finalize (bin);

			/* We can't start drawing yet, the client needs to catch up with
			 * the depth change */
			return 0;
		}
	}

	/* We realize here because in a managed bin the depth for openGL is
	 * requested after the connect, thus we can realize there yet */
	visual_actor_realize (bin->actor);

	visual_actor_run (bin->actor, bin->input->audio);

	if (bin->morphing == TRUE) {
		visual_log_return_val_if_fail (bin->actmorph != NULL, -1);
		visual_log_return_val_if_fail (bin->actmorph->video != NULL, -1);
		visual_log_return_val_if_fail (bin->actor->video != NULL, -1);

		if (bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			bin->actmorph->video->depth != VISUAL_VIDEO_DEPTH_GL &&
			bin->actor->video->depth != VISUAL_VIDEO_DEPTH_GL) {

			visual_actor_run (bin->actmorph, bin->input->audio);

			if (bin->morph == NULL || bin->morph->plugin == NULL) {
				visual_bin_switch_finalize (bin);
		
				return 0;
			}

			/* Same goes for the morph, we realize it here for depth changes
			 * (especially the openGL case */
			visual_morph_realize (bin->morph);
			visual_morph_run (bin->morph, bin->input->audio, bin->actor->video, bin->actmorph->video);

			if (visual_morph_is_done (bin->morph) == TRUE)
				visual_bin_switch_finalize (bin);
		} else {
//			visual_bin_switch_finalize (bin);
		}
	}

	return 0;
}

/**
 * @}
 */

