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

#include <gtk/gtk.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void jump_cb (void * entry)
{
    const char * text = gtk_entry_get_text ((GtkEntry *) entry);
    unsigned minutes, seconds;

    if (sscanf (text, "%u:%u", & minutes, & seconds) == 2 && aud_drct_get_playing ())
        aud_drct_seek ((minutes * 60 + seconds) * 1000);
}

EXPORT void audgui_jump_to_time ()
{
    if (audgui_reshow_unique_window (AUDGUI_JUMP_TO_TIME_WINDOW))
        return;

    GtkWidget * entry = gtk_entry_new ();
    gtk_entry_set_activates_default ((GtkEntry *) entry, true);

    GtkWidget * button1 = audgui_button_new (_("_Jump"), "go-jump", jump_cb, entry);
    GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop", nullptr, nullptr);

    GtkWidget * dialog = audgui_dialog_new (GTK_MESSAGE_OTHER,
     _("Jump to Time"), _("Enter time (minutes:seconds):"), button1, button2);

    audgui_dialog_add_widget (dialog, entry);

    if (aud_drct_get_playing ())
    {
        int time = aud_drct_get_time () / 1000;
        gtk_entry_set_text ((GtkEntry *) entry, str_printf ("%u:%02u", time / 60, time % 60));
    }

    audgui_show_unique_window (AUDGUI_JUMP_TO_TIME_WINDOW, dialog);
}
