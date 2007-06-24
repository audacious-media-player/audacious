/*
 * audacious: Cross-platform multimedia player.
 * dnd.h: Drag and Drop code.             
 *
 * Copyright (c) 2005-2007 Audacious development team.
 * Copyright (c) 2003-2005 BMP development team.
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

#ifndef DND_H
#define DND_H

#include <gtk/gtk.h>

/* Designate dropped data types that we know and care about */
enum {
    BMP_DROP_STRING,
    BMP_DROP_PLAINTEXT,
    BMP_DROP_URLENCODED,
    BMP_DROP_SKIN,
    BMP_DROP_FONT
};

/* Drag data format listing for gtk_drag_dest_set() */
static const GtkTargetEntry bmp_drop_types[] = {
    {"text/plain", 0, BMP_DROP_PLAINTEXT},
    {"text/uri-list", 0, BMP_DROP_URLENCODED},
    {"STRING", 0, BMP_DROP_STRING},
    {"interface/x-winamp-skin", 0, BMP_DROP_SKIN},
    {"application/x-font-ttf", 0, BMP_DROP_FONT},
};

void bmp_drag_dest_set(GtkWidget*);

#endif
