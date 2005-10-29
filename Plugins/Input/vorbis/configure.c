#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "vorbis.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "libaudacious/configdb.h"
#include "libaudacious/dirbrowser.h"
#include "libaudacious/titlestring.h"
#include "libaudacious/util.h"
#include "audacious/plugin.h"

extern GMutex *vf_mutex;

static GtkWidget *vorbis_configurewin = NULL;
static GtkWidget *vbox, *notebook;

static GtkWidget *streaming_proxy_use, *streaming_proxy_host_entry;
static GtkWidget *streaming_proxy_port_entry, *streaming_save_entry;
static GtkWidget *streaming_save_use, *streaming_size_spin,
    *streaming_pre_spin;
static GtkWidget *streaming_proxy_auth_use;
static GtkWidget *streaming_proxy_auth_pass_entry,
    *streaming_proxy_auth_user_entry;
static GtkWidget *streaming_proxy_auth_user_label,
    *streaming_proxy_auth_pass_label;
static GtkWidget *streaming_proxy_hbox, *streaming_proxy_auth_hbox;
static GtkWidget *streaming_save_dirbrowser, *streaming_save_hbox;
static GtkWidget *title_tag_override, *title_tag_box, *title_tag_entry,
    *title_desc;
static GtkWidget *rg_switch, *rg_clip_switch, *rg_booster_switch,
    *rg_track_gain;

vorbis_config_t vorbis_cfg;

static void
vorbis_configurewin_ok(GtkWidget * widget, gpointer data)
{
    ConfigDb *db;
    GtkToggleButton *tb;

    vorbis_cfg.http_buffer_size =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                         (streaming_size_spin));
    vorbis_cfg.http_prebuffer =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(streaming_pre_spin));

    vorbis_cfg.use_proxy =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_use));
    g_free(vorbis_cfg.proxy_host);
    vorbis_cfg.proxy_host =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_host_entry)));
    vorbis_cfg.proxy_port =
        atoi(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_port_entry)));

    vorbis_cfg.proxy_use_auth =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                     (streaming_proxy_auth_use));

    g_free(vorbis_cfg.proxy_user);
    vorbis_cfg.proxy_user = NULL;
    if (strlen
        (gtk_entry_get_text(GTK_ENTRY(streaming_proxy_auth_user_entry))) > 0)
        vorbis_cfg.proxy_user =
            g_strdup(gtk_entry_get_text
                     (GTK_ENTRY(streaming_proxy_auth_user_entry)));

    g_free(vorbis_cfg.proxy_pass);
    vorbis_cfg.proxy_pass = NULL;
    if (strlen
        (gtk_entry_get_text(GTK_ENTRY(streaming_proxy_auth_pass_entry))) > 0)
        vorbis_cfg.proxy_pass =
            g_strdup(gtk_entry_get_text
                     (GTK_ENTRY(streaming_proxy_auth_pass_entry)));


    vorbis_cfg.save_http_stream =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_save_use));
    g_free(vorbis_cfg.save_http_path);
    vorbis_cfg.save_http_path =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(streaming_save_entry)));
    g_free(vorbis_cfg.tag_format);
    vorbis_cfg.tag_format =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(title_tag_entry)));

    tb = GTK_TOGGLE_BUTTON(title_tag_override);
    vorbis_cfg.tag_override = gtk_toggle_button_get_active(tb);
    tb = GTK_TOGGLE_BUTTON(rg_switch);
    vorbis_cfg.use_replaygain = gtk_toggle_button_get_active(tb);
    tb = GTK_TOGGLE_BUTTON(rg_clip_switch);
    vorbis_cfg.use_anticlip = gtk_toggle_button_get_active(tb);
    tb = GTK_TOGGLE_BUTTON(rg_booster_switch);
    vorbis_cfg.use_booster = gtk_toggle_button_get_active(tb);
    tb = GTK_TOGGLE_BUTTON(rg_track_gain);
    if (gtk_toggle_button_get_active(tb))
        vorbis_cfg.replaygain_mode = REPLAYGAIN_MODE_TRACK;
    else
        vorbis_cfg.replaygain_mode = REPLAYGAIN_MODE_ALBUM;


    db = bmp_cfg_db_open();

    bmp_cfg_db_set_int(db, "vorbis", "http_buffer_size",
                       vorbis_cfg.http_buffer_size);
    bmp_cfg_db_set_int(db, "vorbis", "http_prebuffer",
                       vorbis_cfg.http_prebuffer);
    bmp_cfg_db_set_bool(db, "vorbis", "use_proxy", vorbis_cfg.use_proxy);
    bmp_cfg_db_set_string(db, "vorbis", "proxy_host", vorbis_cfg.proxy_host);
    bmp_cfg_db_set_bool(db, "vorbis", "save_http_stream",
                        vorbis_cfg.save_http_stream);
    bmp_cfg_db_set_string(db, "vorbis", "save_http_path",
                          vorbis_cfg.save_http_path);
    bmp_cfg_db_set_bool(db, "vorbis", "tag_override",
                        vorbis_cfg.tag_override);
    bmp_cfg_db_set_string(db, "vorbis", "tag_format", vorbis_cfg.tag_format);
    bmp_cfg_db_set_int(db, "vorbis", "proxy_port", vorbis_cfg.proxy_port);
    bmp_cfg_db_set_bool(db, "vorbis", "proxy_use_auth",
                        vorbis_cfg.proxy_use_auth);
    if (vorbis_cfg.proxy_user)
        bmp_cfg_db_set_string(db, "vorbis", "proxy_user",
                              vorbis_cfg.proxy_user);
    else
        bmp_cfg_db_unset_key(db, "vorbis", "proxy_user");

    if (vorbis_cfg.proxy_pass)
        bmp_cfg_db_set_string(db, "vorbis", "proxy_pass",
                              vorbis_cfg.proxy_pass);
    else
        bmp_cfg_db_unset_key(db, "vorbis", "proxy_pass");
    bmp_cfg_db_set_bool(db, "vorbis", "use_anticlip",
                        vorbis_cfg.use_anticlip);
    bmp_cfg_db_set_bool(db, "vorbis", "use_replaygain",
                        vorbis_cfg.use_replaygain);
    bmp_cfg_db_set_int(db, "vorbis", "replaygain_mode",
                       vorbis_cfg.replaygain_mode);
    bmp_cfg_db_set_bool(db, "vorbis", "use_booster", vorbis_cfg.use_booster);
    bmp_cfg_db_close(db);
    gtk_widget_destroy(vorbis_configurewin);
}

static void
proxy_use_cb(GtkWidget * w, gpointer data)
{
    gboolean use_proxy, use_proxy_auth;

    use_proxy =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_use));
    use_proxy_auth =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                     (streaming_proxy_auth_use));

    gtk_widget_set_sensitive(streaming_proxy_hbox, use_proxy);
    gtk_widget_set_sensitive(streaming_proxy_auth_use, use_proxy);
    gtk_widget_set_sensitive(streaming_proxy_auth_hbox, use_proxy
                             && use_proxy_auth);
}

static void
proxy_auth_use_cb(GtkWidget * w, gpointer data)
{
    gboolean use_proxy, use_proxy_auth;

    use_proxy =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_use));
    use_proxy_auth =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                     (streaming_proxy_auth_use));

    gtk_widget_set_sensitive(streaming_proxy_auth_hbox, use_proxy
                             && use_proxy_auth);
}

static void
streaming_save_dirbrowser_cb(gchar * dir)
{
    gtk_entry_set_text(GTK_ENTRY(streaming_save_entry), dir);
}

static void
streaming_save_browse_cb(GtkWidget * w, gpointer data)
{
    if (streaming_save_dirbrowser)
        return;

    streaming_save_dirbrowser =
        xmms_create_dir_browser(_("Select the directory where you want "
                                  "to store the Ogg Vorbis streams:"),
                                vorbis_cfg.save_http_path,
                                GTK_SELECTION_SINGLE,
                                streaming_save_dirbrowser_cb);
    g_signal_connect(G_OBJECT(streaming_save_dirbrowser),
                     "destroy", G_CALLBACK(gtk_widget_destroyed),
                     &streaming_save_dirbrowser);
    gtk_window_set_transient_for(GTK_WINDOW(streaming_save_dirbrowser),
                                 GTK_WINDOW(vorbis_configurewin));
    gtk_widget_show(streaming_save_dirbrowser);
}

static void
streaming_save_use_cb(GtkWidget * w, gpointer data)
{
    gboolean save_stream;

    save_stream =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_save_use));

    gtk_widget_set_sensitive(streaming_save_hbox, save_stream);
}

static void
configure_destroy(GtkWidget * w, gpointer data)
{
/*  	if (streaming_save_dirbrowser) */
/*  		gtk_widget_destroy(streaming_save_dirbrowser); */
}

static void
title_tag_override_cb(GtkWidget * w, gpointer data)
{
    gboolean override;
    override =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(title_tag_override));
    gtk_widget_set_sensitive(title_tag_box, override);
    gtk_widget_set_sensitive(title_desc, override);
}

static void
rg_switch_cb(GtkWidget * w, gpointer data)
{
    gtk_widget_set_sensitive(GTK_WIDGET(data),
                             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                                          (w)));
}

void
vorbis_configure(void)
{
    GtkWidget *streaming_vbox;
    GtkWidget *streaming_buf_frame, *streaming_buf_hbox;
    GtkWidget *streaming_size_box, *streaming_size_label;
    GtkWidget *streaming_pre_box, *streaming_pre_label;
    GtkWidget *streaming_proxy_frame, *streaming_proxy_vbox;
    GtkWidget *streaming_proxy_port_label, *streaming_proxy_host_label;
    GtkWidget *streaming_save_frame, *streaming_save_vbox;
    GtkWidget *streaming_save_label, *streaming_save_browse;
    GtkWidget *title_frame, *title_tag_vbox, *title_tag_label;
    GtkWidget *rg_frame, *rg_vbox;
    GtkWidget *bbox, *ok, *cancel;
    GtkWidget *rg_type_frame, *rg_type_vbox, *rg_album_gain;
    GtkObject *streaming_size_adj, *streaming_pre_adj;

    char *temp;

    if (vorbis_configurewin != NULL) {
        gtk_window_present(GTK_WINDOW(vorbis_configurewin));
        return;
    }

    vorbis_configurewin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(vorbis_configurewin),
                             GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_position(GTK_WINDOW(vorbis_configurewin),
                            GTK_WIN_POS_CENTER);
    g_signal_connect(G_OBJECT(vorbis_configurewin), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &vorbis_configurewin);
    g_signal_connect(G_OBJECT(vorbis_configurewin), "destroy",
                     G_CALLBACK(configure_destroy), &vorbis_configurewin);
    gtk_window_set_title(GTK_WINDOW(vorbis_configurewin),
                         _("Ogg Vorbis Audio Plugin Configuration"));
    gtk_window_set_resizable(GTK_WINDOW(vorbis_configurewin), FALSE);
    gtk_container_border_width(GTK_CONTAINER(vorbis_configurewin), 10);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(vorbis_configurewin), vbox);

    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    streaming_vbox = gtk_vbox_new(FALSE, 0);

    streaming_buf_frame = gtk_frame_new(_("Buffering:"));
    gtk_container_set_border_width(GTK_CONTAINER(streaming_buf_frame), 5);
    gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_buf_frame, FALSE,
                       FALSE, 0);

    streaming_buf_hbox = gtk_hbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(streaming_buf_hbox), 5);
    gtk_container_add(GTK_CONTAINER(streaming_buf_frame), streaming_buf_hbox);

    streaming_size_box = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(streaming_buf_hbox), streaming_size_box,
                       TRUE, TRUE, 0);
    streaming_size_label = gtk_label_new(_("Buffer size (kb):"));
    gtk_box_pack_start(GTK_BOX(streaming_size_box), streaming_size_label,
                       FALSE, FALSE, 0);
    streaming_size_adj =
        gtk_adjustment_new(vorbis_cfg.http_buffer_size, 4, 4096, 4, 4, 4);
    streaming_size_spin =
        gtk_spin_button_new(GTK_ADJUSTMENT(streaming_size_adj), 8, 0);
    gtk_widget_set_usize(streaming_size_spin, 60, -1);
    gtk_box_pack_start(GTK_BOX(streaming_size_box), streaming_size_spin,
                       FALSE, FALSE, 0);

    streaming_pre_box = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(streaming_buf_hbox), streaming_pre_box,
                       TRUE, TRUE, 0);
    streaming_pre_label = gtk_label_new(_("Pre-buffer (percent):"));
    gtk_box_pack_start(GTK_BOX(streaming_pre_box), streaming_pre_label,
                       FALSE, FALSE, 0);
    streaming_pre_adj =
        gtk_adjustment_new(vorbis_cfg.http_prebuffer, 0, 90, 1, 1, 1);
    streaming_pre_spin =
        gtk_spin_button_new(GTK_ADJUSTMENT(streaming_pre_adj), 1, 0);
    gtk_widget_set_usize(streaming_pre_spin, 60, -1);
    gtk_box_pack_start(GTK_BOX(streaming_pre_box), streaming_pre_spin,
                       FALSE, FALSE, 0);

    /*
     * Proxy config.
     */
    streaming_proxy_frame = gtk_frame_new(_("Proxy:"));
    gtk_container_set_border_width(GTK_CONTAINER(streaming_proxy_frame), 5);
    gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_proxy_frame,
                       FALSE, FALSE, 0);

    streaming_proxy_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(streaming_proxy_vbox), 5);
    gtk_container_add(GTK_CONTAINER(streaming_proxy_frame),
                      streaming_proxy_vbox);

    streaming_proxy_use = gtk_check_button_new_with_label(_("Use proxy"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(streaming_proxy_use),
                                 vorbis_cfg.use_proxy);
    g_signal_connect(G_OBJECT(streaming_proxy_use), "clicked",
                     G_CALLBACK(proxy_use_cb), NULL);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox), streaming_proxy_use,
                       FALSE, FALSE, 0);

    streaming_proxy_hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(streaming_proxy_hbox, vorbis_cfg.use_proxy);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox), streaming_proxy_hbox,
                       FALSE, FALSE, 0);

    streaming_proxy_host_label = gtk_label_new(_("Host:"));
    gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox),
                       streaming_proxy_host_label, FALSE, FALSE, 0);

    streaming_proxy_host_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(streaming_proxy_host_entry),
                       vorbis_cfg.proxy_host);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox),
                       streaming_proxy_host_entry, TRUE, TRUE, 0);

    streaming_proxy_port_label = gtk_label_new(_("Port:"));
    gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox),
                       streaming_proxy_port_label, FALSE, FALSE, 0);

    streaming_proxy_port_entry = gtk_entry_new();
    gtk_widget_set_usize(streaming_proxy_port_entry, 50, -1);
    temp = g_strdup_printf("%d", vorbis_cfg.proxy_port);
    gtk_entry_set_text(GTK_ENTRY(streaming_proxy_port_entry), temp);
    g_free(temp);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox),
                       streaming_proxy_port_entry, FALSE, FALSE, 0);

    streaming_proxy_auth_use =
        gtk_check_button_new_with_label(_("Use authentication"));
    gtk_widget_set_sensitive(streaming_proxy_auth_use, vorbis_cfg.use_proxy);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                 (streaming_proxy_auth_use),
                                 vorbis_cfg.proxy_use_auth);
    g_signal_connect(G_OBJECT(streaming_proxy_auth_use), "clicked",
                     G_CALLBACK(proxy_auth_use_cb), NULL);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox),
                       streaming_proxy_auth_use, FALSE, FALSE, 0);

    streaming_proxy_auth_hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(streaming_proxy_auth_hbox,
                             vorbis_cfg.use_proxy
                             && vorbis_cfg.proxy_use_auth);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox),
                       streaming_proxy_auth_hbox, FALSE, FALSE, 0);

    streaming_proxy_auth_user_label = gtk_label_new(_("Username:"));
    gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox),
                       streaming_proxy_auth_user_label, FALSE, FALSE, 0);

    streaming_proxy_auth_user_entry = gtk_entry_new();
    if (vorbis_cfg.proxy_user)
        gtk_entry_set_text(GTK_ENTRY(streaming_proxy_auth_user_entry),
                           vorbis_cfg.proxy_user);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox),
                       streaming_proxy_auth_user_entry, TRUE, TRUE, 0);

    streaming_proxy_auth_pass_label = gtk_label_new(_("Password:"));
    gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox),
                       streaming_proxy_auth_pass_label, FALSE, FALSE, 0);

    streaming_proxy_auth_pass_entry = gtk_entry_new();
    if (vorbis_cfg.proxy_pass)
        gtk_entry_set_text(GTK_ENTRY(streaming_proxy_auth_pass_entry),
                           vorbis_cfg.proxy_pass);
    gtk_entry_set_visibility(GTK_ENTRY(streaming_proxy_auth_pass_entry),
                             FALSE);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox),
                       streaming_proxy_auth_pass_entry, TRUE, TRUE, 0);


    /*
     * Save to disk config.
     */
    streaming_save_frame = gtk_frame_new(_("Save stream to disk:"));
    gtk_container_set_border_width(GTK_CONTAINER(streaming_save_frame), 5);
    gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_save_frame,
                       FALSE, FALSE, 0);

    streaming_save_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(streaming_save_vbox), 5);
    gtk_container_add(GTK_CONTAINER(streaming_save_frame),
                      streaming_save_vbox);

    streaming_save_use =
        gtk_check_button_new_with_label(_("Save stream to disk"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(streaming_save_use),
                                 vorbis_cfg.save_http_stream);
    g_signal_connect(G_OBJECT(streaming_save_use), "clicked",
                     G_CALLBACK(streaming_save_use_cb), NULL);
    gtk_box_pack_start(GTK_BOX(streaming_save_vbox), streaming_save_use,
                       FALSE, FALSE, 0);

    streaming_save_hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(streaming_save_hbox,
                             vorbis_cfg.save_http_stream);
    gtk_box_pack_start(GTK_BOX(streaming_save_vbox), streaming_save_hbox,
                       FALSE, FALSE, 0);

    streaming_save_label = gtk_label_new(_("Path:"));
    gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_label,
                       FALSE, FALSE, 0);

    streaming_save_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(streaming_save_entry),
                       vorbis_cfg.save_http_path);
    gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_entry,
                       TRUE, TRUE, 0);

    streaming_save_browse = gtk_button_new_with_label(_("Browse"));
    g_signal_connect(G_OBJECT(streaming_save_browse), "clicked",
                     G_CALLBACK(streaming_save_browse_cb), NULL);
    gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_browse,
                       FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), streaming_vbox,
                             gtk_label_new(_("Streaming")));

    /* Title config.. */

    title_frame = gtk_frame_new(_("Ogg Vorbis Tags:"));
    gtk_container_border_width(GTK_CONTAINER(title_frame), 5);

    title_tag_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_border_width(GTK_CONTAINER(title_tag_vbox), 5);
    gtk_container_add(GTK_CONTAINER(title_frame), title_tag_vbox);

    title_tag_override =
        gtk_check_button_new_with_label(_("Override generic titles"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(title_tag_override),
                                 vorbis_cfg.tag_override);
    g_signal_connect(G_OBJECT(title_tag_override), "clicked",
                     G_CALLBACK(title_tag_override_cb), NULL);
    gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_tag_override, FALSE,
                       FALSE, 0);

    title_tag_box = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(title_tag_box, vorbis_cfg.tag_override);
    gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_tag_box, FALSE,
                       FALSE, 0);

    title_tag_label = gtk_label_new(_("Title format:"));
    gtk_box_pack_start(GTK_BOX(title_tag_box), title_tag_label, FALSE,
                       FALSE, 0);

    title_tag_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(title_tag_entry), vorbis_cfg.tag_format);
    gtk_box_pack_start(GTK_BOX(title_tag_box), title_tag_entry, TRUE, TRUE,
                       0);

    title_desc = xmms_titlestring_descriptions("pafFetndgc", 2);
    gtk_widget_set_sensitive(title_desc, vorbis_cfg.tag_override);
    gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_desc, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), title_frame,
                             gtk_label_new(_("Title")));

    /* Replay Gain.. */

    rg_frame = gtk_frame_new(_("ReplayGain Settings:"));
    gtk_container_border_width(GTK_CONTAINER(rg_frame), 5);

    rg_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_border_width(GTK_CONTAINER(rg_vbox), 5);
    gtk_container_add(GTK_CONTAINER(rg_frame), rg_vbox);

    rg_clip_switch =
        gtk_check_button_new_with_label(_("Enable Clipping Prevention"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_clip_switch),
                                 vorbis_cfg.use_anticlip);
    gtk_box_pack_start(GTK_BOX(rg_vbox), rg_clip_switch, FALSE, FALSE, 0);

    rg_switch = gtk_check_button_new_with_label(_("Enable ReplayGain"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_switch),
                                 vorbis_cfg.use_replaygain);
    gtk_box_pack_start(GTK_BOX(rg_vbox), rg_switch, FALSE, FALSE, 0);

    rg_type_frame = gtk_frame_new(_("ReplayGain Type:"));
    gtk_box_pack_start(GTK_BOX(rg_vbox), rg_type_frame, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(rg_switch), "toggled",
                     G_CALLBACK(rg_switch_cb), rg_type_frame);

    rg_type_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(rg_type_vbox), 5);
    gtk_container_add(GTK_CONTAINER(rg_type_frame), rg_type_vbox);

    rg_track_gain =
        gtk_radio_button_new_with_label(NULL, _("use Track Gain/Peak"));
    if (vorbis_cfg.replaygain_mode == REPLAYGAIN_MODE_TRACK)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_track_gain), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_track_gain), FALSE);
    gtk_box_pack_start(GTK_BOX(rg_type_vbox), rg_track_gain, FALSE, FALSE, 0);

    rg_album_gain =
        gtk_radio_button_new_with_label(gtk_radio_button_group
                                        (GTK_RADIO_BUTTON(rg_track_gain)),
                                        _("use Album Gain/Peak"));
    if (vorbis_cfg.replaygain_mode == REPLAYGAIN_MODE_ALBUM)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_album_gain), TRUE);
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_album_gain), FALSE);
    gtk_box_pack_start(GTK_BOX(rg_type_vbox), rg_album_gain, FALSE, FALSE, 0);

    if (!vorbis_cfg.use_replaygain)
        gtk_widget_set_sensitive(rg_type_frame, FALSE);

    rg_booster_switch =
        gtk_check_button_new_with_label(_
                                        ("Enable 6dB Boost + Hard Limiting"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_booster_switch),
                                 vorbis_cfg.use_booster);
    gtk_box_pack_start(GTK_BOX(rg_vbox), rg_booster_switch, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), rg_frame,
                             gtk_label_new(_("ReplayGain")));

    /* Buttons */

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_button_set_use_stock(GTK_BUTTON(cancel), TRUE);
    g_signal_connect_swapped(G_OBJECT(cancel), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             G_OBJECT(vorbis_configurewin));
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

    ok = gtk_button_new_from_stock(GTK_STOCK_OK);
    gtk_button_set_use_stock(GTK_BUTTON(ok), TRUE);
    g_signal_connect(G_OBJECT(ok), "clicked",
                     G_CALLBACK(vorbis_configurewin_ok), NULL);
    GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
    gtk_widget_grab_default(ok);

    gtk_widget_show_all(vorbis_configurewin);
}
