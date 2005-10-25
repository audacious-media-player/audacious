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
#include "lv_object.h"


/**
 * @defgroup VisObject VisObject
 * @{
 */

/**
 * This function is a global VisListDestroyerFunc handler that unrefs VisObjects.
 *
 * @param data Pointer to the VisObject that needs to be unrefed
 */
void visual_object_list_destroyer (void *data)
{
	if (data == NULL)
		return;

	visual_object_unref (VISUAL_OBJECT (data));
}

/**
 * Creates a new VisObject structure.
 *
 * @return A newly allocated VisObject, or NULL on failure.
 */
VisObject *visual_object_new ()
{
	VisObject *object;

	object = visual_mem_new0 (VisObject, 1);

	object->allocated = TRUE;

	visual_object_ref (object);

	return object;
}

/**
 * Frees the VisObject. This does not destroy the object itself but only releases the memory it's using.
 *
 * @param object Pointer to a VisObject that needs to be freed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_OBJECT_NULL, -VISUAL_ERROR_OBJECT_NOT_ALLOCATED or error values
 *	returned by visual_mem_free on failure.
 */
int visual_object_free (VisObject *object)
{
	visual_log_return_val_if_fail (object != NULL, -VISUAL_ERROR_OBJECT_NULL);
	visual_log_return_val_if_fail (object->allocated == TRUE, -VISUAL_ERROR_OBJECT_NOT_ALLOCATED);

	return visual_mem_free (object);
}

/**
 * Destroys the VisObject. This does destruct the VisObject 
 * by using the dtor function if it's set and also frees the memory
 * it's using. It's valid to pass non allocated VisObjects,
 * the function will recognize this by a flag that is set in the VisObject.
 *
 * @param object Pointer to a VisObject that needs to be destroyed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_OBJECT_NULL or error values returned byvisual_object_free on failure.
 */
int visual_object_destroy (VisObject *object)
{
	visual_log_return_val_if_fail (object != NULL, -VISUAL_ERROR_OBJECT_NULL);

	if (object->dtor != NULL)
		object->dtor (object);

	if (object->allocated == TRUE)
		return visual_object_free (object);

	return VISUAL_OK;
}

/**
 * Initializes a VisObject for usage. This also ups the refcount by one, so this function really is for initial object
 * creation.
 *
 * @param object Pointer to a VisObject that is initialized.
 * @param allocated Flag to indicate if the VisObject itself is an allocated piece of memory.
 * @param dtor The destructor function, that is used to destroy the VisObject when it loses all references or when it's
 *	being destroyed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_OBJECT_NULL on failure.
 */
int visual_object_initialize (VisObject *object, int allocated, VisObjectDtorFunc dtor)
{
	visual_log_return_val_if_fail (object != NULL, -VISUAL_ERROR_OBJECT_NULL);

	object->allocated = allocated;
	object->dtor = dtor;

	visual_object_ref (object);

	return VISUAL_OK;
}

/**
 * Increases the reference counter for a VisObject.
 *
 * @param object Pointer to a VisObject in which the reference count is increased.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_OBJECT_NULL on failure.
 */
int visual_object_ref (VisObject *object)
{
	visual_log_return_val_if_fail (object != NULL, -VISUAL_ERROR_OBJECT_NULL);
	
	object->refcount++;

	return VISUAL_OK;
}

/**
 * Decreases the reference counter for a VisObject. If the reference counter hits zero it will
 * destruct the object using visual_object_destroy.
 *
 * @see visual_object_destroy
 *
 * @param object Pointer to a VisObject in which the reference count is decreased.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_OBJECT_NULL or error values returned by
 *	visual_object_destroy on failure.
 */
int visual_object_unref (VisObject *object)
{
	visual_log_return_val_if_fail (object != NULL, -VISUAL_ERROR_OBJECT_NULL);
	
	object->refcount--;

	/* No reference left, start dtoring of this VisObject */
	if (object->refcount <= 0) {
		object->refcount = 0;

		return visual_object_destroy (object);
	}
	return VISUAL_OK;
}

/**
 * Sets the private data pointer to a VisObject.
 *
 * @param object Pointer to a VisObject to which the private data is set.
 * @param priv Pointer to the private data that is set to the VisObject.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_OBJECT_NULL on failure.
 */
int visual_object_set_private (VisObject *object, void *priv)
{
	visual_log_return_val_if_fail (object != NULL, -VISUAL_ERROR_OBJECT_NULL);

	object->priv = priv;

	return VISUAL_OK;
}

/**
 * Retrieves the private data from a VisObject.
 *
 * @param object Pointer to a VisObject from which the private data is requested.
 *
 * @return Pointer to the private data or NULL.
 */
void *visual_object_get_private (VisObject *object)
{
	visual_log_return_val_if_fail (object != NULL, NULL);

	return object->priv;
}

/**
 * @}
 */

