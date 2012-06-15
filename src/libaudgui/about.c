/*
 * about.c
 * Copyright 2012 Thomas Lange
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

#include <gtk/gtk.h>

#include <audacious/i18n.h>
#include <audacious/misc.h>

#include "config.h"

static GtkAboutDialog * about;

static const gchar * authors[] =
{
    N_("Core developers:"),
    "Christian Birchinger",
    "Michael Färber",
    "Matti Hämäläinen",
    "John Lindgren",
    "Michał Lipski",
    "Cristi Măgherușan",
    "Tomasz Moń",
    "William Pitcock",
    "Jonathan Schleifer",
    "Ben Tucker",
    "Tony Vroon",
    "Yoshiki Yazawa\n",

    N_("Winamp skins:"),
    "George Averill",
    "Michael Färber",
    "William Pitcock\n",

    N_("Plugin development:"),
    "Kiyoshi Aman",
    "Luca Barbato",
    "Daniel Barkalow",
    "Michael Färber",
    "Shay Green",
    "Matti Hämäläinen",
    "Sascha Hlusiak",
    "John Lindgren",
    "Michał Lipski",
    "Giacomo Lozito",
    "Cristi Măgherușan",
    "Boris Mikhaylov",
    "Tomasz Moń",
    "Sebastian Pipping",
    "William Pitcock",
    "Derek Pomery",
    "Jonathan Schleifer",
    "Andrew Shadura",
    "Tony Vroon",
    "Yoshiki Yazawa\n",

    N_("Patch authors:"),
    "Chris Arepantis",
    "Anatoly Arzhnikov",
    "Alexis Ballier",
    "Eric Barch",
    "Carlo Bramini",
    "Massimo Cavalleri",
    "Stefano D'Angelo",
    "Jean-Louis Dupond",
    "Laszlo Dvornik",
    "Ralf Ertzinger",
    "Mike Frysinger",
    "Mark Glines",
    "Hans de Goede",
    "David Guglielmi",
    "Michael Hanselmann",
    "Juho Heikkinen",
    "Joseph Jezak",
    "Henrik Johansson",
    "Jussi Judin",
    "Teru Kamogashira",
    "Chris Kehler",
    "Thomas Lange",
    "Mark Loeser",
    "Alex Maclean",
    "Mikael Magnusson",
    "Rodrigo Martins de Matos Ventura",
    "Mihai Maruseac",
    "Diego Pettenò",
    "Mike Ryan",
    "Michael Schwendt",
    "Edward Sheldrake",
    "Kirill Shendrikowski",
    "Kazuki Shimura",
    "Valentine Sinitsyn",
    "Will Storey",
    "Johan Tavelin",
    "Christoph J. Thompson",
    "Bret Towe",
    "Peter Wagner",
    "John Wehle",
    "Ben Wolfson",
    "Tim Yamin",
    "Ivan N. Zlatev\n",

    N_("1.x developers:"),
    "George Averill",
    "Daniel Barkalow",
    "Christian Birchinger",
    "Daniel Bradshaw",
    "Adam Cecile",
    "Michael Färber",
    "Matti Hämäläinen",
    "Troels Bang Jensen",
    "Giacomo Lozito",
    "Cristi Măgherușan",
    "Tomasz Moń",
    "William Pitcock",
    "Derek Pomery",
    "Mohammed Sameer",
    "Jonathan Schleifer",
    "Ben Tucker",
    "Tony Vroon",
    "Yoshiki Yazawa",
    "Eugene Zagidullin\n",

    N_("BMP Developers:"),
    "Artem Baguinski",
    "Edward Brocklesby",
    "Chong Kai Xiong",
    "Milosz Derezynski",
    "David Lau",
    "Ole Andre Vadla Ravnaas",
    "Michiel Sikkes",
    "Andrei Badea",
    "Peter Behroozi",
    "Bernard Blackham",
    "Oliver Blin",
    "Tomas Bzatek",
    "Liviu Danicel",
    "Jon Dowland",
    "Artur Frysiak",
    "Sebastian Kapfer",
    "Lukas Koberstein",
    "Dan Korostelev",
    "Jolan Luff",
    "Michael Marineau",
    "Tim-Philipp Muller",
    "Julien Portalier",
    "Andrew Ruder",
    "Olivier Samyn",
    "Martijn Vernooij",

    NULL
};

static const gchar * lic[] =
{
    ("Audacious is free software; you can redistribute it and/or modify "
     "it under the terms of the GNU General Public License as published by "
     "the Free Software Foundation; either version 2 of the License, or "
     "(at your option) any later version.\n\n"),
    ("Audacious is distributed in the hope that it will be useful, "
     "but WITHOUT ANY WARRANTY; without even the implied warranty of "
     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
     "GNU General Public License for more details.\n\n"),
    ("You should have received a copy of the GNU General Public License "
     "along with Audacious; if not, write to the Free Software Foundation, Inc., "
     "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.\n")
};

static const gchar * artists[] =
{
    "George Averill",
    "Stephan Sokolow",
    "http://tango.freedesktop.org/",
    NULL
};

static void about_dialog_result (GtkWidget * widget, gint response, gpointer data)
{
    if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT)
    {
        gtk_widget_destroy (widget);
        about = NULL;
    }
}

EXPORT void audgui_show_about_window (void)
{
    if (about != NULL)
    {
        gtk_window_present ((GtkWindow *) about);
        return;
    }

    GdkPixbuf * logo;
    gchar * path, * license;

    path = g_strdup_printf ("%s/images/about-logo.png", aud_get_path (AUD_PATH_DATA_DIR));
    logo = gdk_pixbuf_new_from_file (path, NULL);
    license = g_strconcat (lic[0], lic[1], lic[2], NULL);
    g_free (path);

    about = ((GtkAboutDialog *) gtk_about_dialog_new ());
    gtk_about_dialog_set_program_name (about, "Audacious");
    gtk_about_dialog_set_version (about, VERSION);
    gtk_about_dialog_set_copyright (about, _("Copyright \xc2\xa9 2005-2012 Audacious Team"));
    gtk_about_dialog_set_comments (about, _("A lightweight audio player"));
    gtk_about_dialog_set_license (about, license);
    gtk_about_dialog_set_wrap_license (about, TRUE);
    gtk_about_dialog_set_website (about, "http://audacious-media-player.org");
    gtk_about_dialog_set_website_label (about, _("Homepage"));
    gtk_about_dialog_set_artists (about, artists);
    gtk_about_dialog_set_authors (about, authors);
    gtk_about_dialog_set_translator_credits (about, _("translator-credits"));
    gtk_about_dialog_set_logo (about, logo);

    g_signal_connect (about, "response", (GCallback) about_dialog_result, NULL);

    gtk_widget_show_all ((GtkWidget *) about);

    g_object_unref (logo);
    g_free (license);
}
