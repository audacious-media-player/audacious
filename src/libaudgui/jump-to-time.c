/*
 * jump-to-time.c
 * Copyright 2012-2013 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <stdio.h>
#include <gtk/gtk.h>

#include <audacious/drct.h>
#include <audacious/i18n.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void jump_cb (void * entry)
{
    const char * text = gtk_entry_get_text ((GtkEntry *) entry);
    unsigned minutes, seconds;

    if (sscanf (text, "%u:%u", & minutes, & seconds) == 2 && aud_drct_get_playing ())
        aud_drct_seek ((minutes * 60 + seconds) * 1000);
}

EXPORT void audgui_jump_to_time (void)
{
    GtkWidget * entry = gtk_entry_new ();
    gtk_entry_set_activates_default ((GtkEntry *) entry, TRUE);

    GtkWidget * button1 = audgui_button_new (_("Jump"), "go-jump", jump_cb, entry);
    GtkWidget * button2 = audgui_button_new (_("Cancel"), NULL, NULL, NULL);

    GtkWidget * dialog = audgui_dialog_new (GTK_MESSAGE_OTHER,
     _("Jump to Time"), _("Enter time (minutes:seconds):"), button1, button2);

    GtkWidget * box = gtk_message_dialog_get_message_area ((GtkMessageDialog *) dialog);
    gtk_box_pack_start ((GtkBox *) box, entry, FALSE, FALSE, 0);

    if (aud_drct_get_playing ())
    {
        int time = aud_drct_get_time () / 1000;
        SPRINTF (buf, "%u:%02u", time / 60, time % 60);
        gtk_entry_set_text ((GtkEntry *) entry, buf);
    }

    gtk_widget_show_all (dialog);
}
