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

typedef void (*HookFunction)(void * hook_data, void * user_data);

typedef struct {
    HookFunction func;
    void * user_data;
} HookItem;

typedef struct {
    const char *name;
    GSList *items;
} Hook;

void hook_init (void);
void hook_register(const char *name);
int hook_associate(const char *name, HookFunction func, void * user_data);
int hook_dissociate(const char *name, HookFunction func);
int hook_dissociate_full(const char *name, HookFunction func, void * user_data);
void hook_call(const char *name, void * hook_data);

#endif /* AUDACIOUS_HOOK_H */
