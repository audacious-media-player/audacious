/*
 * libnotify plugin for audacious 1.1+
 * http://nenolod.net/code/audacious-notify
 *
 * See `COPYING' in the directory root for license details.
 */
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libnotify/notify.h>
#include "audacious/plugin.h"
#include "audacious/playlist.h"
#include "audacious/input.h"
#include "libaudacious/configdb.h"

#include "config.h"

static void init(void);
static void cleanup(void);
static void configure_gui(void);
static void configure_load(void);
static void configure_save(void);
static gboolean watchdog_func(gpointer unused);
static gint timeout_tag = 0;

static gint notify_playlist_pos = -1;
static gchar *previous_title = NULL;
static gboolean was_playing = FALSE;

typedef struct
{
  gint notif_timeout;
  gboolean notif_neverexpire;
  gboolean notif_skipnf;
}
audcfg_t;

static audcfg_t audcfg = { 5000 , FALSE , TRUE };

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
        configure_gui,
        cleanup
};

GeneralPlugin *get_gplugin_info(void)
{
	notify_gp.description = g_strdup_printf(_("libnotify Plugin %s"), PACKAGE_VERSION);
	return &notify_gp;
}

static void init(void) 
{
   /* load configuration */
   configure_load();

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
			(tuple->performer ? tuple->performer : ( audcfg.notif_skipnf == FALSE ? _("Unknown Artist") : "" )),
			(tuple->album_name ? tuple->album_name : ( audcfg.notif_skipnf == FALSE ? _("Unknown Album") : "" )),
			(tuple->track_name ? tuple->track_name : ( audcfg.notif_skipnf == FALSE ? _("Unknown Track") : "" )));

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

   if ( audcfg.notif_neverexpire == FALSE )
     notify_notification_set_timeout(n, audcfg.notif_timeout);
	else
     notify_notification_set_timeout(n, NOTIFY_EXPIRES_NEVER);

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


/* ABOUTBOX - TODO -> complete me with credits info!

static void about_gui(void)
{
  static GtkWidget *aboutwin = NULL;
  gchar *aboutstr;

  if ( aboutwin != NULL )
    return;

  aboutstr = g_strdup_printf( _("Audacious libnotify Plugin\n\n...") );
  aboutwin = xmms_show_message( _("About libnotify Plugin"), _(aboutstr), _("Ok"), FALSE, NULL, NULL);
  g_free( aboutstr );
  g_signal_connect( G_OBJECT(aboutwin) , "destroy", G_CALLBACK(gtk_widget_destroyed), &aboutwin );
}
*/


/* CONFIGURATION */

static void configure_load(void)
{
  ConfigDb *db;

  db = bmp_cfg_db_open();
  bmp_cfg_db_get_bool(db, "libnotify", "notif_skipnf", &audcfg.notif_skipnf);
  bmp_cfg_db_get_int(db, "libnotify", "notif_timeout", &audcfg.notif_timeout);
  bmp_cfg_db_get_bool(db, "libnotify", "notif_neverexpire", &audcfg.notif_neverexpire);
  bmp_cfg_db_close(db);
}

static void configure_save(void)
{
  ConfigDb *db = bmp_cfg_db_open();
  bmp_cfg_db_set_bool(db, "libnotify", "notif_skipnf", audcfg.notif_skipnf);
  bmp_cfg_db_set_int(db, "libnotify", "notif_timeout", audcfg.notif_timeout);
  bmp_cfg_db_set_bool(db, "libnotify", "notif_neverexpire", audcfg.notif_neverexpire);
  bmp_cfg_db_close(db);
}


static void configure_ev_notiftimeout_toggle( GtkToggleButton *togglebt , gpointer hbox )
{
  gtk_widget_set_sensitive( GTK_WIDGET(hbox) , !gtk_toggle_button_get_active( togglebt ) );
}

static void configure_ev_notifskipnf_commit( gpointer togglebt )
{
  audcfg.notif_skipnf = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(togglebt) );
}

static void configure_ev_notiftimeout_commit( gpointer spinbt )
{
  audcfg.notif_timeout = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(spinbt) );
}

static void configure_ev_notifexpire_commit( gpointer togglebt )
{
  audcfg.notif_neverexpire = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(togglebt) );
}

static void configure_ev_bok( gpointer configwin )
{
  configure_save();
  gtk_widget_destroy( GTK_WIDGET(configwin) );
}

static void configure_gui(void)
{
  static GtkWidget *configwin = NULL;
  GtkWidget *configwin_vbox;
  GtkWidget *notif_info_frame, *notif_info_vbox, *notif_info_skipnf_cbt;
  GtkWidget *notif_timeout_frame, *notif_timeout_hbox, *notif_timeout_vbox;
  GtkWidget *notif_timeout_spinbt, *notif_timeout_cbt;
  GtkWidget *hbuttonbox, *button_ok, *button_cancel;

  if ( configwin != NULL )
    return;

  configwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_type_hint( GTK_WINDOW(configwin), GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_title( GTK_WINDOW(configwin), _("libnotify plugin") );
  gtk_container_set_border_width( GTK_CONTAINER(configwin), 10 );
  g_signal_connect( G_OBJECT(configwin) , "destroy" , G_CALLBACK(gtk_widget_destroyed) , &configwin );
  configwin_vbox = gtk_vbox_new( FALSE , 6 );
  gtk_container_add( GTK_CONTAINER(configwin) , configwin_vbox );
  button_ok = gtk_button_new_from_stock( GTK_STOCK_OK );

  /* notification info */
  notif_info_frame = gtk_frame_new( _("Notification details") );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , notif_info_frame , TRUE , TRUE , 0 );
  notif_info_vbox = gtk_vbox_new( FALSE , 4 );
  gtk_container_set_border_width( GTK_CONTAINER(notif_info_vbox), 4 );
  gtk_container_add( GTK_CONTAINER(notif_info_frame) , notif_info_vbox );
  notif_info_skipnf_cbt = gtk_check_button_new_with_label( _("Skip empty fields") );
  g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
    G_CALLBACK(configure_ev_notifskipnf_commit) , notif_info_skipnf_cbt );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(notif_info_skipnf_cbt) , audcfg.notif_skipnf );
  gtk_box_pack_start( GTK_BOX(notif_info_vbox) , notif_info_skipnf_cbt , FALSE , FALSE , 0 );

  /* notification timeout */
  notif_timeout_frame = gtk_frame_new( _("Notification timeout") );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , notif_timeout_frame , TRUE , TRUE , 0 );
  notif_timeout_vbox = gtk_vbox_new( FALSE , 4 );
  gtk_container_set_border_width( GTK_CONTAINER(notif_timeout_vbox), 4 );
  gtk_container_add( GTK_CONTAINER(notif_timeout_frame) , notif_timeout_vbox );
  notif_timeout_hbox = gtk_hbox_new( FALSE , 2 );
  notif_timeout_spinbt = gtk_spin_button_new_with_range( 1 , 20000 , 100 );
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(notif_timeout_spinbt) , (gdouble)audcfg.notif_timeout );
  g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
    G_CALLBACK(configure_ev_notiftimeout_commit) , notif_timeout_spinbt );
  gtk_box_pack_start( GTK_BOX(notif_timeout_hbox) ,
                      gtk_label_new( _("Expire time:") ) , FALSE , FALSE , 0 );
  gtk_box_pack_start( GTK_BOX(notif_timeout_hbox) , notif_timeout_spinbt , FALSE , FALSE , 0 );
  gtk_box_pack_start( GTK_BOX(notif_timeout_hbox) ,
                      gtk_label_new( _("ms") ) , FALSE , FALSE , 0 );
  gtk_box_pack_start( GTK_BOX(notif_timeout_vbox) , notif_timeout_hbox , FALSE , FALSE , 0 );
  notif_timeout_cbt = gtk_check_button_new_with_label( _("Notification never expires") );
  g_signal_connect( G_OBJECT(notif_timeout_cbt) , "toggled" ,
    G_CALLBACK(configure_ev_notiftimeout_toggle) , notif_timeout_hbox );
  g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
    G_CALLBACK(configure_ev_notifexpire_commit) , notif_timeout_cbt );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(notif_timeout_cbt) , audcfg.notif_neverexpire );
  gtk_box_pack_start( GTK_BOX(notif_timeout_vbox) , notif_timeout_cbt , FALSE , FALSE , 0 );

  /* buttons */
  hbuttonbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout( GTK_BUTTON_BOX(hbuttonbox) , GTK_BUTTONBOX_END );
  button_cancel = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
  g_signal_connect_swapped( G_OBJECT(button_cancel) , "clicked" ,
    G_CALLBACK(gtk_widget_destroy) , configwin );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_cancel );
  g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" ,
    G_CALLBACK(configure_ev_bok) , configwin );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_ok );
  gtk_box_pack_start( GTK_BOX(configwin_vbox) , hbuttonbox , FALSE , FALSE , 0 );

  gtk_widget_show_all( configwin );
}
