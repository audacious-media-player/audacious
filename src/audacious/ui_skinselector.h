/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#ifndef AUDACIOUS_UI_SKINSELECTOR_H
#define AUDACIOUS_UI_SKINSELECTOR_H

#include <glib.h>
#include <gtk/gtk.h>

#define SKIN_NODE(x)  ((SkinNode *)(x))
struct _SkinNode {
    gchar *name;
    gchar *desc;
    gchar *path;
    GTime *time;
};

typedef struct _SkinNode SkinNode;

extern GList *skinlist;

void skinlist_update();
void skin_view_realize(GtkTreeView * treeview);
void skin_view_update(GtkTreeView * treeview, GtkWidget * refresh_button);

#endif /* AUDACIOUS_UI_SKINSELECTOR_H */
