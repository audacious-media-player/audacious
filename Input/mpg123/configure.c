
#include "mpg123.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <libaudacious/configdb.h>
#include <libaudacious/dirbrowser.h>
#include <libaudacious/titlestring.h>


static GtkWidget *mpg123_configurewin = NULL;
static GtkWidget *vbox, *notebook;
static GtkWidget *decode_vbox, *decode_hbox1;
static GtkWidget *decode_res_frame, *decode_res_vbox, *decode_res_16,
    *decode_res_8;
static GtkWidget *decode_ch_frame, *decode_ch_vbox, *decode_ch_stereo,
    *decode_ch_mono;
static GtkWidget *decode_freq_frame, *decode_freq_vbox, *decode_freq_1to1,
    *decode_freq_1to2, *decode_freq_1to4;
static GtkWidget *option_frame, *option_vbox, *detect_by_content,
    *detect_by_extension, *detect_by_both;
#ifdef USE_SIMD
static GtkWidget *auto_select, *decoder_3dnow, *decoder_mmx, *decoder_fpu;

static void auto_select_cb(GtkWidget * w, gpointer data);
#endif

static GtkObject *streaming_size_adj, *streaming_pre_adj;
static GtkWidget *streaming_proxy_use, *streaming_proxy_host_entry;
static GtkWidget *streaming_proxy_port_entry, *streaming_save_use,
    *streaming_save_entry;
static GtkWidget *streaming_proxy_auth_use;
static GtkWidget *streaming_proxy_auth_pass_entry,
    *streaming_proxy_auth_user_entry;
static GtkWidget *streaming_proxy_auth_user_label,
    *streaming_proxy_auth_pass_label;
static GtkWidget *streaming_cast_title, *streaming_udp_title;
static GtkWidget *streaming_proxy_hbox, *streaming_proxy_auth_hbox,
    *streaming_save_dirbrowser;
static GtkWidget *streaming_save_hbox, *title_id3_box, *title_tag_desc;
static GtkWidget *title_override, *title_id3_entry, *title_id3v2_disable;

/* Encoding patch */
static GtkWidget *title_encoding_hbox, *title_encoding_enabled, *title_encoding, *title_encoding_label;
/* Encoding patch */

MPG123Config mpg123_cfg;

static void
mpg123_configurewin_ok(GtkWidget * widget, gpointer data)
{
    ConfigDb *db;

    if (GTK_TOGGLE_BUTTON(decode_res_16)->active)
        mpg123_cfg.resolution = 16;
    else if (GTK_TOGGLE_BUTTON(decode_res_8)->active)
        mpg123_cfg.resolution = 8;

    if (GTK_TOGGLE_BUTTON(decode_ch_stereo)->active)
        mpg123_cfg.channels = 2;
    else if (GTK_TOGGLE_BUTTON(decode_ch_mono)->active)
        mpg123_cfg.channels = 1;

    if (GTK_TOGGLE_BUTTON(decode_freq_1to1)->active)
        mpg123_cfg.downsample = 0;
    else if (GTK_TOGGLE_BUTTON(decode_freq_1to2)->active)
        mpg123_cfg.downsample = 1;
    if (GTK_TOGGLE_BUTTON(decode_freq_1to4)->active)
        mpg123_cfg.downsample = 2;

    if (GTK_TOGGLE_BUTTON(detect_by_content)->active)
        mpg123_cfg.detect_by = DETECT_CONTENT;
    else if (GTK_TOGGLE_BUTTON(detect_by_extension)->active)
        mpg123_cfg.detect_by = DETECT_EXTENSION;
    else if (GTK_TOGGLE_BUTTON(detect_by_both)->active)
        mpg123_cfg.detect_by = DETECT_BOTH;
    else
        mpg123_cfg.detect_by = DETECT_EXTENSION;

#ifdef USE_SIMD
    if (GTK_TOGGLE_BUTTON(auto_select)->active)
        mpg123_cfg.default_synth = SYNTH_AUTO;
    else if (GTK_TOGGLE_BUTTON(decoder_fpu)->active)
        mpg123_cfg.default_synth = SYNTH_FPU;
    else if (GTK_TOGGLE_BUTTON(decoder_mmx)->active)
        mpg123_cfg.default_synth = SYNTH_MMX;
    else
        mpg123_cfg.default_synth = SYNTH_3DNOW;

#endif
    mpg123_cfg.http_buffer_size =
        (gint) GTK_ADJUSTMENT(streaming_size_adj)->value;
    mpg123_cfg.http_prebuffer =
        (gint) GTK_ADJUSTMENT(streaming_pre_adj)->value;

    mpg123_cfg.use_proxy =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_use));
    g_free(mpg123_cfg.proxy_host);
    mpg123_cfg.proxy_host =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_host_entry)));
    mpg123_cfg.proxy_port =
        atoi(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_port_entry)));

    mpg123_cfg.proxy_use_auth =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                     (streaming_proxy_auth_use));

    if (mpg123_cfg.proxy_user)
        g_free(mpg123_cfg.proxy_user);
    mpg123_cfg.proxy_user = NULL;
    if (strlen
        (gtk_entry_get_text(GTK_ENTRY(streaming_proxy_auth_user_entry))) > 0)
        mpg123_cfg.proxy_user =
            g_strdup(gtk_entry_get_text
                     (GTK_ENTRY(streaming_proxy_auth_user_entry)));

    if (mpg123_cfg.proxy_pass)
        g_free(mpg123_cfg.proxy_pass);
    mpg123_cfg.proxy_pass = NULL;
    if (strlen
        (gtk_entry_get_text(GTK_ENTRY(streaming_proxy_auth_pass_entry))) > 0)
        mpg123_cfg.proxy_pass =
            g_strdup(gtk_entry_get_text
                     (GTK_ENTRY(streaming_proxy_auth_pass_entry)));


    mpg123_cfg.save_http_stream =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_save_use));
    if (mpg123_cfg.save_http_path)
        g_free(mpg123_cfg.save_http_path);
    mpg123_cfg.save_http_path =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(streaming_save_entry)));

    mpg123_cfg.use_udp_channel =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_udp_title));

    mpg123_cfg.title_override =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(title_override));
    mpg123_cfg.disable_id3v2 =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(title_id3v2_disable));
    g_free(mpg123_cfg.id3_format);
    mpg123_cfg.id3_format =
        g_strdup(gtk_entry_get_text(GTK_ENTRY(title_id3_entry)));

    mpg123_cfg.title_encoding_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(title_encoding_enabled));
    mpg123_cfg.title_encoding = g_strdup(gtk_entry_get_text(GTK_ENTRY(title_encoding)));
    if (mpg123_cfg.title_encoding_enabled)
        mpg123_id3_encoding_list = g_strsplit_set(mpg123_cfg.title_encoding, ENCODING_SEPARATOR, 0);
    db = bmp_cfg_db_open();
    bmp_cfg_db_set_int(db, "MPG123", "resolution", mpg123_cfg.resolution);
    bmp_cfg_db_set_int(db, "MPG123", "channels", mpg123_cfg.channels);
    bmp_cfg_db_set_int(db, "MPG123", "downsample", mpg123_cfg.downsample);
    bmp_cfg_db_set_int(db, "MPG123", "http_buffer_size",
                       mpg123_cfg.http_buffer_size);
    bmp_cfg_db_set_int(db, "MPG123", "http_prebuffer",
                       mpg123_cfg.http_prebuffer);
    bmp_cfg_db_set_bool(db, "MPG123", "use_proxy", mpg123_cfg.use_proxy);
    bmp_cfg_db_set_string(db, "MPG123", "proxy_host", mpg123_cfg.proxy_host);
    bmp_cfg_db_set_int(db, "MPG123", "proxy_port", mpg123_cfg.proxy_port);
    bmp_cfg_db_set_bool(db, "MPG123", "proxy_use_auth",
                        mpg123_cfg.proxy_use_auth);
    if (mpg123_cfg.proxy_user)
        bmp_cfg_db_set_string(db, "MPG123", "proxy_user",
                              mpg123_cfg.proxy_user);
    else
        bmp_cfg_db_unset_key(db, "MPG123", "proxy_user");
    if (mpg123_cfg.proxy_pass)
        bmp_cfg_db_set_string(db, "MPG123", "proxy_pass",
                              mpg123_cfg.proxy_pass);
    else
        bmp_cfg_db_unset_key(db, "MPG123", "proxy_pass");
    bmp_cfg_db_set_bool(db, "MPG123", "save_http_stream",
                        mpg123_cfg.save_http_stream);
    bmp_cfg_db_set_string(db, "MPG123", "save_http_path",
                          mpg123_cfg.save_http_path);
    bmp_cfg_db_set_bool(db, "MPG123", "use_udp_channel",
                        mpg123_cfg.use_udp_channel);
    bmp_cfg_db_set_bool(db, "MPG123", "title_override",
                        mpg123_cfg.title_override);
    bmp_cfg_db_set_bool(db, "MPG123", "disable_id3v2",
                        mpg123_cfg.disable_id3v2);
    bmp_cfg_db_set_string(db, "MPG123", "id3_format", mpg123_cfg.id3_format);
    bmp_cfg_db_set_int(db, "MPG123", "detect_by", mpg123_cfg.detect_by);
#ifdef USE_SIMD
    bmp_cfg_db_set_int(db, "MPG123", "default_synth",
                       mpg123_cfg.default_synth);
#endif

/* Encoding patch */
    bmp_cfg_db_set_bool(db, "MPG123", "title_encoding_enabled", mpg123_cfg.title_encoding_enabled);
    bmp_cfg_db_set_string(db, "MPG123", "title_encoding", mpg123_cfg.title_encoding);
/* Encoding patch */

    bmp_cfg_db_close(db);
    gtk_widget_destroy(mpg123_configurewin);
}

#ifdef USE_SIMD
static void
auto_select_cb(GtkWidget * w, gpointer data)
{
    gboolean autom = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

    gtk_widget_set_sensitive(decoder_fpu, !autom);
    gtk_widget_set_sensitive(decoder_mmx, !autom);
    gtk_widget_set_sensitive(decoder_3dnow, !autom);
}

#endif

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
    if (!streaming_save_dirbrowser) {
        streaming_save_dirbrowser =
            xmms_create_dir_browser(_
                                    ("Select the directory where you want to store the MPEG streams:"),
                                    mpg123_cfg.save_http_path,
                                    GTK_SELECTION_SINGLE,
                                    streaming_save_dirbrowser_cb);
        g_signal_connect(G_OBJECT(streaming_save_dirbrowser), "destroy",
                         G_CALLBACK(gtk_widget_destroyed),
                         &streaming_save_dirbrowser);
        gtk_window_set_transient_for(GTK_WINDOW(streaming_save_dirbrowser),
                                     GTK_WINDOW(mpg123_configurewin));
        gtk_widget_show(streaming_save_dirbrowser);
    }
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
title_override_cb(GtkWidget * w, gpointer data)
{
    gboolean override;
    override =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(title_override));
    gtk_widget_set_sensitive(title_id3_box, override);
    gtk_widget_set_sensitive(title_tag_desc, override);
}

/* Encoding patch */
static void
title_encoding_enabled_cb(GtkWidget * w, gpointer data)
{
    gboolean encoding_enabled;
    encoding_enabled =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(title_encoding_enabled));
    gtk_widget_set_sensitive(title_encoding_hbox, encoding_enabled);
}
/* Encoding patch */

static void
configure_destroy(GtkWidget * w, gpointer data)
{
    if (streaming_save_dirbrowser)
        gtk_widget_destroy(streaming_save_dirbrowser);
}

void
mpg123_configure(void)
{
    GtkWidget *streaming_vbox;
    GtkWidget *streaming_buf_frame, *streaming_buf_hbox;
    GtkWidget *streaming_size_box, *streaming_size_label,
        *streaming_size_spin;
    GtkWidget *streaming_pre_box, *streaming_pre_label, *streaming_pre_spin;
    GtkWidget *streaming_proxy_frame, *streaming_proxy_vbox;
    GtkWidget *streaming_proxy_port_label, *streaming_proxy_host_label;
    GtkWidget *streaming_save_frame, *streaming_save_vbox;
    GtkWidget *streaming_save_label, *streaming_save_browse;
    GtkWidget *streaming_cast_frame, *streaming_cast_vbox;
    GtkWidget *title_frame, *title_id3_vbox, *title_id3_label;
    GtkWidget *bbox, *ok, *cancel;

    char *temp;

    if (mpg123_configurewin != NULL) {
        gtk_window_present(GTK_WINDOW(mpg123_configurewin));
        return;
    }
    mpg123_configurewin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(mpg123_configurewin),
                             GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_position(GTK_WINDOW(mpg123_configurewin),
                            GTK_WIN_POS_CENTER);
    g_signal_connect(G_OBJECT(mpg123_configurewin), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &mpg123_configurewin);
    g_signal_connect(G_OBJECT(mpg123_configurewin), "destroy",
                     G_CALLBACK(configure_destroy), &mpg123_configurewin);
    gtk_window_set_title(GTK_WINDOW(mpg123_configurewin),
                         _("MPEG Audio Plugin Configuration"));
    gtk_window_set_resizable(GTK_WINDOW(mpg123_configurewin), FALSE);
    /*  gtk_window_set_position(GTK_WINDOW(mpg123_configurewin), GTK_WIN_POS_MOUSE); */
    gtk_container_border_width(GTK_CONTAINER(mpg123_configurewin), 10);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(mpg123_configurewin), vbox);

    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    decode_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(decode_vbox), 5);

    decode_hbox1 = gtk_hbox_new(TRUE, 5);
    gtk_box_pack_start(GTK_BOX(decode_vbox), decode_hbox1, FALSE, FALSE, 0);

    decode_res_frame = gtk_frame_new(_("Resolution:"));
    gtk_box_pack_start(GTK_BOX(decode_hbox1), decode_res_frame, TRUE, TRUE,
                       0);

    decode_res_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(decode_res_vbox), 5);
    gtk_container_add(GTK_CONTAINER(decode_res_frame), decode_res_vbox);

    decode_res_16 = gtk_radio_button_new_with_label(NULL, _("16 bit"));
    if (mpg123_cfg.resolution == 16)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decode_res_16), TRUE);
    gtk_box_pack_start(GTK_BOX(decode_res_vbox), decode_res_16, FALSE,
                       FALSE, 0);

    decode_res_8 =
        gtk_radio_button_new_with_label(gtk_radio_button_group
                                        (GTK_RADIO_BUTTON(decode_res_16)),
                                        _("8 bit"));
    if (mpg123_cfg.resolution == 8)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decode_res_8), TRUE);

    gtk_box_pack_start(GTK_BOX(decode_res_vbox), decode_res_8, FALSE,
                       FALSE, 0);

    decode_ch_frame = gtk_frame_new(_("Channels:"));
    gtk_box_pack_start(GTK_BOX(decode_hbox1), decode_ch_frame, TRUE, TRUE, 0);

    decode_ch_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(decode_ch_vbox), 5);
    gtk_container_add(GTK_CONTAINER(decode_ch_frame), decode_ch_vbox);

    decode_ch_stereo =
        gtk_radio_button_new_with_label(NULL, _("Stereo (if available)"));
    if (mpg123_cfg.channels == 2)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decode_ch_stereo),
                                     TRUE);

    gtk_box_pack_start(GTK_BOX(decode_ch_vbox), decode_ch_stereo, FALSE,
                       FALSE, 0);

    decode_ch_mono =
        gtk_radio_button_new_with_label(gtk_radio_button_group
                                        (GTK_RADIO_BUTTON
                                         (decode_ch_stereo)), _("Mono"));
    if (mpg123_cfg.channels == 1)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decode_ch_mono), TRUE);

    gtk_box_pack_start(GTK_BOX(decode_ch_vbox), decode_ch_mono, FALSE,
                       FALSE, 0);

    decode_freq_frame = gtk_frame_new(_("Down sample:"));
    gtk_box_pack_start(GTK_BOX(decode_vbox), decode_freq_frame, FALSE,
                       FALSE, 0);

    decode_freq_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(decode_freq_vbox), 5);
    gtk_container_add(GTK_CONTAINER(decode_freq_frame), decode_freq_vbox);

    decode_freq_1to1 =
        gtk_radio_button_new_with_label(NULL, _("1:1 (44 kHz)"));
    if (mpg123_cfg.downsample == 0)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decode_freq_1to1),
                                     TRUE);
    gtk_box_pack_start(GTK_BOX(decode_freq_vbox), decode_freq_1to1, FALSE,
                       FALSE, 0);

    decode_freq_1to2 =
        gtk_radio_button_new_with_label(gtk_radio_button_group
                                        (GTK_RADIO_BUTTON
                                         (decode_freq_1to1)),
                                        _("1:2 (22 kHz)"));
    if (mpg123_cfg.downsample == 1)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decode_freq_1to2),
                                     TRUE);
    gtk_box_pack_start(GTK_BOX(decode_freq_vbox), decode_freq_1to2, FALSE,
                       FALSE, 0);

    decode_freq_1to4 =
        gtk_radio_button_new_with_label(gtk_radio_button_group
                                        (GTK_RADIO_BUTTON
                                         (decode_freq_1to1)),
                                        _("1:4 (11 kHz)"));
    if (mpg123_cfg.downsample == 2)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decode_freq_1to4),
                                     TRUE);

    gtk_box_pack_start(GTK_BOX(decode_freq_vbox), decode_freq_1to4, FALSE,
                       FALSE, 0);


#ifdef USE_SIMD
    {
        GtkWidget *decoder_frame, *decoder_vbox;

        decoder_frame = gtk_frame_new(_("Decoder:"));
        gtk_box_pack_start(GTK_BOX(decode_vbox), decoder_frame, FALSE,
                           FALSE, 0);

        decoder_vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(decoder_vbox), 5);
        gtk_container_add(GTK_CONTAINER(decoder_frame), decoder_vbox);

        auto_select =
            gtk_check_button_new_with_label(_("Automatic detection"));
        gtk_box_pack_start(GTK_BOX(decoder_vbox), auto_select, FALSE,
                           FALSE, 0);
        g_signal_connect(G_OBJECT(auto_select), "clicked",
                         G_CALLBACK(auto_select_cb), NULL);

        decoder_3dnow =
            gtk_radio_button_new_with_label(NULL,
                                            _("3DNow! optimized decoder"));
        gtk_box_pack_start(GTK_BOX(decoder_vbox), decoder_3dnow, FALSE,
                           FALSE, 0);

        decoder_mmx =
            gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
                                                        (decoder_3dnow),
                                                        _
                                                        ("MMX optimized decoder"));
        gtk_box_pack_start(GTK_BOX(decoder_vbox), decoder_mmx, FALSE,
                           FALSE, 0);

        decoder_fpu =
            gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
                                                        (decoder_3dnow),
                                                        _("FPU decoder"));
        gtk_box_pack_start(GTK_BOX(decoder_vbox), decoder_fpu, FALSE,
                           FALSE, 0);

        switch (mpg123_cfg.default_synth) {
        case SYNTH_3DNOW:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decoder_3dnow),
                                         TRUE);
            break;
        case SYNTH_MMX:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decoder_mmx),
                                         TRUE);
            break;
        case SYNTH_FPU:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decoder_fpu),
                                         TRUE);
            break;
        default:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decoder_fpu),
                                         TRUE);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_select),
                                         TRUE);
            break;
        }
    }
#endif
    option_frame = gtk_frame_new(_("Options"));
    gtk_box_pack_start(GTK_BOX(decode_vbox), option_frame, FALSE, FALSE, 0);

    option_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(option_vbox), 5);
    gtk_container_add(GTK_CONTAINER(option_frame), option_vbox);

    detect_by_content = gtk_radio_button_new_with_label(NULL, _("Content"));

    detect_by_extension =
        gtk_radio_button_new_with_label(gtk_radio_button_group
                                        (GTK_RADIO_BUTTON
                                         (detect_by_content)),
                                        _("Extension"));

    detect_by_both =
        gtk_radio_button_new_with_label(gtk_radio_button_group
                                        (GTK_RADIO_BUTTON
                                         (detect_by_content)),
                                        _("Extension and content"));

    switch (mpg123_cfg.detect_by) {
    case DETECT_CONTENT:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(detect_by_content),
                                     TRUE);
        break;
    case DETECT_BOTH:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(detect_by_both), TRUE);
        break;
    case DETECT_EXTENSION:
    default:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                     (detect_by_extension), TRUE);
        break;
    }

    gtk_box_pack_start(GTK_BOX(option_vbox), detect_by_content, FALSE,
                       FALSE, 0);
    gtk_box_pack_start(GTK_BOX(option_vbox), detect_by_extension, FALSE,
                       FALSE, 0);
    gtk_box_pack_start(GTK_BOX(option_vbox), detect_by_both, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), decode_vbox,
                             gtk_label_new(_("Decoder")));

    streaming_vbox = gtk_vbox_new(FALSE, 0);

    streaming_buf_frame = gtk_frame_new(_("Buffering:"));
    gtk_container_set_border_width(GTK_CONTAINER(streaming_buf_frame), 5);
    gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_buf_frame, FALSE,
                       FALSE, 0);

    streaming_buf_hbox = gtk_hbox_new(TRUE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(streaming_buf_hbox), 5);
    gtk_container_add(GTK_CONTAINER(streaming_buf_frame), streaming_buf_hbox);

    streaming_size_box = gtk_hbox_new(FALSE, 5);
    /*gtk_table_attach_defaults(GTK_TABLE(streaming_buf_table),streaming_size_box,0,1,0,1); */
    gtk_box_pack_start(GTK_BOX(streaming_buf_hbox), streaming_size_box,
                       TRUE, TRUE, 0);
    streaming_size_label = gtk_label_new(_("Buffer size (kb):"));
    gtk_box_pack_start(GTK_BOX(streaming_size_box), streaming_size_label,
                       FALSE, FALSE, 0);
    streaming_size_adj =
        gtk_adjustment_new(mpg123_cfg.http_buffer_size, 4, 4096, 4, 4, 4);
    streaming_size_spin =
        gtk_spin_button_new(GTK_ADJUSTMENT(streaming_size_adj), 8, 0);
    gtk_widget_set_usize(streaming_size_spin, 60, -1);
    gtk_box_pack_start(GTK_BOX(streaming_size_box), streaming_size_spin,
                       FALSE, FALSE, 0);

    streaming_pre_box = gtk_hbox_new(FALSE, 5);
    /*gtk_table_attach_defaults(GTK_TABLE(streaming_buf_table),streaming_pre_box,1,2,0,1); */
    gtk_box_pack_start(GTK_BOX(streaming_buf_hbox), streaming_pre_box,
                       TRUE, TRUE, 0);
    streaming_pre_label = gtk_label_new(_("Pre-buffer (percent):"));
    gtk_box_pack_start(GTK_BOX(streaming_pre_box), streaming_pre_label,
                       FALSE, FALSE, 0);
    streaming_pre_adj =
        gtk_adjustment_new(mpg123_cfg.http_prebuffer, 0, 90, 1, 1, 1);
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
                                 mpg123_cfg.use_proxy);
    g_signal_connect(G_OBJECT(streaming_proxy_use), "clicked",
                     G_CALLBACK(proxy_use_cb), NULL);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox), streaming_proxy_use,
                       FALSE, FALSE, 0);

    streaming_proxy_hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(streaming_proxy_hbox, mpg123_cfg.use_proxy);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox), streaming_proxy_hbox,
                       FALSE, FALSE, 0);

    streaming_proxy_host_label = gtk_label_new(_("Host:"));
    gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox),
                       streaming_proxy_host_label, FALSE, FALSE, 0);

    streaming_proxy_host_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(streaming_proxy_host_entry),
                       mpg123_cfg.proxy_host);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox),
                       streaming_proxy_host_entry, TRUE, TRUE, 0);

    streaming_proxy_port_label = gtk_label_new(_("Port:"));
    gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox),
                       streaming_proxy_port_label, FALSE, FALSE, 0);

    streaming_proxy_port_entry = gtk_entry_new();
    gtk_widget_set_usize(streaming_proxy_port_entry, 50, -1);
    temp = g_strdup_printf("%d", mpg123_cfg.proxy_port);
    gtk_entry_set_text(GTK_ENTRY(streaming_proxy_port_entry), temp);
    g_free(temp);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox),
                       streaming_proxy_port_entry, FALSE, FALSE, 0);

    streaming_proxy_auth_use =
        gtk_check_button_new_with_label(_("Use authentication"));
    gtk_widget_set_sensitive(streaming_proxy_auth_use, mpg123_cfg.use_proxy);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                 (streaming_proxy_auth_use),
                                 mpg123_cfg.proxy_use_auth);
    g_signal_connect(G_OBJECT(streaming_proxy_auth_use), "clicked",
                     G_CALLBACK(proxy_auth_use_cb), NULL);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox),
                       streaming_proxy_auth_use, FALSE, FALSE, 0);

    streaming_proxy_auth_hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(streaming_proxy_auth_hbox,
                             mpg123_cfg.use_proxy
                             && mpg123_cfg.proxy_use_auth);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox),
                       streaming_proxy_auth_hbox, FALSE, FALSE, 0);

    streaming_proxy_auth_user_label = gtk_label_new(_("Username:"));
    gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox),
                       streaming_proxy_auth_user_label, FALSE, FALSE, 0);

    streaming_proxy_auth_user_entry = gtk_entry_new();
    if (mpg123_cfg.proxy_user)
        gtk_entry_set_text(GTK_ENTRY(streaming_proxy_auth_user_entry),
                           mpg123_cfg.proxy_user);
    gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox),
                       streaming_proxy_auth_user_entry, TRUE, TRUE, 0);

    streaming_proxy_auth_pass_label = gtk_label_new(_("Password:"));
    gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox),
                       streaming_proxy_auth_pass_label, FALSE, FALSE, 0);

    streaming_proxy_auth_pass_entry = gtk_entry_new();
    if (mpg123_cfg.proxy_pass)
        gtk_entry_set_text(GTK_ENTRY(streaming_proxy_auth_pass_entry),
                           mpg123_cfg.proxy_pass);
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
                                 mpg123_cfg.save_http_stream);
    g_signal_connect(G_OBJECT(streaming_save_use), "clicked",
                     G_CALLBACK(streaming_save_use_cb), NULL);
    gtk_box_pack_start(GTK_BOX(streaming_save_vbox), streaming_save_use,
                       FALSE, FALSE, 0);

    streaming_save_hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(streaming_save_hbox,
                             mpg123_cfg.save_http_stream);
    gtk_box_pack_start(GTK_BOX(streaming_save_vbox), streaming_save_hbox,
                       FALSE, FALSE, 0);

    streaming_save_label = gtk_label_new(_("Path:"));
    gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_label,
                       FALSE, FALSE, 0);

    streaming_save_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(streaming_save_entry),
                       mpg123_cfg.save_http_path);
    gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_entry,
                       TRUE, TRUE, 0);

    streaming_save_browse = gtk_button_new_with_label(_("Browse"));
    g_signal_connect(G_OBJECT(streaming_save_browse), "clicked",
                     G_CALLBACK(streaming_save_browse_cb), NULL);
    gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_browse,
                       FALSE, FALSE, 0);

    streaming_cast_frame = gtk_frame_new(_("SHOUT/Icecast:"));
    gtk_container_set_border_width(GTK_CONTAINER(streaming_cast_frame), 5);
    gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_cast_frame,
                       FALSE, FALSE, 0);

    streaming_cast_vbox = gtk_vbox_new(5, FALSE);
    gtk_container_add(GTK_CONTAINER(streaming_cast_frame),
                      streaming_cast_vbox);

    gtk_box_pack_start(GTK_BOX(streaming_cast_vbox), streaming_cast_title,
                       FALSE, FALSE, 0);

    streaming_udp_title =
        gtk_check_button_new_with_label(_
                                        ("Enable Icecast Metadata UDP Channel"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(streaming_udp_title),
                                 mpg123_cfg.use_udp_channel);
    gtk_box_pack_start(GTK_BOX(streaming_cast_vbox), streaming_udp_title,
                       FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), streaming_vbox,
                             gtk_label_new(_("Streaming")));

    title_frame = gtk_frame_new(_("ID3 Tags:"));
    gtk_container_border_width(GTK_CONTAINER(title_frame), 5);

    title_id3_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_border_width(GTK_CONTAINER(title_id3_vbox), 5);
    gtk_container_add(GTK_CONTAINER(title_frame), title_id3_vbox);

    title_id3v2_disable =
        gtk_check_button_new_with_label(_("Disable ID3V2 tags"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(title_id3v2_disable),
                                 mpg123_cfg.disable_id3v2);
    gtk_box_pack_start(GTK_BOX(title_id3_vbox), title_id3v2_disable, FALSE,
                       FALSE, 0);


/* Encoding patch */
    title_encoding_enabled =
        gtk_check_button_new_with_label(_("Convert non-UTF8 ID3 tags to UTF8"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(title_encoding_enabled),
                                 mpg123_cfg.title_encoding_enabled);
    g_signal_connect(G_OBJECT(title_encoding_enabled), "clicked",
                     G_CALLBACK(title_encoding_enabled_cb), NULL);
    gtk_box_pack_start(GTK_BOX(title_id3_vbox), title_encoding_enabled, FALSE,
                       FALSE, 0);

    title_encoding_hbox = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(title_encoding_hbox, mpg123_cfg.title_encoding_enabled);
    gtk_box_pack_start(GTK_BOX(title_id3_vbox), title_encoding_hbox, FALSE,
                       FALSE, 0);

    title_encoding_label = gtk_label_new(_("ID3 encoding:"));
    gtk_box_pack_start(GTK_BOX(title_encoding_hbox), title_encoding_label, FALSE,
                       FALSE, 0);

    title_encoding = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(title_encoding), mpg123_cfg.title_encoding);
    gtk_box_pack_start(GTK_BOX(title_encoding_hbox), title_encoding, TRUE, TRUE,
                       0);
/* Encoding patch */


    title_override =
        gtk_check_button_new_with_label(_("Override generic titles"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(title_override),
                                 mpg123_cfg.title_override);
    g_signal_connect(G_OBJECT(title_override), "clicked",
                     G_CALLBACK(title_override_cb), NULL);
    gtk_box_pack_start(GTK_BOX(title_id3_vbox), title_override, FALSE,
                       FALSE, 0);

    title_id3_box = gtk_hbox_new(FALSE, 5);
    gtk_widget_set_sensitive(title_id3_box, mpg123_cfg.title_override);
    gtk_box_pack_start(GTK_BOX(title_id3_vbox), title_id3_box, FALSE,
                       FALSE, 0);

    title_id3_label = gtk_label_new(_("ID3 format:"));
    gtk_box_pack_start(GTK_BOX(title_id3_box), title_id3_label, FALSE,
                       FALSE, 0);

    title_id3_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(title_id3_entry), mpg123_cfg.id3_format);
    gtk_box_pack_start(GTK_BOX(title_id3_box), title_id3_entry, TRUE, TRUE,
                       0);

    title_tag_desc = xmms_titlestring_descriptions("pafFetnygc", 2);
    gtk_widget_set_sensitive(title_tag_desc, mpg123_cfg.title_override);
    gtk_box_pack_start(GTK_BOX(title_id3_vbox), title_tag_desc, FALSE,
                       FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), title_frame,
                             gtk_label_new(_("Title")));

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect_swapped(G_OBJECT(cancel), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             GTK_OBJECT(mpg123_configurewin));
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);


    ok = gtk_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect(G_OBJECT(ok), "clicked",
                     G_CALLBACK(mpg123_configurewin_ok), NULL);
    GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
    gtk_widget_grab_default(ok);

    gtk_widget_show_all(mpg123_configurewin);
}
