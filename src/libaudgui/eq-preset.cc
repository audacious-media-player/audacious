/*
 * eq-preset.c
 * Copyright 2015 John Lindgren
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

#include <libaudcore/i18n.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static GtkWidget * create_eq_preset_window ()
{
    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window, _("Equalizer Presets"));
    gtk_window_set_type_hint ((GtkWindow *) window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_container_set_border_width ((GtkContainer *) window, 6);
    audgui_destroy_on_escape (window);

    return window;
}

EXPORT void audgui_show_eq_preset_window ()
{
    if (! audgui_reshow_unique_window (AUDGUI_EQ_PRESET_WINDOW))
        audgui_show_unique_window (AUDGUI_EQ_PRESET_WINDOW, create_eq_preset_window ());
}
