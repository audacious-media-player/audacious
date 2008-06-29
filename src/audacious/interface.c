/*
 * Audacious2
 * Copyright (c) 2008 William Pitcock <nenolod@dereferenced.org>
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

#include <glib.h>
#include <mowgli.h>

#include "interface.h"

static mowgli_dictionary_t *interface_dict_ = NULL;

void
interface_register(Interface *i)
{
    if (interface_dict_ == NULL)
        interface_dict_ = mowgli_dictionary_create(g_ascii_strcasecmp);

    mowgli_dictionary_add(interface_dict_, i->id, i);
}

void
interface_deregister(Interface *i)
{
    g_return_if_fail(interface_dict_ != NULL);

    mowgli_dictionary_delete(interface_dict_, i->id);
}

/*
 * TODO:
 *     - set up InterfaceOps struct for the Interface to use
 */
void
interface_run(Interface *i)
{
    i->init();
}

void
interface_destroy(Interface *i)
{
    if (i->fini != NULL)
        i->fini();
}

Interface *
interface_get(gchar *id)
{
    if (interface_dict_ == NULL)
        return NULL;

    return mowgli_dictionary_retrieve(interface_dict_, id);
}
