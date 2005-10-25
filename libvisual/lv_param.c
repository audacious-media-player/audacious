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

#include "lv_log.h"
#include "lv_param.h"

static int paramcontainer_dtor (VisObject *object);
static int paramentry_dtor (VisObject *object);

static int get_next_pcall_id (VisList *callbacks);

static int paramcontainer_dtor (VisObject *object)
{
	VisParamContainer *paramcontainer = VISUAL_PARAMCONTAINER (object);

	visual_list_destroy_elements (&paramcontainer->entries);

	return VISUAL_OK;
}

static int paramentry_dtor (VisObject *object)
{
	VisParamEntry *param = VISUAL_PARAMENTRY (object);

	if (param->string != NULL)
		visual_mem_free (param->string);

	if (param->name != NULL)
		visual_mem_free (param->name);

	if (param->objdata != NULL)
		visual_object_unref (param->objdata);
	
	visual_palette_free_colors (&param->pal);

	visual_list_destroy_elements (&param->callbacks);

	param->string = NULL;
	param->name = NULL;
	param->objdata = NULL;

	return VISUAL_OK;
}

static int get_next_pcall_id (VisList *callbacks)
{
	VisListEntry *le = NULL;
	VisParamEntryCallback *pcall;
	int found = FALSE;
	int i;

	/* Walk through all possible ids */
	for (i = 0; i < VISUAL_PARAM_CALLBACK_ID_MAX; i++) {

		found = FALSE;
		/* Check all the callbacks if the id is used */
		while ((pcall = visual_list_next (callbacks, &le)) != NULL) {
		
			/* Found the ID, break and get ready for the next iterate */
			if (pcall->id == i) {
				found = TRUE;

				break;
			}
		}

		/* The id has NOT been found, thus is an original, and we return this as the next id */
		if (found == FALSE)
			return i;
	}

	/* This is virtually impossible, or something very wrong is going ok, but no id seems to be left */
	return -1;
}


/**
 * @defgroup VisParam VisParam
 * @{
 */

/**
 * Creates a new VisParamContainer structure.
 *
 * @return A newly allocated VisParamContainer structure.
 */
VisParamContainer *visual_param_container_new ()
{
	VisParamContainer *paramcontainer;

	paramcontainer = visual_mem_new0 (VisParamContainer, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (paramcontainer), TRUE, paramcontainer_dtor);

	visual_list_set_destroyer (&paramcontainer->entries, visual_object_list_destroyer);

	return paramcontainer;
}

/**
 * Sets the eventqueue in the VisParamContainer, so events can be emitted on param changes.
 *
 * @param paramcontainer A pointer to the VisParamContainer to which the VisEventQueue needs to be set.
 * @param eventqueue A Pointer to the VisEventQueue that is used for the events the VisParamContainer can emit.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_CONTAINER_NULL on failure.
 */
int visual_param_container_set_eventqueue (VisParamContainer *paramcontainer, VisEventQueue *eventqueue)
{
	visual_log_return_val_if_fail (paramcontainer != NULL, -VISUAL_ERROR_PARAM_CONTAINER_NULL);

	paramcontainer->eventqueue = eventqueue;

	return VISUAL_OK;
}

/**
 * Get the pointer to the VisEventQueue the VisParamContainer is emitting events to.
 *
 * @param paramcontainer A pointer to the VisParamContainer from which the VisEventQueue is requested.
 * 
 * @return Pointer to the VisEventQueue possibly NULL, NULL on failure.
 */
VisEventQueue *visual_param_container_get_eventqueue (VisParamContainer *paramcontainer)
{
	visual_log_return_val_if_fail (paramcontainer != NULL, NULL);

	return paramcontainer->eventqueue;
}

/**
 * Adds a VisParamEntry to a VisParamContainer.
 *
 * @param paramcontainer A pointer to the VisParamContainer in which the VisParamEntry is added.
 * @param param A pointer to the VisParamEntry that is added to the VisParamContainer.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_CONTAINER_NULL, -VISUAL_ERROR_PARAM_NULL or
 *	error values returned by visual_list_add () on failure.
 */
int visual_param_container_add (VisParamContainer *paramcontainer, VisParamEntry *param)
{
	visual_log_return_val_if_fail (paramcontainer != NULL, -VISUAL_ERROR_PARAM_CONTAINER_NULL);
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->parent = paramcontainer;

	/* On container add, we always set changed once, so vars can be synchronised in the plugin
	 * it's event loop */
	visual_param_entry_changed (param);

	return visual_list_add (&paramcontainer->entries, param);
}

/**
 * Adds a list of VisParamEntry elements, the list is terminated by an entry of type VISUAL_PARAM_ENTRY_TYPE_END.
 * All the elements are reallocated, so this function can be used for static param lists.
 *
 * @param paramcontainer A pointer to the VisParamContainer in which the VisParamEntry elements are added.
 * @param params A pointer to the VisParamEntry elements that are added to the VisParamContainer.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_CONTAINER_NULL or -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_container_add_many (VisParamContainer *paramcontainer, VisParamEntry *params)
{
	VisParamEntry *pnew;
	int i = 0;

	visual_log_return_val_if_fail (paramcontainer != NULL, -VISUAL_ERROR_PARAM_CONTAINER_NULL);
	visual_log_return_val_if_fail (params != NULL, -VISUAL_ERROR_PARAM_NULL);

	while (params[i].type != VISUAL_PARAM_ENTRY_TYPE_END) {
		pnew = visual_param_entry_new (visual_param_entry_get_name (&params[i]));
		visual_param_entry_set_from_param (pnew, &params[i]);

		visual_param_container_add (paramcontainer, pnew);
		
		i++;
	}

	return VISUAL_OK;
}

/**
 * Removes a VisParamEntry from the VisParamContainer by giving the name of the VisParamEntry that needs
 * to be removed.
 *
 * @param paramcontainer A pointer to the VisParamContainer from which a VisParamEntry needs to be removed.
 * @param name The name of the VisParamEntry that needs to be removed from the VisParamContainer.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_CONTAINER_NULL, -VISUAL_ERROR_NULL
 *	or -VISUAL_ERROR_PARAM_NOT_FOUND on failure.
 */
int visual_param_container_remove (VisParamContainer *paramcontainer, const char *name)
{
	VisListEntry *le = NULL;
	VisParamEntry *param;

	visual_log_return_val_if_fail (paramcontainer != NULL, -VISUAL_ERROR_PARAM_CONTAINER_NULL);
	visual_log_return_val_if_fail (name != NULL, -VISUAL_ERROR_NULL);
	
	while ((param = visual_list_next (&paramcontainer->entries, &le)) != NULL) {

		if (strcmp (param->name, name) == 0) {
			visual_list_delete (&paramcontainer->entries, &le);

			return VISUAL_OK;
		}
	}

	return -VISUAL_ERROR_PARAM_NOT_FOUND;
}

/**
 * Clones the source VisParamContainer into the destination VisParamContainer. When an entry with a certain name
 * already exists in the destination container, it will be overwritten with a new value.
 *
 * @param destcont A pointer to the VisParamContainer in which the VisParamEntry values are copied.
 * @param srccont A pointer to the VisParamContainer from which the VisParamEntry values are copied.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_CONTAINER_NULL, on failure.
 */
int visual_param_container_copy (VisParamContainer *destcont, VisParamContainer *srccont)
{
	VisListEntry *le = NULL;
	VisParamEntry *destparam;
	VisParamEntry *srcparam;
	VisParamEntry *tempparam;

	visual_log_return_val_if_fail (destcont != NULL, -VISUAL_ERROR_PARAM_CONTAINER_NULL);
	visual_log_return_val_if_fail (srccont != NULL, -VISUAL_ERROR_PARAM_CONTAINER_NULL);

	while ((srcparam = visual_list_next (&srccont->entries, &le)) != NULL) {
		tempparam = visual_param_container_get (destcont, visual_param_entry_get_name (srcparam));

		/* Already exists, overwrite */
		if (tempparam != NULL) {
			visual_param_entry_set_from_param (tempparam, srcparam);
			
			continue;
		}
		
		/* Does not yet exist, create a new entry */
		destparam = visual_param_entry_new (visual_param_entry_get_name (srcparam));
		visual_param_entry_set_from_param (destparam, srcparam);

		visual_param_container_add (destcont, destparam);
	}

	return VISUAL_OK;
}

/**
 * Copies matching VisParamEntry elements from srccont into destcont, matching on the name.
 *
 * @param destcont A pointer to the VisParamContainer in which the VisParamEntry values are copied.
 * @param srccont A pointer to the VisParamContainer from which the VisParamEntry values are copied.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_CONTAINER_NULL, on failure.
 */
int visual_param_container_copy_match (VisParamContainer *destcont, VisParamContainer *srccont)
{
	VisListEntry *le = NULL;
	VisParamEntry *destparam;
	VisParamEntry *srcparam;

	visual_log_return_val_if_fail (destcont != NULL, -VISUAL_ERROR_PARAM_CONTAINER_NULL);
	visual_log_return_val_if_fail (srccont != NULL, -VISUAL_ERROR_PARAM_CONTAINER_NULL);

	while ((destparam = visual_list_next (&destcont->entries, &le)) != NULL) {
		srcparam = visual_param_container_get (srccont, visual_param_entry_get_name (destparam));

		if (srcparam != NULL)
			visual_param_entry_set_from_param (destparam, srcparam);
	}

	return VISUAL_OK;
}

/**
 * Retrieve a VisParamEntry from a VisParamContainer by giving the name of the VisParamEntry that is requested.
 *
 * @param paramcontainer A pointer to the VisParamContainer from which a VisParamEntry is requested.
 * @param name The name of the VisParamEntry that is requested from the VisParamContainer.
 *
 * @return Pointer to the VisParamEntry, or NULL.
 */
VisParamEntry *visual_param_container_get (VisParamContainer *paramcontainer, const char *name)
{
	VisListEntry *le = NULL;
	VisParamEntry *param;

	visual_log_return_val_if_fail (paramcontainer != NULL, NULL);
	visual_log_return_val_if_fail (name != NULL, NULL);

	while ((param = visual_list_next (&paramcontainer->entries, &le)) != NULL) {
		param = le->data;
		
		if (strcmp (param->name, name) == 0)
			return param;
	}
	
	return NULL;
}

/**
 * Creates a new VisParamEntry structure.
 *
 * @param name The name that is assigned to the VisParamEntry.
 *
 * @return A newly allocated VisParamEntry structure.
 */
VisParamEntry *visual_param_entry_new (char *name)
{
	VisParamEntry *param;

	param = visual_mem_new0 (VisParamEntry, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (param), TRUE, paramentry_dtor);

	visual_param_entry_set_name (param, name);
	
	visual_list_set_destroyer (&param->callbacks, visual_object_list_destroyer);

	return param;
}

/**
 * Adds a change notification callback, this shouldn't be used to get notificated within a plugin, but is for
 * things like VisUI.
 *
 * @param param Pointer to the VisParamEntry to which a change notification callback is added.
 * @param callback The notification callback, which is called on changes in the VisParamEntry.
 * @param priv A private that can be used in the callback function.
 *
 * return callback id in the form of a positive value on succes,
 * 	-VISUAL_ERROR_PARAM_NULL, -VISUAL_ERROR_PARAM_CALLBACK_NULL or
 * 	-VISUAL_ERROR_PARAM_CALLBACK_TOO_MANY on failure.
 */
int visual_param_entry_add_callback (VisParamEntry *param, VisParamChangedCallbackFunc callback, void *priv)
{
	VisParamEntryCallback *pcall;
	int id;

	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);
	visual_log_return_val_if_fail (callback != NULL, -VISUAL_ERROR_PARAM_CALLBACK_NULL);

	id = get_next_pcall_id (&param->callbacks);

	visual_log_return_val_if_fail (id >= 0, -VISUAL_ERROR_PARAM_CALLBACK_TOO_MANY);

	pcall = visual_mem_new0 (VisParamEntryCallback, 1);

	/* Do the VisObject initialization for the VisParamEntryCallback */
	visual_object_initialize (VISUAL_OBJECT (pcall), TRUE, NULL);

	pcall->id = id;
	pcall->callback = callback;
	visual_object_set_private (VISUAL_OBJECT (pcall), priv);
	
	visual_list_add (&param->callbacks, pcall);

	return id;
}

/**
 * Removes a change notification callback from the list of callbacks.
 *
 * @param param Pointer to the VisParamEntry from which a change notification callback is removed.
 * @param id The callback ID that was given by the visual_param_entry_add_callback method.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_remove_callback (VisParamEntry *param, int id)
{
	VisListEntry *le = NULL;
	VisParamEntryCallback *pcall;

	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	while ((pcall = visual_list_next (&param->callbacks, &le)) != NULL) {
		
		if (id == pcall->id) {
			visual_list_delete (&param->callbacks, &le);

			visual_object_unref (VISUAL_OBJECT (pcall));

			return VISUAL_OK;
		}
	}

	return VISUAL_OK;
}

/**
 * Notifies all the callbacks for the given VisParamEntry parameter.
 *
 * @param param Pointer to the VisParamEntry of which all the change notification
 * 	callbacks need to be called.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_notify_callbacks (VisParamEntry *param)
{
	VisListEntry *le = NULL;
	VisParamEntryCallback *pcall;
	
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	while ((pcall = visual_list_next (&param->callbacks, &le)) != NULL)
		pcall->callback (param, visual_object_get_private (VISUAL_OBJECT (pcall)));

	return VISUAL_OK;
}

/**
 * Checks if the VisParamEntry it's name is the given name.
 *
 * @param param Pointer to the VisParamEntry of which we want to check the name.
 * @param name The name we want to check against.
 *
 * @return TRUE if the VisParamEntry is the one we requested, or FALSE if not.
 */
int visual_param_entry_is (VisParamEntry *param, const char *name)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	if (strcmp (param->name, name) == 0)
		return TRUE;

	return FALSE;
}

/**
 * When called, emits an event in the VisParamContainer it's VisEventQueue when the VisEventQueue
 * is set.
 * 
 * @param param Pointer to the VisParamEntry that is changed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_changed (VisParamEntry *param)
{
	VisEventQueue *eventqueue;

	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	if (param->parent == NULL)
		return VISUAL_OK;

	eventqueue = param->parent->eventqueue;

	if (eventqueue != NULL)
		visual_event_queue_add_param (eventqueue, param);

	visual_param_entry_notify_callbacks (param);

	return VISUAL_OK;
}

/**
 * Retrieves the VisParamEntryType from a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the VisParamEntryType is requested.
 *
 * @return The VisParamEntryType on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
VisParamEntryType visual_param_entry_get_type (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	return param->type;
}

/**
 * Compares two parameters with each other, When they are the same, TRUE is returned, if not FALSE.
 * Keep in mind that FALSE is always returned for VISUAL_PARAM_ENTRY_TYPE_PALETTE and VISUAL_PARAM_ENTRY_TYPE_OBJECT.
 *
 * @param src1 Pointer to the first VisParamEntry for comparison.
 * @param src2 Pointer to the second VisParamEntry for comparison.
 *
 * @return TRUE if the same, FALSE if not the same,
 *	-VISUAL_ERROR_PARAM_NULL, -VISUAL_ERROR_PARAM_INVALID_TYPE or -VISUAL_ERROR_IMPOSSIBLE on failure.
 */
int visual_param_entry_compare (VisParamEntry *src1, VisParamEntry *src2)
{
	visual_log_return_val_if_fail (src1 != NULL, -VISUAL_ERROR_PARAM_NULL);
	visual_log_return_val_if_fail (src2 != NULL, -VISUAL_ERROR_PARAM_NULL);

	if (src1->type != src2->type)
		return FALSE;

	switch (src1->type) {
		case VISUAL_PARAM_ENTRY_TYPE_NULL:
			return TRUE;
			
			break;

		case VISUAL_PARAM_ENTRY_TYPE_STRING:
			if (!strcmp (src1->string, src2->string))
				return TRUE;

			break;

		case VISUAL_PARAM_ENTRY_TYPE_INTEGER:
			if (src1->numeric.integer == src2->numeric.integer)
				return TRUE;
			
			break;

		case VISUAL_PARAM_ENTRY_TYPE_FLOAT:
			if (src1->numeric.floating == src2->numeric.floating)
				return TRUE;

			break;

		case VISUAL_PARAM_ENTRY_TYPE_DOUBLE:
			if (src1->numeric.doubleflt == src2->numeric.doubleflt)
				return TRUE;

			break;

		case VISUAL_PARAM_ENTRY_TYPE_COLOR:
			return visual_color_compare (&src1->color, &src2->color);
			
			break;

		case VISUAL_PARAM_ENTRY_TYPE_PALETTE:
			return FALSE;

			break;

		case VISUAL_PARAM_ENTRY_TYPE_OBJECT:
			return FALSE;

			break;

		default:
			visual_log (VISUAL_LOG_CRITICAL, "param type is not valid");

			return -VISUAL_ERROR_PARAM_INVALID_TYPE;

			break;
	}

	return -VISUAL_ERROR_IMPOSSIBLE;
}

/**
 * Copies the value of the src param into the param. Also sets the param to the type of which the
 * source param is.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param src Pointer to the VisParamEntry from which the value is retrieved.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL, -VISUAL_ERROR_PARAM_INVALID_TYPE on failure.
 */
int visual_param_entry_set_from_param (VisParamEntry *param, VisParamEntry *src)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);
	visual_log_return_val_if_fail (src != NULL, -VISUAL_ERROR_PARAM_NULL);

	switch (src->type) {
		case VISUAL_PARAM_ENTRY_TYPE_NULL:

			break;

		case VISUAL_PARAM_ENTRY_TYPE_STRING:
			visual_param_entry_set_string (param, visual_param_entry_get_string (src));
			break;

		case VISUAL_PARAM_ENTRY_TYPE_INTEGER:
			visual_param_entry_set_integer (param, visual_param_entry_get_integer (src));
			
			break;

		case VISUAL_PARAM_ENTRY_TYPE_FLOAT:
			visual_param_entry_set_float (param, visual_param_entry_get_float (src));

			break;

		case VISUAL_PARAM_ENTRY_TYPE_DOUBLE:
			visual_param_entry_set_double (param, visual_param_entry_get_double (src));

			break;

		case VISUAL_PARAM_ENTRY_TYPE_COLOR:
			visual_param_entry_set_color_by_color (param, visual_param_entry_get_color (src));

			break;

		case VISUAL_PARAM_ENTRY_TYPE_PALETTE:
			visual_param_entry_set_palette (param, visual_param_entry_get_palette (src));

			break;

		case VISUAL_PARAM_ENTRY_TYPE_OBJECT:
			visual_param_entry_set_object (param, visual_param_entry_get_object (src));

			break;
		
		default:
			visual_log (VISUAL_LOG_CRITICAL, "param type is not valid");

			return -VISUAL_ERROR_PARAM_INVALID_TYPE;

			break;
	}
	
	return VISUAL_OK;
}

/**
 * Set the name for a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry to which the name is set.
 * @param name The name that is set to the VisParamEntry.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_name (VisParamEntry *param, char *name)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	if (param->name != NULL)
		visual_mem_free (param->name);

	param->name = NULL;
	
	if (name != NULL)
		param->name = strdup (name);

	return VISUAL_OK;
}

/**
 * Sets the VisParamEntry to VISUAL_PARAM_ENTRY_TYPE_STRING and assigns the string given as argument to it.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param string The string for this parameter.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_string (VisParamEntry *param, char *string)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->type = VISUAL_PARAM_ENTRY_TYPE_STRING;

	if (string == NULL && param->string == NULL)
		return VISUAL_OK;

	if (string == NULL && param->string != NULL) {
		visual_mem_free (param->string);
		param->string = NULL;

		visual_param_entry_changed (param);

	} else if (param->string == NULL && string != NULL) {
		param->string = strdup (string);

		visual_param_entry_changed (param);

	} else if (strcmp (string, param->string) != 0) {
		visual_mem_free (param->string);
		
		param->string = strdup (string);

		visual_param_entry_changed (param);
	}

	return VISUAL_OK;
}

/**
 * Sets the VisParamEntry to VISUAL_PARAM_ENTRY_TYPE_INTEGER and assigns the integer given as argument to it.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param integer The integer value for this parameter.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_integer (VisParamEntry *param, int integer)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->type = VISUAL_PARAM_ENTRY_TYPE_INTEGER;

	if (param->numeric.integer != integer) {
		param->numeric.integer = integer;

		visual_param_entry_changed (param);
	}
	
	return VISUAL_OK;
}

/**
 * Sets the VisParamEntry to VISUAL_PARAM_ENTRY_TYPE_FLOAT and assigns the float given as argument to it.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param floating The float value for this parameter.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_float (VisParamEntry *param, float floating)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->type = VISUAL_PARAM_ENTRY_TYPE_FLOAT;

	if (param->numeric.floating != floating) {
		param->numeric.floating = floating;

		visual_param_entry_changed (param);
	}
	
	return VISUAL_OK;
}

/**
 * Sets the VisParamEntry to VISUAL_PARAM_ENTRY_TYPE_DOUBLE and assigns the double given as argument to it.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param doubleflt The double value for this parameter.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_double (VisParamEntry *param, double doubleflt)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->type = VISUAL_PARAM_ENTRY_TYPE_DOUBLE;

	if (param->numeric.doubleflt != doubleflt) {
		param->numeric.doubleflt = doubleflt;

		visual_param_entry_changed (param);
	}

	return VISUAL_OK;
}

/**
 * Sets the VisParamEntry to VISUAL_PARAM_ENTRY_TYPE_COLOR and assigns the rgb values given as arguments to it.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param r The red value for this color parameter.
 * @param g The green value for this color parameter.
 * @param b The blue value for this color parameter.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_color (VisParamEntry *param, uint8_t r, uint8_t g, uint8_t b)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->type = VISUAL_PARAM_ENTRY_TYPE_COLOR;

	if (param->color.r != r || param->color.g != g || param->color.b != b) {
		param->color.r = r;
		param->color.g = g;
		param->color.b = b;

		visual_param_entry_changed (param);
	}

	return VISUAL_OK;
}

/**
 * Sets the VisParamEntry to VISUAL_PARAM_ENTRY_TYPE_COLOR and assigns the rgb values from the given VisColor as argument to it.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param color Pointer to the VisColor from which the rgb values are copied into the parameter.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_color_by_color (VisParamEntry *param, VisColor *color)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->type = VISUAL_PARAM_ENTRY_TYPE_COLOR;

	if (visual_color_compare (&param->color, color) == FALSE) {
		visual_color_copy (&param->color, color);

		visual_param_entry_changed (param);
	}

	return VISUAL_OK;
}

/**
 * Sets the VisParamEntry to VISUAL_PARAM_ENTRY_TYPE_PALETTE and assigns a VisPalette to the VisParamEntry.
 * This function does not check if there is a difference between the prior set palette and the new one, and always
 * emits the changed event. so watch out with usage.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param pal Pointer to the VisPalette from which the palette data is retrieved for the VisParamEntry.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_palette (VisParamEntry *param, VisPalette *pal)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->type = VISUAL_PARAM_ENTRY_TYPE_PALETTE;

	visual_palette_free_colors (&param->pal);
	
	if (pal != NULL) {
		visual_palette_allocate_colors (&param->pal, pal->ncolors);
		
		visual_palette_copy (&param->pal, pal);
	}

	visual_param_entry_changed (param);
	
	return VISUAL_OK;
}

/**
 * Sets the VisParamEntry to VISUAL_PARAM_ENTRY_TYPE_OBJECT and assigns a VisObject to the VisParamEntry.
 * With a VisObject VisParamEntry, the VisObject is referenced, not cloned.
 *
 * @param param Pointer to the VisParamEntry to which a parameter is set.
 * @param object Pointer to the VisObject that is linked to the VisParamEntry.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_param_entry_set_object (VisParamEntry *param, VisObject *object)
{
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	param->type = VISUAL_PARAM_ENTRY_TYPE_OBJECT;

	if (param->objdata != NULL)
		visual_object_unref (param->objdata);

	param->objdata = object;

	if (param->objdata != NULL)
		visual_object_ref (param->objdata);

	visual_param_entry_changed (param);
	
	return VISUAL_OK;
}

/**
 * Get the name of the VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the name is requested.
 * 
 * @return The name of the VisParamEntry or NULL.
 */
char *visual_param_entry_get_name (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, NULL);

	return param->name;
}

/**
 * Get the string parameter from a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the string parameter is requested.
 *
 * @return The string parameter from the VisParamEntry or NULL.
 */
char *visual_param_entry_get_string (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, NULL);

	if (param->type != VISUAL_PARAM_ENTRY_TYPE_STRING) {
		visual_log (VISUAL_LOG_WARNING, "Requesting string from a non string param");

		return NULL;
	}

	return param->string;
}

/**
 * Get the integer parameter from a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the integer parameter is requested.
 *
 * @return The integer parameter from the VisParamEntry.
 */
int visual_param_entry_get_integer (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, 0);

	if (param->type != VISUAL_PARAM_ENTRY_TYPE_INTEGER)
		visual_log (VISUAL_LOG_WARNING, "Requesting integer from a non integer param");

	return param->numeric.integer;
}

/**
 * Get the float parameter from a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the float parameter is requested.
 *
 * @return The float parameter from the VisParamEntry.
 */
float visual_param_entry_get_float (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, 0);

	if (param->type != VISUAL_PARAM_ENTRY_TYPE_FLOAT)
		visual_log (VISUAL_LOG_WARNING, "Requesting float from a non float param");

	return param->numeric.floating;
}

/**
 * Get the double parameter from a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the double parameter is requested.
 *
 * @return The double parameter from the VisParamEntry.
 */
double visual_param_entry_get_double (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, 0);

	if (param->type != VISUAL_PARAM_ENTRY_TYPE_DOUBLE)
		visual_log (VISUAL_LOG_WARNING, "Requesting double from a non double param");

	return param->numeric.doubleflt;
}

/**
 * Get the color parameter from a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the color parameter is requested.
 *
 * @return Pointer to the VisColor parameter from the VisParamEntry. It's adviced to
 *	use the VisColor that is returned as read only seen changing it directly won't emit events and
 *	can cause synchronous problems between the plugin and the parameter system. Instead use the
 *	visual_param_entry_set_color* methods to change the parameter value.
 */
VisColor *visual_param_entry_get_color (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, NULL);

	if (param->type != VISUAL_PARAM_ENTRY_TYPE_COLOR) {
		visual_log (VISUAL_LOG_WARNING, "Requesting color from a non color param");

		return NULL;
	}

	return &param->color;
}

/**
 * Get the palette parameter from a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the palette parameter is requested.
 *
 * @return Pointer to the VisPalette parameter from the VisParamEntry. The returned VisPalette
 *	should be exclusively used as read only.
 */
VisPalette *visual_param_entry_get_palette (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, NULL);

	if (param->type != VISUAL_PARAM_ENTRY_TYPE_PALETTE) {
		visual_log (VISUAL_LOG_WARNING, "Requested palette from a non palette param\n");

		return NULL;
	}

	return &param->pal;
}

/**
 * Get the object parameter from a VisParamEntry.
 *
 * @param param Pointer to the VisParamEntry from which the object parameter is requested.
 *
 * @return Pointer to the VisObject parameter from the VisParamEntry.
 */
VisObject *visual_param_entry_get_object (VisParamEntry *param)
{
	visual_log_return_val_if_fail (param != NULL, NULL);

	if (param->type != VISUAL_PARAM_ENTRY_TYPE_OBJECT) {
		visual_log (VISUAL_LOG_WARNING, "Requested object from a non object param\n");

		return NULL;
	}

	return param->objdata;
}

/**
 * @}
 */

