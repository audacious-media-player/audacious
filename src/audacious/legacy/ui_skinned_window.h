/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
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

#ifndef AUDACIOUS_UI_SKINNED_WINDOW_H
#define AUDACIOUS_UI_SKINNED_WINDOW_H

#define SKINNED_WINDOW(obj)          GTK_CHECK_CAST (obj, ui_skinned_window_get_type (), SkinnedWindow)
#define SKINNED_WINDOW_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, ui_skinned_window_get_type (), SkinnedWindowClass)
#define SKINNED_CHECK_WINDOW(obj)    GTK_CHECK_TYPE (obj, ui_skinned_window_get_type ())
#define SKINNED_TYPE_WINDOW          (ui_skinned_window_get_type())

#ifdef GDK_WINDOWING_QUARTZ
# define SKINNED_WINDOW_TYPE		GTK_WINDOW_POPUP
#else
# define SKINNED_WINDOW_TYPE		GTK_WINDOW_TOPLEVEL
#endif

enum {
    WINDOW_MAIN,
    WINDOW_EQ,
    WINDOW_PLAYLIST
};

typedef struct _SkinnedWindow SkinnedWindow;
typedef struct _SkinnedWindowClass SkinnedWindowClass;

struct _SkinnedWindow
{
  GtkWindow window;

  GtkWidget *canvas;
  gint x,y;

  gint type;
  GtkWidget *fixed;
};

struct _SkinnedWindowClass
{
  GtkWindowClass        parent_class;
};

extern GType ui_skinned_window_get_type(void);
extern GtkWidget *ui_skinned_window_new(const gchar *wmclass_name);
extern void ui_skinned_window_draw_all(GtkWidget *widget);

#endif /* AUDACIOUS_UI_SKINNED_WINDOW_H */
