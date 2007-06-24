/*
 * audacious: Cross-platform multimedia player.
 * hook.c: Hooking.
 *
 * Copyright (c) 2006-2007 William Pitcock
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
    hook->items = NULL;

    hook_list = g_slist_append(hook_list, hook);
}

gint
hook_associate(const gchar *name, HookFunction func, gpointer user_data)
{
    Hook *hook;
    HookItem *hookitem;

    g_return_val_if_fail(name != NULL, -1);
    g_return_val_if_fail(func != NULL, -1);

    hook = hook_find(name);

    if (hook == NULL)
    {
        hook_register(name);
        hook = hook_find(name);
    }

    /* this *cant* happen */
    g_return_val_if_fail(hook != NULL, -1);

    hookitem = g_new0(HookItem, 1);
    hookitem->func = func;
    hookitem->user_data = user_data;

    hook->items = g_slist_append(hook->items, hookitem);
    return 0;
}

gint
hook_dissociate(const gchar *name, HookFunction func)
{
    Hook *hook;
    GSList *iter;

    g_return_val_if_fail(name != NULL, -1);
    g_return_val_if_fail(func != NULL, -1);

    hook = hook_find(name);

    if (hook == NULL)
        return -1;

    iter = hook->items;
    while (iter != NULL)
    {
        HookItem *hookitem = (HookItem*)iter->data;
        if (hookitem->func == func)
        {
            hook->items = g_slist_delete_link(hook->items, iter);
            g_free( hookitem );
            return 0;
        }
        iter = g_slist_next(iter);
    }
    return -1;
}

void
hook_call(const gchar *name, gpointer hook_data)
{
    Hook *hook;
    GSList *iter;

    g_return_if_fail(name != NULL);

    hook = hook_find(name);

    if (hook == NULL)
        return;

    for (iter = hook->items; iter != NULL; iter = g_slist_next(iter))
    {
        HookItem *hookitem = (HookItem*)iter->data;

        g_return_if_fail(hookitem->func != NULL);

        hookitem->func(hook_data, hookitem->user_data);
    }
}
