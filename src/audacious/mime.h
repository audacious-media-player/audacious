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
 */

#include <glib.h>
#include <mowgli.h>

#include <audacious/plugin.h>

#ifndef __AUDACIOUS_MIME_H__
#define __AUDACIOUS_MIME_H__

G_BEGIN_DECLS

InputPlugin *mime_get_plugin(const gchar *mimetype);
void mime_set_plugin(const gchar *mimetype, InputPlugin *ip);

G_END_DECLS

#endif
