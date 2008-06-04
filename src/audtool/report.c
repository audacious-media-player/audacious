/*
 * Audtool2
 * Copyright (c) 2007 Audacious development team
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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <mowgli.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void audtool_report(const gchar *str, ...)
{
	gchar *buf;
	va_list va;

	va_start(va, str);
	buf = g_strdup_vprintf(str, va);
	va_end(va);

	g_print("%s\n", buf);
	g_free(buf);
}

void audtool_whine(const gchar *str, ...)
{
	gchar *buf;
	va_list va;

	va_start(va, str);
	buf = g_strdup_vprintf(str, va);
	va_end(va);

	g_printerr("audtool: %s", buf);
	g_free(buf);
}

void audtool_whine_args(const gchar *name, const gchar *fmt, ...)
{
	gchar *buf;
	va_list va;

	va_start(va, fmt);
	buf = g_strdup_vprintf(fmt, va);
	va_end(va);

	g_printerr("audtool: Invalid parameters for %s\n", name);
	g_printerr(" syntax: %s %s\n", name, buf);
	g_free(buf);
}

void audtool_whine_tuple_fields(void)
{
    gint nfields, i;
    gchar **fields = audacious_remote_get_tuple_fields(dbus_proxy),
          **tmp = fields;
    
    audtool_whine("Field names include, but are not limited to:\n");
    
    for (nfields = 0; tmp && *tmp; nfields++, tmp++);
    
    tmp = fields;
    i = 0;
    g_printerr("         ");
    while (tmp && *tmp) {
        i += g_utf8_strlen(*tmp, -1);
        if (i > 45) {
            g_printerr("\n         ");
            i = 0;
        }
        g_printerr("%s", *tmp);
        if (--nfields > 0)
            g_printerr(", ");
        
        g_free(*tmp);
        tmp++;
    }
    
    g_printerr("\n");
    
    g_free(fields);
}

