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

#ifndef __AUDACIOUS_HOOK_H__
#define __AUDACIOUS_HOOK_H__

#include <glib.h>

typedef void (*HookFunction)(gpointer hook_data, gpointer user_data);

typedef struct {
    HookFunction func;
    gpointer user_data;
} HookItem;

typedef struct {
    const gchar *name;
    GSList *items;
} Hook;

void hook_register(const gchar *name);
gint hook_associate(const gchar *name, HookFunction func, gpointer user_data);
gint hook_dissociate(const gchar *name, HookFunction func);
void hook_call(const gchar *name, gpointer hook_data);

#endif
