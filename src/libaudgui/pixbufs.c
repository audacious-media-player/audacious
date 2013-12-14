/*
 * pixbufs.c
 * Copyright 2010-2012 John Lindgren
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <audacious/debug.h>
#include <audacious/misc.h>
#include <audacious/playlist.h>
#include <libaudcore/audstrings.h>

#include "libaudgui-gtk.h"

static GdkPixbuf * current_pixbuf;

EXPORT GdkPixbuf * audgui_pixbuf_fallback (void)
{
    static GdkPixbuf * fallback = NULL;

    if (! fallback)
    {
        const char * data_dir = aud_get_path (AUD_PATH_DATA_DIR);
        SCONCAT2 (path, data_dir, "/images/album.png");
        fallback = gdk_pixbuf_new_from_file (path, NULL);
    }

    if (fallback)
        g_object_ref ((GObject *) fallback);

    return fallback;
}

void audgui_pixbuf_uncache (void)
{
    if (current_pixbuf)
    {
        g_object_unref ((GObject *) current_pixbuf);
        current_pixbuf = NULL;
    }
}

EXPORT void audgui_pixbuf_scale_within (GdkPixbuf * * pixbuf, int size)
{
    int width = gdk_pixbuf_get_width (* pixbuf);
    int height = gdk_pixbuf_get_height (* pixbuf);

    if (width <= size && height <= size)
        return;

    if (width > height)
    {
        height = size * height / width;
        width = size;
    }
    else
    {
        width = size * width / height;
        height = size;
    }

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;

    GdkPixbuf * pixbuf2 = gdk_pixbuf_scale_simple (* pixbuf, width, height, GDK_INTERP_BILINEAR);
    g_object_unref (* pixbuf);
    * pixbuf = pixbuf2;
}

EXPORT GdkPixbuf * audgui_pixbuf_request (const char * filename)
{
    const void * data;
    int64_t size;

    aud_art_request_data (filename, & data, & size);
    if (! data)
        return NULL;

    GdkPixbuf * p = audgui_pixbuf_from_data (data, size);

    aud_art_unref (filename);
    return p;
}

EXPORT GdkPixbuf * audgui_pixbuf_request_current (void)
{
    if (! current_pixbuf)
    {
        int list = aud_playlist_get_playing ();
        int entry = aud_playlist_get_position (list);
        if (entry < 0)
            return NULL;

        char * filename = aud_playlist_entry_get_filename (list, entry);
        current_pixbuf = audgui_pixbuf_request (filename);
        str_unref (filename);
    }

    if (current_pixbuf)
        g_object_ref ((GObject *) current_pixbuf);

    return current_pixbuf;
}

EXPORT GdkPixbuf * audgui_pixbuf_from_data (const void * data, int64_t size)
{
    GdkPixbuf * pixbuf = NULL;
    GdkPixbufLoader * loader = gdk_pixbuf_loader_new ();
    GError * error = NULL;

    if (gdk_pixbuf_loader_write (loader, data, size, & error) &&
     gdk_pixbuf_loader_close (loader, & error))
    {
        if ((pixbuf = gdk_pixbuf_loader_get_pixbuf (loader)))
            g_object_ref (pixbuf);
    }
    else
    {
        AUDDBG ("error while loading pixbuf: %s\n", error->message);
        g_error_free (error);
    }

    g_object_unref (loader);
    return pixbuf;
}
