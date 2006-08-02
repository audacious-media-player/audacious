/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef XMMS_ENTRY_H
#define XMMS_ENTRY_H


#include <gdk/gdk.h>
#include <gtk/gtkentry.h>


#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */


#define XMMS_TYPE_ENTRY            (xmms_entry_get_type())
#define XMMS_ENTRY(obj)            (GTK_CHECK_CAST((obj), XMMS_TYPE_ENTRY, XmmsEntry))
#define XMMS_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), XMMS_TYPE_ENTRY, XmmsEntryClass))
#define XMMS_IS_ENTRY(obj)         (GTK_CHECK_TYPE((obj), XMMS_TYPE_ENTRY))
#define XMMS_IS_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE((klass), XMMS_TYPE_ENTRY))


    typedef struct _XmmsEntry XmmsEntry;
    typedef struct _XmmsEntryClass XmmsEntryClass;

    struct _XmmsEntry {
        GtkEntry entry;
    };

    struct _XmmsEntryClass {
        GtkEntryClass parent_class;
    };

    GtkType xmms_entry_get_type(void);
    GtkWidget *xmms_entry_new(void);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* __GTK_ENTRY_H__ */
