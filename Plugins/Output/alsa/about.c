/*  XMMS - ALSA output plugin
 *    Copyright (C) 2001-2003 Matthieu Sozeau <mattam@altern.org>
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

#include "alsa.h"
#include <xmms/util.h>

void alsa_about(void)
{
	static GtkWidget *dialog;

	if (dialog != NULL)
		return;
	
	dialog = xmms_show_message(
		_("About ALSA Driver"),
		_("XMMS ALSA Driver\n\n "
		  "This program is free software; you can redistribute it and/or modify\n"
		  "it under the terms of the GNU General Public License as published by\n"
		  "the Free Software Foundation; either version 2 of the License, or\n"
		  "(at your option) any later version.\n"
		  "\n"
		  "This program is distributed in the hope that it will be useful,\n"
		  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		  "GNU General Public License for more details.\n"
		  "\n"
		  "You should have received a copy of the GNU General Public License\n"
		  "along with this program; if not, write to the Free Software\n"
		  "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,\n"
		  "USA.\n"
		  "Author: Matthieu Sozeau (mattam@altern.org)"), _("OK"), FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &dialog);
}
