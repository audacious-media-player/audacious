#include "libaudacious/util.h"
#include "libaudacious/configdb.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "md5.h"

static int errorbox_done;
void about_show(void)
{
	static GtkWidget *aboutbox;
	gchar *tmp;
	if (aboutbox)
		return;

	tmp = g_strdup_printf("Audacious AudioScrobbler Plugin\n\n"
				"Originally created by Audun Hove <audun@nlc.no> and Pipian <pipian@pipian.com>\n");
	aboutbox = xmms_show_message(_("About Scrobbler Plugin"),
			_(tmp),
			_("Ok"), FALSE, NULL, NULL);

	g_free(tmp);
	gtk_signal_connect(GTK_OBJECT(aboutbox), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &aboutbox);
}

static void set_errorbox_done(GtkWidget *errorbox, GtkWidget **errorboxptr)
{
	errorbox_done = -1;
	gtk_widget_destroyed(errorbox, errorboxptr);
}

void init_errorbox_done(void)
{
	errorbox_done = 1;
}

int get_errorbox_done(void)
{
	return errorbox_done;
}

void errorbox_show(char *errortxt)
{
	static GtkWidget *errorbox;
	gchar *tmp;

	if(errorbox_done != 1)
		return;
	errorbox_done = 0;
	tmp = g_strdup_printf("There has been an error"
			" that may require your attention.\n\n"
			"Contents of server error:\n\n"
			"%s\n",
			errortxt);

	errorbox = xmms_show_message("Scrobbler Error",
			tmp,
			"OK", FALSE, NULL, NULL);
	g_free(tmp);
	gtk_signal_connect(GTK_OBJECT(errorbox), "destroy",
			GTK_SIGNAL_FUNC(set_errorbox_done), &errorbox);
}
