/*
 * icons-stock.c
 * Copyright 2007-2010 Michael Färber and John Lindgren
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
#include <audacious/misc.h>

#include "libaudgui.h"

static void load_stock_icon (char * id, char * filename,
 GtkIconFactory * iconfactory)
{
    char * path = g_strdup_printf ("%s/images/%s",
     aud_get_path (AUD_PATH_DATA_DIR), filename);

    GdkPixbuf * pixbuf = gdk_pixbuf_new_from_file (path, NULL);
    if (pixbuf == NULL)
        goto ERR;

    GtkIconSet * iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    gtk_icon_factory_add(iconfactory, id, iconset);

ERR:
    g_free (path);
}

EXPORT void
audgui_register_stock_icons(void)
{
    GtkIconFactory *iconfactory = gtk_icon_factory_new();

    load_stock_icon(AUD_STOCK_AUDACIOUS,
                    "audacious.png", iconfactory);
    load_stock_icon(AUD_STOCK_PLAYLIST,
                    "menu_playlist.png", iconfactory);
    load_stock_icon(AUD_STOCK_PLUGIN,
                    "menu_plugin.png", iconfactory);
    load_stock_icon(AUD_STOCK_QUEUETOGGLE,
                    "menu_queue_toggle.png", iconfactory);

    gtk_icon_factory_add_default( iconfactory );
    g_object_unref( iconfactory );
}
