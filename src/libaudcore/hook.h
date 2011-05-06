/*  Audacious
 *  Copyright (c) 2006-2007 William Pitcock
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef AUDACIOUS_HOOK_H
#define AUDACIOUS_HOOK_H

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

void hook_init (void);
void hook_register(const gchar *name);
gint hook_associate(const gchar *name, HookFunction func, gpointer user_data);
gint hook_dissociate(const gchar *name, HookFunction func);
gint hook_dissociate_full(const gchar *name, HookFunction func, gpointer user_data);
void hook_call(const gchar *name, gpointer hook_data);

#endif /* AUDACIOUS_HOOK_H */
