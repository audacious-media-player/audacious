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

#include "lv_list.h"
#include "lv_input.h"

extern VisList *__lv_plugins_input;

static int input_dtor (VisObject *object);

static VisInputPlugin *get_input_plugin (VisInput *input);

static int input_dtor (VisObject *object)
{
	VisInput *input = VISUAL_INPUT (object);

	if (input->plugin != NULL)
		visual_plugin_unload (input->plugin);

	if (input->audio != NULL)
		visual_object_unref (VISUAL_OBJECT (input->audio));

	input->plugin = NULL;
	input->audio = NULL;

	return VISUAL_OK;
}

static VisInputPlugin *get_input_plugin (VisInput *input)
{
	VisInputPlugin *inplugin;

	visual_log_return_val_if_fail (input != NULL, NULL);
	visual_log_return_val_if_fail (input->plugin != NULL, NULL);

	inplugin = VISUAL_PLUGIN_INPUT (input->plugin->info->plugin);

	return inplugin;
}

/**
 * @defgroup VisInput VisInput
 * @{
 */

/**
 * Gives the encapsulated VisPluginData from a VisInput.
 *
 * @param input Pointer of a VisInput of which the VisPluginData needs to be returned.
 *
 * @return VisPluginData that is encapsulated in the VisInput, possibly NULL.
 */
VisPluginData *visual_input_get_plugin (VisInput *input)
{
	        return input->plugin;
}

/**
 * Gives a list of input plugins in the current plugin registry.
 *
 * @return An VisList of VisPluginRef's containing the input plugins in the plugin registry.
 */
VisList *visual_input_get_list ()
{
	return __lv_plugins_input;
}

/**
 * Gives the next input plugin based on the name of a plugin.
 *
 * @see visual_input_get_prev_by_name
 *
 * @param name The name of the current plugin, or NULL to get the first.
 *
 * @return The name of the next plugin within the list.
 */
const char *visual_input_get_next_by_name (const char *name)
{
	return visual_plugin_get_next_by_name (visual_input_get_list (), name);
}

/**
 * Gives the previous input plugin based on the name of a plugin.
 *
 * @see visual_input_get_next_by_name
 *
 * @param name The name of the current plugin. or NULL to get the last.
 *
 * @return The name of the previous plugin within the list.
 */
const char *visual_input_get_prev_by_name (const char *name)
{
	return visual_plugin_get_prev_by_name (visual_input_get_list (), name);
}


/**
 * Checks if the input plugin is in the registry, based on it's name.
 *
 * @param name The name of the plugin that needs to be checked.
 *
 * @return TRUE if found, else FALSE.
 */
int visual_input_valid_by_name (const char *name)
{
	if (visual_plugin_find (visual_input_get_list (), name) == NULL)
		return FALSE;
	else
		return TRUE;
}

/**
 * Creates a new VisInput from name, the plugin will be loaded but won't be realized.
 *
 * @param inputname
 * 	The name of the plugin to load, or NULL to simply allocate a new
 * 	input.
 *
 * @return A newly allocated VisInput, optionally containing a loaded plugin. Or NULL on failure.
 */
VisInput *visual_input_new (const char *inputname)
{
	VisInput *input;
	VisPluginRef *ref;

//	visual_log_return_val_if_fail (__lv_plugins_input != NULL && inputname == NULL, NULL);

	if (__lv_plugins_input == NULL && inputname != NULL) {
		visual_log (VISUAL_LOG_CRITICAL, "the plugin list is NULL");
		return NULL;
	}
	
	input = visual_mem_new0 (VisInput, 1);
	
	input->audio = visual_audio_new ();

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (input), TRUE, input_dtor);

	if (inputname == NULL)
		return input;
	
	ref = visual_plugin_find (__lv_plugins_input, inputname);
	
	input->plugin = visual_plugin_load (ref);

	return input;
}

/**
 * Realize the VisInput. This also calls the plugin init function.
 * 
 * @param input Pointer to a VisInput that needs to be realized.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_INPUT_NULL or error values returned by
 *	visual_plugin_realize () on failure.
 */
int visual_input_realize (VisInput *input)
{
	visual_log_return_val_if_fail (input != NULL, -VISUAL_ERROR_INPUT_NULL);

	if (input->plugin != NULL && input->callback == NULL)
		return visual_plugin_realize (input->plugin);

	return VISUAL_OK;
}

/**
 * Sets a callback function for VisInput. Callback functions can be used instead of plugins. Using
 * a callback function you can implement an in app PCM data upload function which is like the
 * upload callback that is used for input plugins.
 *
 * @param input Pointer to a VisInput that to which a callback needs to be set.
 * @param callback The in app callback function that should be used instead of a plugin.
 * @param priv A private that can be read within the callback function.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_INPUT_NULL on failure.
 */
int visual_input_set_callback (VisInput *input, VisInputUploadCallbackFunc callback, void *priv)
{
	visual_log_return_val_if_fail (input != NULL, -VISUAL_ERROR_INPUT_NULL);

	input->callback = callback;
	visual_object_set_private (VISUAL_OBJECT (input), priv);

	return VISUAL_OK;
}

/**
 * This is called to run a VisInput. This function will call the plugin to upload it's samples and run it
 * through the visual_audio_analyze function. If a callback is set it will use the callback instead of
 * the plugin.
 *
 * @param input A pointer to a VisInput that needs to be runned.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_INPUT_NULL or -VISUAL_ERROR_INPUT_PLUGIN_NULL on failure.
 */
int visual_input_run (VisInput *input)
{
	VisInputPlugin *inplugin;

	visual_log_return_val_if_fail (input != NULL, -VISUAL_ERROR_INPUT_NULL);

	if (input->callback == NULL) {
		inplugin = get_input_plugin (input);

		if (inplugin == NULL) {
			visual_log (VISUAL_LOG_CRITICAL, "The input plugin is not loaded correctly.");
		
			return -VISUAL_ERROR_INPUT_PLUGIN_NULL;
		}
		
		inplugin->upload (input->plugin, input->audio);
	} else
		input->callback (input, input->audio, visual_object_get_private (VISUAL_OBJECT (input)));

	visual_audio_analyze (input->audio);

	return VISUAL_OK;
}

/**
 * @}
 */

