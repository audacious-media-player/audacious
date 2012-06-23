/*
 * report.c
 * Copyright 2007-2008 William Pitcock and Matti Hämäläinen
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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
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

