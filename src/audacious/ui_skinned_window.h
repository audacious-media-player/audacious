/*
 * audacious: Cross-platform multimedia player.
 * ui_skinned_window.h: Common code for skinned UI windows.        
 *
 * Copyright (c) 2005-2007 Audacious development team.
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
  GtkWidget *fixed;
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
