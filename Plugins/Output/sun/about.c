/*
 *  Copyright (C) 2001  CubeSoft Communications, Inc.
 *  <http://www.csoft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "sun.h"
#include "libaudacious/util.h"

#include <glib.h>
#include <glib/gi18n.h>

void sun_about(void)
{
	static GtkWidget *dialog;

	if (dialog != NULL)
		return;

	dialog = xmms_show_message(
		_("About the Sun Driver"),
		_("XMMS BSD Sun Driver\n\n"
		  "Copyright (c) 2001 CubeSoft Communications, Inc.\n"
		  "Maintainer: <vedge at csoft.org>.\n"),
		_("Ok"), FALSE, NULL, NULL);

	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &dialog);
}

