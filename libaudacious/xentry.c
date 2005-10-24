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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

/*
 * Small modification of the entry widget where keyboard navigation
 * works even when the entry is not editable.
 * Copyright 2003 Haavard Kvaalen <havardk@xmms.org>
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <ctype.h>
#include <string.h>

#include "xentry.h"

static gint gtk_entry_key_press(GtkWidget * widget, GdkEventKey * event);
static void gtk_entry_move_cursor(GtkOldEditable * editable, int x);

static void gtk_move_forward_character(GtkEntry * entry);
static void gtk_move_backward_character(GtkEntry * entry);
static void gtk_move_forward_word(GtkEntry * entry);
static void gtk_move_backward_word(GtkEntry * entry);
static void gtk_move_beginning_of_line(GtkEntry * entry);
static void gtk_move_end_of_line(GtkEntry * entry);


static const GtkTextFunction control_keys[26] = {
    (GtkTextFunction) gtk_move_beginning_of_line,   /* a */
    (GtkTextFunction) gtk_move_backward_character,  /* b */
    (GtkTextFunction) gtk_editable_copy_clipboard,  /* c */
    NULL,                       /* d */
    (GtkTextFunction) gtk_move_end_of_line, /* e */
    (GtkTextFunction) gtk_move_forward_character,   /* f */
};

static const GtkTextFunction alt_keys[26] = {
    NULL,                       /* a */
    (GtkTextFunction) gtk_move_backward_word,   /* b */
    NULL,                       /* c */
    NULL,                       /* d */
    NULL,                       /* e */
    (GtkTextFunction) gtk_move_forward_word,    /* f */
};


static void
xmms_entry_class_init(GtkEntryClass * class)
{
    GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

    widget_class->key_press_event = gtk_entry_key_press;
}

GtkType
xmms_entry_get_type(void)
{
    static GtkType entry_type = 0;

    if (!entry_type) {
        static const GtkTypeInfo entry_info = {
            "XmmsEntry",
            sizeof(XmmsEntry),
            sizeof(XmmsEntryClass),
            (GtkClassInitFunc) xmms_entry_class_init,
            NULL,
            /* reserved_1 */ NULL,
            /* reserved_2 */ NULL,
            (GtkClassInitFunc) NULL,
        };

        entry_type = gtk_type_unique(GTK_TYPE_ENTRY, &entry_info);
    }

    return entry_type;
}

GtkWidget *
xmms_entry_new(void)
{
    return GTK_WIDGET(gtk_type_new(XMMS_TYPE_ENTRY));
}

static int
gtk_entry_key_press(GtkWidget * widget, GdkEventKey * event)
{
    GtkEntry *entry;
    GtkOldEditable *editable;

    int return_val;
    guint initial_pos, sel_start_pos, sel_end_pos;
    int extend_selection;
    gboolean extend_start = FALSE;

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(XMMS_IS_ENTRY(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    entry = GTK_ENTRY(widget);
    editable = GTK_OLD_EDITABLE(widget);
    return_val = FALSE;

    if (editable->editable)
        /* Let the regular entry handler do it */
        return FALSE;

    initial_pos = gtk_editable_get_position(GTK_EDITABLE(editable));

    extend_selection = event->state & GDK_SHIFT_MASK;

    sel_start_pos = editable->selection_start_pos;
    sel_end_pos = editable->selection_end_pos;

    if (extend_selection) {
        if (sel_start_pos == sel_end_pos) {
            sel_start_pos = editable->current_pos;
            sel_end_pos = editable->current_pos;
        }

        extend_start = (editable->current_pos == sel_start_pos);
    }

    switch (event->keyval) {
    case GDK_Insert:
        return_val = TRUE;
        if (event->state & GDK_CONTROL_MASK)
            gtk_editable_copy_clipboard(GTK_EDITABLE(editable));
        break;
    case GDK_Home:
        return_val = TRUE;
        gtk_move_beginning_of_line(entry);
        break;
    case GDK_End:
        return_val = TRUE;
        gtk_move_end_of_line(entry);
        break;
    case GDK_Left:
        return_val = TRUE;
        if (!extend_selection && sel_start_pos != sel_end_pos) {
            gtk_editable_set_position(GTK_EDITABLE(editable),
                                      MIN(sel_start_pos, sel_end_pos));
            /* Force redraw below */
            initial_pos = -1;
        }
        else
            gtk_move_backward_character(entry);
        break;
    case GDK_Right:
        return_val = TRUE;
        if (!extend_selection && sel_start_pos != sel_end_pos) {
            gtk_editable_set_position(GTK_EDITABLE(editable),
                                      MAX(sel_start_pos, sel_end_pos));
            /* Force redraw below */
            initial_pos = -1;
        }
        else
            gtk_move_forward_character(entry);
        break;
    case GDK_Return:
        return_val = TRUE;
        gtk_widget_activate(widget);
        break;
    default:
        if ((event->keyval >= 0x20) && (event->keyval <= 0xFF)) {
            int key = event->keyval;

            if (key >= 'A' && key <= 'Z')
                key -= 'A' - 'a';

            if (key >= 'a' && key <= 'z')
                key -= 'a';
            else
                break;

            if (event->state & GDK_CONTROL_MASK) {
                if (control_keys[key]) {
                    (*control_keys[key]) (editable, event->time);
                    return_val = TRUE;
                }
                break;
            }
            else if (event->state & GDK_MOD1_MASK) {
                if (alt_keys[key]) {
                    (*alt_keys[key]) (editable, event->time);
                    return_val = TRUE;
                }
                break;
            }
        }
    }

    if (return_val && (editable->current_pos != initial_pos)) {
        if (extend_selection) {
            int cpos = gtk_editable_get_position(GTK_EDITABLE(editable));
            if (cpos < sel_start_pos)
                sel_start_pos = cpos;
            else if (cpos > sel_end_pos)
                sel_end_pos = cpos;
            else {
                if (extend_start)
                    sel_start_pos = cpos;
                else
                    sel_end_pos = cpos;
            }
        }
        else {
            sel_start_pos = 0;
            sel_end_pos = 0;
        }

        gtk_editable_select_region(GTK_EDITABLE(editable), sel_start_pos,
                                   sel_end_pos);
    }

    return return_val;
}

static void
gtk_entry_move_cursor(GtkOldEditable * editable, int x)
{
    int set, pos = gtk_editable_get_position(GTK_EDITABLE(editable));
    if (pos + x < 0)
        set = 0;
    else
        set = pos + x;
    gtk_editable_set_position(GTK_EDITABLE(editable), set);
}

static void
gtk_move_forward_character(GtkEntry * entry)
{
    gtk_entry_move_cursor(GTK_OLD_EDITABLE(entry), 1);
}

static void
gtk_move_backward_character(GtkEntry * entry)
{
    gtk_entry_move_cursor(GTK_OLD_EDITABLE(entry), -1);
}

static void
gtk_move_forward_word(GtkEntry * entry)
{
    GtkOldEditable *editable;
    GdkWChar *text;
    int i;

    editable = GTK_OLD_EDITABLE(entry);

    /* Prevent any leak of information */
    if (!editable->visible) {
        gtk_editable_set_position(GTK_EDITABLE(entry), -1);
        return;
    }

    if (entry->text && (editable->current_pos < entry->text_length)) {
        text = (GdkWChar *) entry->text;
        i = editable->current_pos;

/*		if ((entry->use_wchar && !gdk_iswalnum(text[i])) ||
		    !isalnum(text[i]))
			for (; i < entry->text_length; i++)
			{
				if (entry->use_wchar)
				{
					if (gdk_iswalnum(text[i]))
						break;
					else if (isalnum(text[i]))
						break;
				}
			}

		for (; i < entry->text_length; i++)
		{
			if (entry->use_wchar)
			{
				if (gdk_iswalnum(text[i]))
					break;
				else if (isalnum(text[i]))
					break;
			}
		}

*/

        gtk_editable_set_position(GTK_EDITABLE(entry), i);
    }
}

static void
gtk_move_backward_word(GtkEntry * entry)
{
    GtkOldEditable *editable;
    GdkWChar *text;
    int i;

    editable = GTK_OLD_EDITABLE(entry);

    /* Prevent any leak of information */
    if (!editable->visible) {
        gtk_editable_set_position(GTK_EDITABLE(entry), 0);
        return;
    }

    if (entry->text && editable->current_pos > 0) {
        text = (GdkWChar *) entry->text;
        i = editable->current_pos;

/*		if ((entry->use_wchar && !gdk_iswalnum(text[i])) ||
		    !isalnum(text[i]))
			for (; i >= 0; i--)
			{
				if (entry->use_wchar)
				{
					if (gdk_iswalnum(text[i]))
						break;
					else if (isalnum(text[i]))
						break;
				}
			}
		for (; i >= 0; i--)
		{
			if ((entry->use_wchar && !gdk_iswalnum(text[i])) ||
			    !isalnum(text[i]))
			{
				i++;
				break;
			}
		}
*/
        if (i < 0)
            i = 0;

        gtk_editable_set_position(GTK_EDITABLE(entry), i);
    }
}

static void
gtk_move_beginning_of_line(GtkEntry * entry)
{
    gtk_editable_set_position(GTK_EDITABLE(entry), 0);
}

static void
gtk_move_end_of_line(GtkEntry * entry)
{
    gtk_editable_set_position(GTK_EDITABLE(entry), -1);
}
