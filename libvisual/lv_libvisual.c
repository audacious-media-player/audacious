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

#include "lvconfig.h"
#include "lv_plugin.h"
#include "lv_libvisual.h"
#include "lv_log.h"
#include "lv_param.h"
#include "config.h"


/** Set when libvisual is initialized. */
int __lv_initialized = FALSE;
/** Set a progname from argv[0] when we're capable of doing so. */
char *__lv_progname = NULL;

/** Contains the completely plugin registry after initialize. */
VisList *__lv_plugins = NULL;
/** Contains all the actor plugins after initialize. */
VisList *__lv_plugins_actor = NULL;
/** Contains all the input plugins after initialize. */
VisList *__lv_plugins_input = NULL;
/** Contains all the morph plugins after initialize. */
VisList *__lv_plugins_morph = NULL;
/** Contains all the transform plugins after initialize. */
VisList *__lv_plugins_transform = NULL;

/** The global params container */
VisParamContainer *__lv_paramcontainer = NULL;

/** The userinterface for the global params */
VisUIWidget *__lv_userinterface = NULL;

/** Contains the number of plugin registry paths. */
int __lv_plugpath_cnt = 0;
/** Char ** list of all the plugin paths. */
char **__lv_plugpaths = NULL;


static int init_params (VisParamContainer *paramcontainer);
static VisUIWidget *make_userinterface (void);

static int init_params (VisParamContainer *paramcontainer)
{
	VisParamEntry *param;

	visual_log_return_val_if_fail (paramcontainer != NULL, -1);

	/* Initialize all the global parameters here */

	/* Song information parameters */
	/* Show songinfo */
	param = visual_param_entry_new ("songinfo show");
	visual_param_entry_set_integer (param, 1);
	visual_param_container_add (paramcontainer, param);

	/* Songinfo timeout, in seconds */
	param = visual_param_entry_new ("songinfo timeout");
	visual_param_entry_set_integer (param, 5);
	visual_param_container_add (paramcontainer, param);
	
	/* 
	 * Show songinfo in plugins, plugins that optionally show song
	 * info should query this parameter
	 */
	param = visual_param_entry_new ("songinfo in plugin");
	visual_param_entry_set_integer (param, 1);
	visual_param_container_add (paramcontainer, param);

	return 0;
}

static VisUIWidget *make_userinterface ()
{
	VisUIWidget *vbox;
	VisUIWidget *hbox;
	VisUIWidget *label1;
	VisUIWidget *label2;
	VisUIWidget *checkbox1;
	VisUIWidget *checkbox2;
	VisUIWidget *numeric;

	vbox = visual_ui_box_new (VISUAL_ORIENT_TYPE_VERTICAL);
	hbox = visual_ui_box_new (VISUAL_ORIENT_TYPE_HORIZONTAL);
	
	label1 = visual_ui_label_new ("Show info for", FALSE);
	label2 = visual_ui_label_new ("seconds", FALSE);

	checkbox1 = visual_ui_checkbox_new ("Show song information", TRUE);
	visual_ui_mutator_set_param (VISUAL_UI_MUTATOR (checkbox1),
			visual_param_container_get (__lv_paramcontainer, "songinfo show"));

	checkbox2 = visual_ui_checkbox_new ("Show song information in plugins", TRUE);
	visual_ui_mutator_set_param (VISUAL_UI_MUTATOR (checkbox2),
			visual_param_container_get (__lv_paramcontainer, "songinfo in plugin"));

	numeric = visual_ui_numeric_new ();
	visual_ui_mutator_set_param (VISUAL_UI_MUTATOR (numeric),
			visual_param_container_get (__lv_paramcontainer, "songinfo timeout"));
	visual_ui_range_set_properties (VISUAL_UI_RANGE (numeric), 1, 60, 1, 0);

	visual_ui_box_pack (VISUAL_UI_BOX (hbox), label1);
	visual_ui_box_pack (VISUAL_UI_BOX (hbox), numeric);
	visual_ui_box_pack (VISUAL_UI_BOX (hbox), label2);

	visual_ui_box_pack (VISUAL_UI_BOX (vbox), checkbox1);
	visual_ui_box_pack (VISUAL_UI_BOX (vbox), checkbox2);
	visual_ui_box_pack (VISUAL_UI_BOX (vbox), hbox);

	return vbox;
}

/**
 * @defgroup Libvisual Libvisual
 * @{
 */

/**
 * Gives the libvisual version.
 *
 * @return A const char containing the libvisual version.
 */
const char *visual_get_version ()
{
	return VERSION;
}

/**
 * Returns a pointer to the libvisual global VisParamContainer.
 *
 * @return A pointer to the libvisual global VisParamContainer.
 */
VisParamContainer *visual_get_params ()
{
	return __lv_paramcontainer;
}

/**
 * Returns a pointer to the top container of libvisual it's configuration userinterface.
 *
 * @return A pointer to the libvisual configuration VisUIWidget.
 */
VisUIWidget *visual_get_userinterface ()
{
	return __lv_userinterface;
}

/**
 * Adds extra plugin registry paths.
 *
 * @param pathadd A string containing a path where plugins are located.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_LIBVISUAL_NO_PATHS on failure.
 */
int visual_init_path_add (char *pathadd)
{
	__lv_plugpath_cnt++;
	__lv_plugpaths = realloc (__lv_plugpaths, sizeof (char *) * __lv_plugpath_cnt);

	visual_log_return_val_if_fail (__lv_plugpaths != NULL, -VISUAL_ERROR_LIBVISUAL_NO_PATHS);

	__lv_plugpaths[__lv_plugpath_cnt - 1] = pathadd;

	return VISUAL_OK;
}

/**
 * Initialize libvisual. Sets up a plugin registry, register the program name and such.
 *
 * @param argc Pointer to an int containing the number of arguments within argv or NULL.
 * @param argv Pointer to a list of strings that make up the argument vector or NULL.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_LIBVISUAL_ALREADY_INITIALIZED,
 *	-VISUAL_ERROR_LIBVISUAL_NO_REGISTRY or error values returned by visual_init_path_add () on failure.
 */
int visual_init (int *argc, char ***argv)
{
	int ret = 0;

	if (__lv_initialized == TRUE) {
		visual_log (VISUAL_LOG_ERROR, "Over initialized");
                return -VISUAL_ERROR_LIBVISUAL_ALREADY_INITIALIZED;
        }
		
	if (argc == NULL || argv == NULL) {
		if (argc == NULL && argv == NULL) {
			__lv_progname = strdup ("no progname");
	

			if (__lv_progname == NULL)
				visual_log (VISUAL_LOG_WARNING, "Could not set program name");
		} else
			visual_log (VISUAL_LOG_ERROR, "Initialization failed, bad argv, argc");
		
	} else {
                /*
                 * We must copy the argument, to let the client
                 * call this method from any context.
                 */
#ifdef __USE_GNU
                __lv_progname = strndup (*argv[0], 1024);
#else
                __lv_progname = strdup (*argv[0]);
#endif
                if (__lv_progname == NULL)
                        visual_log (VISUAL_LOG_WARNING, "Could not set program name");
        }
	
	/* Initialize CPU caps */
	visual_cpu_initialize ();

	/* Add the standard plugin paths */
	ret = visual_init_path_add (PLUGPATH"/actor");
	visual_log_return_val_if_fail (ret == VISUAL_OK, ret);

	ret = visual_init_path_add (PLUGPATH"/input");
	visual_log_return_val_if_fail (ret == VISUAL_OK, ret);

	ret = visual_init_path_add (PLUGPATH"/morph");
	visual_log_return_val_if_fail (ret == VISUAL_OK, ret);

	ret = visual_init_path_add (PLUGPATH"/transform");
	visual_log_return_val_if_fail (ret == VISUAL_OK, ret);

	/* And null terminate the list */
	ret = visual_init_path_add (NULL);
	visual_log_return_val_if_fail (ret == VISUAL_OK, ret);

	__lv_plugins = visual_plugin_get_list ((const char**)__lv_plugpaths);
	visual_log_return_val_if_fail (__lv_plugins != NULL, -VISUAL_ERROR_LIBVISUAL_NO_REGISTRY);

	__lv_plugins_actor = visual_plugin_registry_filter (__lv_plugins, VISUAL_PLUGIN_TYPE_ACTOR);
	__lv_plugins_input = visual_plugin_registry_filter (__lv_plugins, VISUAL_PLUGIN_TYPE_INPUT);
	__lv_plugins_morph = visual_plugin_registry_filter (__lv_plugins, VISUAL_PLUGIN_TYPE_MORPH);
	__lv_plugins_transform = visual_plugin_registry_filter (__lv_plugins, VISUAL_PLUGIN_TYPE_TRANSFORM);

	__lv_paramcontainer = visual_param_container_new ();
	init_params (__lv_paramcontainer);
	__lv_userinterface = make_userinterface ();

	__lv_initialized = TRUE;

	return VISUAL_OK;
}

/*
 * Tells whether Libvisual is (correctly) initialized.
 *
 * @return TRUE if is it is initialized, FALSE otherwise.
 */
int visual_is_initialized ()
{
	return __lv_initialized;
}

/**
 * Quits libvisual, destroys all the plugin registries.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_LIBVISUAL_NOT_INITIALIZED on failure.
 */
int visual_quit ()
{
	int ret;

	if (__lv_initialized == FALSE) {
                visual_log (VISUAL_LOG_WARNING, "Never initialized");

		return -VISUAL_ERROR_LIBVISUAL_NOT_INITIALIZED;
	}

	/* FIXME: Use VisError here, for human readable error strings */
	ret = visual_object_unref (VISUAL_OBJECT (__lv_plugins));
	if (ret < 0)
		visual_log (VISUAL_LOG_WARNING, "Plugins references list: destroy failed");

	ret = visual_object_unref (VISUAL_OBJECT (__lv_plugins_actor));
	if (ret < 0)
		visual_log (VISUAL_LOG_WARNING, "Actor plugins list: destroy failed");

	ret = visual_object_unref (VISUAL_OBJECT (__lv_plugins_input));
	if (ret < 0)
		visual_log (VISUAL_LOG_WARNING, "Input plugins list: destroy failed");

	ret = visual_object_unref (VISUAL_OBJECT (__lv_plugins_morph));
	if (ret < 0)
		visual_log (VISUAL_LOG_WARNING, "Morph plugins list: destroy failed");

	ret = visual_object_unref (VISUAL_OBJECT (__lv_plugins_transform));
	if (ret < 0)
		visual_log (VISUAL_LOG_WARNING, "Transform plugins list: destroy failed");

	ret = visual_object_unref (VISUAL_OBJECT (__lv_paramcontainer));
	if (ret < 0)
		visual_log (VISUAL_LOG_WARNING, "Global param container: destroy failed");

	ret = visual_object_unref (VISUAL_OBJECT (__lv_userinterface));
	if (ret < 0)
		visual_log (VISUAL_LOG_WARNING, "Error during UI destroy:");

        if (__lv_progname != NULL) {
                visual_mem_free (__lv_progname);
	
		__lv_progname = NULL;
	}

	__lv_initialized = FALSE;
	return VISUAL_OK;
}

/**
 * @}
 */

