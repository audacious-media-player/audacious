/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef BMP_GLADE_H
#define BMP_GLADE_H

#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>


typedef struct _FuncMap FuncMap;

struct _FuncMap {
    gchar *name;
    GCallback function;
};


#define FUNC_MAP_BEGIN(map)       static FuncMap map[] = {
#define FUNC_MAP_ENTRY(function)  { #function, (GCallback) function },
#define FUNC_MAP_END              { NULL, NULL } };


GladeXML *glade_xml_new_or_die(const gchar * name, const gchar * path,
                               const gchar * root, const gchar * domain);

GtkWidget *glade_xml_get_widget_warn(GladeXML * xml, const gchar * name);

void glade_xml_signal_autoconnect_map(GladeXML * xml, FuncMap * map);

#endif
