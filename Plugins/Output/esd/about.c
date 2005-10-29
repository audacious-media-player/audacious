/*      xmms - esound output plugin
 *    Copyright (C) 1999      Galex Yen
 *      3/9/99
 *      
 *      this program is free software
 *      
 *      Description:
 *              This program is an output plugin for xmms v0.9 or greater.
 *              The program uses the esound daemon to output audio in order
 *              to allow more than one program to play audio on the same
 *              device at the same time.
 *
 *              Contains code Copyright (C) 1998-1999 Mikael Alm, Olle Hallnas,
 *              Thomas Nillson and 4Front Technologies
 */

#include "esdout.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libaudacious/util.h>


void
esdout_about(void)
{
    static GtkWidget *dialog;

    if (dialog != NULL)
        return;

    dialog = xmms_show_message(_("About ESounD Plugin"),
                               _("XMMS ESounD Plugin\n\n "
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
                                 "USA."), _("Ok"), FALSE, NULL, NULL);
    g_signal_connect(G_OBJECT(dialog), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &dialog);
}
