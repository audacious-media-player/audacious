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
#include "audacious/input.h"

#include "config.h"

static void init(void);
static void cleanup(void);
static gboolean watchdog_func(gpointer unused);
static gint timeout_tag = 0;

static gint notify_playlist_pos = -1;
static gchar *previous_title = NULL;
static gboolean was_playing = FALSE;

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
        cleanup
};

GeneralPlugin *get_gplugin_info(void)
{
	notify_gp.description = g_strdup_printf(_("libnotify Plugin %s"), PACKAGE_VERSION);
	return &notify_gp;
}

static void init(void) 
{
	/* Initialise libnotify */
	notify_init(PACKAGE);

	/* 
	TODO: I assume 100 means 100ms checking interval? Why not 200? 
	It would be twice as efficient and the user shouldn't notice any difference.
	*/
	timeout_tag = g_timeout_add(100, watchdog_func, NULL);
}

static void cleanup(void)
{
   if ( timeout_tag > 0 )
   {
	  g_source_remove(timeout_tag);
	  timeout_tag = 0;
	}

	if (previous_title != NULL)
	{
		g_free(previous_title);
		previous_title = NULL;
	}

	/* Uninitialise libnotify */
	if ( notify_is_initted() == TRUE )
	  notify_uninit();
}

static gboolean watchdog_func(gpointer unused)
{
	gint pos = playlist_get_position();
	gchar *title = playlist_get_songtitle(pos);

	/*
	Trigger a notice if a song is now playing and one of the following is true:
		1. it has a different title from the last time this ran
		2. there is no title but it's a different playlist position
		3. the player was stopped the last time this ran
	(listed in the order they are checked)
	
	FIXME: The same song twice in a row will only get announced once.
	Proposed Remedy: Add a check for playback progress.
	*/
	if (ip_data.playing && 
		((title != NULL && previous_title != NULL &&
		g_strcasecmp(title, previous_title)) ||
		(title == NULL && pos != notify_playlist_pos) || (! was_playing)))
	{
		gchar *tmpbuf;
		TitleInput *tuple;

		tuple = playlist_get_tuple(pos);

		if (tuple == NULL)
			return TRUE;

		/* 
		FIXME: This is useless for formats without metadata.
		Proposed Remedy: playlist_get_filename(pos) instead of _("Unknown Track")
		*/
		tmpbuf = g_markup_printf_escaped("<b>%s</b>\n<i>%s</i>\n%s",
			(tuple->performer ? tuple->performer : _("Unknown Artist")),
			(tuple->album_name ? tuple->album_name : _("Unknown Album")),
			(tuple->track_name ? tuple->track_name : _("Unknown Track")));

		do_notification("Audacious", tmpbuf, DATA_DIR "/pixmaps/audacious.png");
		g_free(tmpbuf);
	}

	notify_playlist_pos = pos;

	if (previous_title != NULL)
	{
		g_free(previous_title);
		previous_title = NULL;
	}

	previous_title = g_strdup(title);
	was_playing = ip_data.playing;

	return TRUE;
}

static void do_notification(gchar *summary, gchar *message, gchar *icon_uri)
{
	NotifyNotification *n;  
	gint ret;

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
