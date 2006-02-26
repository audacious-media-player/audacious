#include "libaudacious/util.h"
#include "libaudacious/configdb.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "md5.h"

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

void errorbox_show(char *errortxt)
{
	gchar *tmp;

	tmp = g_strdup_printf("There has been an error"
			" that may require your attention.\n\n"
			"Contents of server error:\n\n"
			"%s\n",
			errortxt);

	xmms_show_message("Scrobbler Error",
			tmp,
			"OK", FALSE, NULL, NULL);
	g_free(tmp);
}
