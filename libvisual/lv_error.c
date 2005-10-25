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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "lv_log.h"
#include "lv_error.h"

static const char *__lv_error_human_readable[] = {
	"There was no error",							/* VISUAL_OK */

	"General error occurred",						/* VISUAL_ERROR_GENERAL */
	"General NULL pointer error",						/* VISUAL_ERROR_NULL */
	"An impossible event occurred",						/* VISUAL_ERROR_IMPOSSIBLE */

	"VisActor is NULL",							/* VISUAL_ERROR_ACTOR_NULL */
	"VisActor it's video is NULL",						/* VISUAL_ERROR_ACTOR_VIDEO_NULL */
	"VisActor it's plugin is NULL",						/* VISUAL_ERROR_ACTOR_PLUGIN_NULL */
	"VisActor failed while trying to forcefully negotiate a GL surface",	/* VISUAL_ERROR_ACTOR_GL_NEGOTIATE */

	"VisAudio is NULL",							/* VISUAL_ERROR_AUDIO_NULL */
	
	"Bitmap is not a bitmap file",						/* VISUAL_ERROR_BMP_NO_BMP */
	"Bitmap can not be found",						/* VISUAL_ERROR_BMP_NOT_FOUND */
	"Bitmap is not supported",						/* VISUAL_ERROR_BMP_NOT_SUPPORTED */
	"Bitmap is corrupted",							/* VISUAL_ERROR_BMP_CORRUPTED */

	"VisColor is NULL",							/* VISUAL_ERROR_COLOR_NULL */
	
	"The code can not run on this architecture",				/* VISUAL_ERROR_CPU_INVALID_CODE */
	
	"Global error handler is NULL",						/* VISUAL_ERROR_ERROR_HANDLER_NULL */
	
	"VisEvent is NULL",							/* VISUAL_ERROR_EVENT_NULL */
	"VisEventQueue is NULL",						/* VISUAL_ERROR_EVENT_QUEUE_NULL */
	
	"VisInput is NULL",							/* VISUAL_ERROR_INPUT_NULL */
	"VisInput it's plugin is NULL",						/* VISUAL_ERROR_INPUT_PLUGIN_NULL */
	
	"No paths were given to seek for plugins",				/* VISUAL_ERROR_LIBVISUAL_NO_PATHS */
	"Libvisual is already initialized",					/* VISUAL_ERROR_LIBVISUAL_ALREADY_INITIALIZED */
	"Libvisual is not initialized",						/* VISUAL_ERROR_LIBVISUAL_NOT_INITIALIZED */
	"Libvisual has not build a plugin registry",				/* VISUAL_ERROR_LIBVISUAL_NO_REGISTRY */
	
	"VisList is NULL",							/* VISUAL_ERROR_LIST_NULL */
	"VisListEntry is NULL",							/* VISUAL_ERROR_LIST_ENTRY_NULL */
	"VisListEntry is invalid",						/* VISUAL_ERROR_LIST_ENTRY_INVALID */
	
	"Given memory pointer is NULL",						/* VISUAL_ERROR_MEM_NULL */
	
	"VisMorph is NULL",							/* VISUAL_ERROR_MORPH_NULL */
	"VisMorph it's plugin is NULL",						/* VISUAL_ERROR_MORPH_PLUGIN_NULL */
	
	"VisPalette is NULL",							/* VISUAL_ERROR_PALETTE_NULL */
	"VisPalette it's size conflicts",					/* VISUAL_ERROR_PALETTE_SIZE */
	
	"VisParamEntry is NULL",						/* VISUAL_ERROR_PARAM_NULL */
	"VisParamContainer is NULL",						/* VISUAL_ERROR_PARAM_CONTAINER_NULL */
	"VisParamEntry not found in VisParamContainer",				/* VISUAL_ERROR_PARAM_NOT_FOUND */
	"VisParamEntry it's change notify callback is NULL",			/* VISUAL_ERROR_PARAM_CALLBACK_NULL */
	"VisParamEntry contains too many change notify callbacks",		/* VISUAL_ERROR_PARAM_CALLBACK_TOO_MANY */
	"VisParamEntry is of invalid type",					/* VISUAL_ERROR_PARAM_INVALID_TYPE */
	
	"VisPluginData is NULL",						/* VISUAL_ERROR_PLUGIN_NULL */
	"VisPluginInfo is NULL",						/* VISUAL_ERROR_PLUGIN_INFO_NULL */
	"VisPluginRef is NULL",							/* VISUAL_ERROR_PLUGIN_REF_NULL */
	"VisPluginEnvironElement is NULL",					/* VISUAL_ERROR_PLUGIN_ENVIRON_NULL */
	"Plugin does not have an event handler",				/* VISUAL_ERROR_PLUGIN_NO_EVENT_HANDLER */
	"Plugin handle is NULL",						/* VISUAL_ERROR_PLUGIN_HANDLE_NULL */
	"Plugin is already realized",						/* VISUAL_ERROR_PLUGIN_ALREADY_REALIZED */
	
	"VisRandomContext is NULL",						/* VISUAL_ERROR_RANDOM_CONTEXT_NULL */
	
	"VisSongInfo is NULL",							/* VISUAL_ERROR_SONGINFO_NULL */

	"VisThread is NULL",							/* VISUAL_ERROR_THREAD_NULL */
	"Threading is disabled or not supported",				/* VISUAL_ERROR_THREAD_NO_THREADING */
	"VisMutex is NULL",							/* VISUAL_ERROR_MUTEX_NULL */
	"VisMutex lock failed",							/* VISUAL_ERROR_MUTEX_LOCK_FAILURE */
	"VisMutex trylock failed",						/* VISUAL_ERROR_MUTEX_TRYLOCK_FAILURE */
	"VisMutex unlock failed",						/* VISUAL_ERROR_MUTEX_UNLOCK_FAILURE */

	"VisTransform is NULL",							/* VISUAL_ERROR_TRANSFORM_NULL */
	"The VisTransform negotiate with the target VisVideo failed",		/* VISUAL_ERROR_TRANSFORM_NEGOTIATE */
	"The VisTransform it's plugin is NULL",					/* VISUAL_ERROR_TRANSFORM_PLUGIN_NULL */
	"The VisTransform it's video is NULL",					/* VISUAL_ERROR_TRANSFORM_VIDEO_NULL */
	"The VisTransform it's palette is NULL",				/* VISUAL_ERROR_TRANSFORM_PALETTE_NULL */

	"VisObject destruction failed",						/* VISUAL_ERROR_OBJECT_DTOR_FAILED */
	"VisObject is NULL",							/* VISUAL_ERROR_OBJECT_NULL */
	"VisObject is not allocated",						/* VISUAL_ERROR_OBJECT_NOT_ALLOCATED */
	
	"VisTime is NULL",							/* VISUAL_ERROR_TIME_NULL */
	"visual_time_usleep() is not supported",				/* VISUAL_ERROR_TIME_NO_USLEEP */
	"VisTimer is NULL",							/* VISUAL_ERROR_TIMER_NULL */
	
	"VisUIWidget is NULL",							/* VISUAL_ERROR_UI_WIDGET_NULL */
	"VisUIContainer is NULL",						/* VISUAL_ERROR_UI_CONTAINER_NULL */
	"VisUIBox is NULL",							/* VISUAL_ERROR_UI_BOX_NULL */
	"VisUITable is NULL",							/* VISUAL_ERROR_UI_TABLE_NULL */
	"VisUIFrame is NULL",							/* VISUAL_ERROR_UI_FRAME_NULL */
	"VisUILabel is NULL",							/* VISUAL_ERROR_UI_LABEL_NULL */
	"VisUIImage is NULL",							/* VISUAL_ERROR_UI_IMAGE_NULL */
	"VisUISeparator is NULL",						/* VISUAL_ERROR_UI_SEPARATOR_NULL */
	"VisUIMutator is NULL",							/* VISUAL_ERROR_UI_MUTATOR_NULL */
	"VisUIRange is NULL",							/* VISUAL_ERROR_UI_RANGE_NULL */
	"VisUIEntry is NULL",							/* VISUAL_ERROR_UI_ENTRY_NULL */
	"VisUISlider is NULL",							/* VISUAL_ERROR_UI_SLIDER_NULL */
	"VisUINumeric is NULL",							/* VISUAL_ERROR_UI_NUMERIC_NULL */
	"VisUIColor is NULL",							/* VISUAL_ERROR_UI_COLOR_NULL */
	"VisUIChoice is NULL",							/* VISUAL_ERROR_UI_CHOICE_NULL */
	"VisUIPopup is NULL",							/* VISUAL_ERROR_UI_POPUP_NULL */
	"VisUIList is NULL",							/* VISUAL_ERROR_UI_LIST_NULL */
	"VisUIRadio is NULL",							/* VISUAL_ERROR_UI_RADIO_NULL */
	"VisUICheckbox is NULL",						/* VISUAL_ERROR_UI_CHECKBOX_NULL */
	"VisUIChoiceEntry is NULL",						/* VISUAL_ERROR_UI_CHOICE_ENTRY_NULL */
	"No choice in VisUIChoice is activated",				/* VISUAL_ERROR_UI_CHOICE_NONE_ACTIVE */

	"VisVideo is NULL",							/* VISUAL_ERROR_VIDEO_NULL */
	"VisVideo has allocated pixel buffer",					/* VISUAL_ERROR_VIDEO_HAS_ALLOCATED */
	"VisVideo it's pixel buffer is NULL",					/* VISUAL_ERROR_VIDEO_PIXELS_NULL */
	"VisVideo it's pixel buffer is not allocated",				/* VISUAL_ERROR_VIDEO_NO_ALLOCATED */
	"VisVideo has pixel buffer",						/* VISUAL_ERROR_VIDEO_HAS_PIXELS */
	"VisVideo is of invalid bytes per pixel",				/* VISUAL_ERROR_VIDEO_INVALID_BPP */
	"VisVideo is of invalid depth",						/* VISUAL_ERROR_VIDEO_INVALID_DEPTH */
	"Invalid scale method given",						/* VISUAL_ERROR_VIDEO_INVALID_SCALE_METHOD */
	"Given coordinates are out of bounds",					/* VISUAL_ERROR_VIDEO_OUT_OF_BOUNDS */
	"Given VisVideos are not indentical",					/* VISUAL_ERROR_VIDEO_NOT_INDENTICAL */
	"VisVideo is not depth transformed as requested"			/* VISUAL_ERROR_VIDEO_NOT_TRANSFORMED */
};

static VisErrorHandlerFunc error_handler = NULL;
static void *error_handler_priv = NULL;

/**
 * @defgroup VisError VisError
 * @{
 */

/**
 * Raise a libvisual error. With the standard error handler this will
 * do a raise(SIGTRAP). You can set your own error handler function using the
 * visual_error_set_handler.
 *
 * @see visual_error_set_handler
 *
 * @return Returns the return value from the handler that is set.
 */
int visual_error_raise ()
{
	if (error_handler == NULL) {
		raise (SIGTRAP);
		exit (1);
	}
	
	return error_handler (error_handler_priv);
}

/**
 * Sets the error handler callback. By using this function you
 * can override libvisual it's default error handler.
 *
 * @param handler The error handler which you want to use
 *      to handle libvisual errors.
 * @param priv Optional private data which could be needed in the
 *      error handler that has been set.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_ERROR_HANDLER_NULL on failure.
 */
int visual_error_set_handler (VisErrorHandlerFunc handler, void *priv)
{
	visual_log_return_val_if_fail (handler != NULL, -VISUAL_ERROR_ERROR_HANDLER_NULL);

	error_handler = handler;
	error_handler_priv = priv;

	return VISUAL_OK;
}

/**
 * Translates an error into a human readable string, the returned string should not be freed.
 *
 * @param err Numeric error value.
 * 
 * @return Human readable string, or NULL on failure.
 */
const char *visual_error_to_string (int err)
{
	if (abs (err) >= VISUAL_ERROR_LIST_END)
		return "The error value given to visual_error_to_string() is invalid";

	return __lv_error_human_readable[abs (err)];
}

/**
 * @}
 */

