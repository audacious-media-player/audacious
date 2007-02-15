/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef UI_SKINNED_WINDOW_H
#define UI_SKINNED_WINDOW_H

#define SKINNED_WINDOW(obj)          GTK_CHECK_CAST (obj, ui_skinned_window_get_type (), SkinnedWindow)
#define SKINNED_WINDOW_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, ui_skinned_window_get_type (), SkinnedWindowClass)
#define SKINNED_CHECK_WINDOW(obj)    GTK_CHECK_TYPE (obj, ui_skinned_window_get_type ())
#define SKINNED_TYPE_WINDOW          (ui_skinned_window_get_type())

typedef struct _SkinnedWindow SkinnedWindow;
typedef struct _SkinnedWindowClass SkinnedWindowClass;

struct _SkinnedWindow
{
  GtkWindow window;

  GtkWidget *canvas;
  gint x,y;

  GList *widget_list;
  GdkGC *gc;
};

struct _SkinnedWindowClass
{
  GtkWindowClass        parent_class;
};

extern GType ui_skinned_window_get_type(void);
extern GtkWidget *ui_skinned_window_new(GtkWindowType type, const gchar *wmclass_name);
extern void ui_skinned_window_widgetlist_associate(GtkWidget * widget, Widget * w);
extern void ui_skinned_window_widgetlist_dissociate(GtkWidget * widget, Widget * w);
extern gboolean ui_skinned_window_widgetlist_contained(GtkWidget * widget, gint x, gint y);

#endif
