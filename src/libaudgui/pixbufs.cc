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

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>

#include "internal.h"
#include "libaudgui-gtk.h"

static AudguiPixbuf current_pixbuf;

EXPORT AudguiPixbuf audgui_pixbuf_fallback ()
{
    static AudguiPixbuf fallback;

    if (! fallback)
        fallback.capture (gdk_pixbuf_new_from_file (filename_build
         ({aud_get_path (AudPath::DataDir), "images", "album.png"}), nullptr));

    return fallback.ref ();
}

void audgui_pixbuf_uncache ()
{
    current_pixbuf.clear ();
}

EXPORT void audgui_pixbuf_scale_within (AudguiPixbuf & pixbuf, int size)
{
    int width = pixbuf.width ();
    int height = pixbuf.height ();

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

    pixbuf.capture (gdk_pixbuf_scale_simple (pixbuf.get (), width, height, GDK_INTERP_BILINEAR));
}

EXPORT AudguiPixbuf audgui_pixbuf_request (const char * filename, bool * queued)
{
    AudArtPtr art = aud_art_request (filename, AUD_ART_DATA, queued);

    auto data = art.data ();
    return data ? audgui_pixbuf_from_data (data->begin (), data->len ()) : AudguiPixbuf ();
}

EXPORT AudguiPixbuf audgui_pixbuf_request_current (bool * queued)
{
    if (queued)
        * queued = false;

    if (! current_pixbuf)
    {
        String filename = aud_drct_get_filename ();
        if (filename)
            current_pixbuf = audgui_pixbuf_request (filename, queued);
    }

    return current_pixbuf.ref ();
}

EXPORT AudguiPixbuf audgui_pixbuf_from_data (const void * data, int64_t size)
{
    GdkPixbuf * pixbuf = nullptr;
    GdkPixbufLoader * loader = gdk_pixbuf_loader_new ();
    GError * error = nullptr;

    if (gdk_pixbuf_loader_write (loader, (const unsigned char *) data, size,
     & error) && gdk_pixbuf_loader_close (loader, & error))
    {
        if ((pixbuf = gdk_pixbuf_loader_get_pixbuf (loader)))
            g_object_ref (pixbuf);
    }
    else
    {
        AUDWARN ("While loading pixbuf: %s\n", error->message);
        g_error_free (error);
    }

    g_object_unref (loader);
    return AudguiPixbuf (pixbuf);
}
