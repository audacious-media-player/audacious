/*
 * audacious: Cross-platform multimedia player.
 * rcfile.h: Reading and manipulation of .ini-like files.
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

#ifndef RCFILE_H
#define RCFILE_H

#include <glib.h>

/**
 * RcLine:
 * @key: A key for the key->value mapping.
 * @value: A value for the key->value mapping.
 *
 * RcLine objects contain key->value mappings.
 **/
typedef struct {
    gchar *key;
    gchar *value;
} RcLine;

/**
 * RcSection:
 * @name: The name for the #RcSection.
 * @lines: A list of key->value mappings for the #RcSection.
 *
 * RcSection objects contain collections of key->value mappings.
 **/
typedef struct {
    gchar *name;
    GList *lines;
} RcSection;

/**
 * RcFile:
 * @sections: A list of sections.
 *
 * An RcFile object contains a collection of key->value mappings organized by section.
 **/
typedef struct {
    GList *sections;
} RcFile;

G_BEGIN_DECLS

RcFile *bmp_rcfile_new(void);
void bmp_rcfile_free(RcFile * file);

RcFile *bmp_rcfile_open(const gchar * filename);
gboolean bmp_rcfile_write(RcFile * file, const gchar * filename);

gboolean bmp_rcfile_read_string(RcFile * file, const gchar * section,
                                const gchar * key, gchar ** value);
gboolean bmp_rcfile_read_int(RcFile * file, const gchar * section,
                             const gchar * key, gint * value);
gboolean bmp_rcfile_read_bool(RcFile * file, const gchar * section,
                              const gchar * key, gboolean * value);
gboolean bmp_rcfile_read_float(RcFile * file, const gchar * section,
                               const gchar * key, gfloat * value);
gboolean bmp_rcfile_read_double(RcFile * file, const gchar * section,
                                const gchar * key, gdouble * value);

void bmp_rcfile_write_string(RcFile * file, const gchar * section,
                             const gchar * key, const gchar * value);
void bmp_rcfile_write_int(RcFile * file, const gchar * section,
                          const gchar * key, gint value);
void bmp_rcfile_write_boolean(RcFile * file, const gchar * section,
                              const gchar * key, gboolean value);
void bmp_rcfile_write_float(RcFile * file, const gchar * section,
                            const gchar * key, gfloat value);
void bmp_rcfile_write_double(RcFile * file, const gchar * section,
                             const gchar * key, gdouble value);

void bmp_rcfile_remove_key(RcFile * file, const gchar * section,
                           const gchar * key);

G_END_DECLS

#endif // RCFILE_H
