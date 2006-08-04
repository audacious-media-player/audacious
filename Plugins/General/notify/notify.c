/*
 * libnotify plugin for audacious 1.1+
 * http://nenolod.net/code/audacious-notify
 *
 * See `COPYING' in the directory root for license details.
 */
#include <glib.h>
#include <glib/gi18n.h>

#include <libnotify/notify.h>
#include "audacious/plugin.h"
#include "audacious/playlist.h"

#include "config.h"

static void init(void);
static void cleanup(void);
static gboolean watchdog_func(gpointer unused);
static gint timeout_tag = 0;

static gint notify_playlist_pos = -1;

/* our API. */
static void do_notification(gchar *summary, gchar *message, gchar *icon_uri);

GeneralPlugin notify_gp =
{
        NULL,                   /* handle */
        NULL,                   /* filename */
        -1,                     /* xmms_session */
        NULL,                   /* Description */
        init,
        NULL,
        NULL,
        cleanup,
};

GeneralPlugin *get_gplugin_info(void)
{
	notify_gp.description = g_strdup_printf(_("libnotify Plugin %s"), PACKAGE_VERSION);
	return &notify_gp;
}

static void init(void) 
{
	timeout_tag = gtk_timeout_add(100, watchdog_func, NULL);
}

static void cleanup(void)
{
	gtk_timeout_remove(timeout_tag);
}

static gboolean watchdog_func(gpointer unused)
{
	gint pos = playlist_get_position();

	if (pos != notify_playlist_pos)
	{
		gchar *tmpbuf;
		TitleInput *tuple;

		tuple = playlist_get_tuple(pos);

		if (tuple == NULL)
			return;

		tmpbuf = g_strdup_printf("<b>%s</b>\n<i>%s</i>\n%s",
			tuple->performer,
			tuple->album_name,
			tuple->track_name);

		do_notification("Audacious", tmpbuf, DATA_DIR "/pixmaps/audacious.png");
		g_free(tmpbuf);
	}

	notify_playlist_pos = pos;

	return TRUE;
}

static void do_notification(gchar *summary, gchar *message, gchar *icon_uri)
{
	NotifyNotification *n;  
	gint ret;

	/* Initialise libnotify if we haven't done so yet. */
	notify_init(PACKAGE);

	n = notify_notification_new(summary, 
 	                            message,
        	                    NULL, NULL);

	/* make sure we have a notify object before continuing,
	 * for paranoia's sake anyhow. -nenolod
	 */
	if (n == NULL)
		return;

	notify_notification_set_timeout(n, 5000);	/* todo: configurable? */

	if (icon_uri != NULL)
	{
		GdkPixbuf *gp = gdk_pixbuf_new_from_file(icon_uri, NULL);

		if (gp != NULL)
		{
			notify_notification_set_icon_from_pixbuf(n, GDK_PIXBUF(gp));
			g_object_unref(G_OBJECT(gp));
		}
	}

	ret = notify_notification_show(n, NULL);

#if 0
	/* rainy day: handle this condition below -nenolod */
	if (ret == 0)
	{
		/* do something */
	}
#endif

	g_object_unref(G_OBJECT(n));
}
