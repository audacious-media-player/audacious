/*  Audacious
 *  Copyright (c) 2006-2007 William Pitcock
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include "hook.h"

static GSList *hook_list;

static Hook *
hook_find(const gchar *name)
{
    GSList *list;

    for (list = hook_list; list != NULL; list = g_slist_next(list))
    {
        Hook *hook = (Hook *) list->data;

        if (!g_ascii_strcasecmp(hook->name, name))
            return hook;
    }

    return NULL;
}

void
hook_register(const gchar *name)
{
    Hook *hook;

    g_return_if_fail(name != NULL);

    if ((hook = hook_find(name)) != NULL)
        return;

    hook = g_new0(Hook, 1);
    hook->name = g_strdup(name);

    hook_list = g_slist_append(hook_list, hook);
}

void
hook_associate(const gchar *name, HookFunction func)
{
    Hook *hook;

    g_return_if_fail(name != NULL);
    g_return_if_fail(func != NULL);

    hook = hook_find(name);

    if (hook == NULL)
        return;

    hook->funcs = g_slist_append(hook->funcs, func);
}

void
hook_dissociate(const gchar *name, HookFunction func)
{
    Hook *hook;

    g_return_if_fail(name != NULL);
    g_return_if_fail(func != NULL);

    hook = hook_find(name);

    if (hook == NULL)
        return;

    hook->funcs = g_slist_remove(hook->funcs, func);
}

void
hook_call(const gchar *name, gpointer user_data)
{
    Hook *hook;
    GSList *iter;

    g_return_if_fail(name != NULL);

    hook = hook_find(name);

    if (hook == NULL)
        return;

    for (iter = hook->funcs; iter != NULL; iter = g_slist_next(iter))
    {
        HookFunction func = (HookFunction) iter->data;

        g_return_if_fail(func != NULL);

        func(user_data);
    }
}
