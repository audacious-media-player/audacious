/*
 * Audacious
 * Copyright (c) 2007 William Pitcock
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include "mime.h"

mowgli_dictionary_t *mime_type_dict = NULL;

void mime_set_plugin(const gchar *mimetype, InputPlugin *ip)
{
    g_return_if_fail(mimetype != NULL);
    g_return_if_fail(ip != NULL);

    if (mime_type_dict == NULL)
        mime_type_dict = mowgli_dictionary_create(strcasecmp);
    else if (mowgli_dictionary_find(mime_type_dict, mimetype))
        mowgli_dictionary_delete(mime_type_dict, mimetype);
    mowgli_dictionary_add(mime_type_dict, mimetype, ip);
}

InputPlugin *mime_get_plugin(const gchar *mimetype)
{
    if (mimetype == NULL)
        return NULL;

    if (mime_type_dict == NULL)
        return NULL;

    return mowgli_dictionary_retrieve(mime_type_dict, mimetype);
}
