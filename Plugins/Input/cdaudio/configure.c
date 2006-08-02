/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */

#include "cdaudio.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include <libaudacious/configdb.h>
#include <libaudacious/titlestring.h>


#define GET_TB(b) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b))
#define SET_TB(b) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE)

struct driveconfig {
    GtkWidget *device, *directory;
    GtkWidget *mixer_oss, *mixer_drive;
    GtkWidget *remove_button;
    GtkWidget *dae;
};

static GList *drives;

static GtkWidget *cdda_configure_win;
static GtkWidget *cdi_name, *cdi_name_override;
static GtkWidget *cdi_use_cddb, *cdi_cddb_server, *cdi_use_cdin,
    *cdi_cdin_server;

void cdda_cddb_show_server_dialog(GtkWidget * w, gpointer data);
void cdda_cddb_show_network_window(GtkWidget * w, gpointer data);
void cdda_cddb_set_server(const char *new_server);

static GtkWidget *configurewin_add_drive(struct driveinfo *drive,
                                         gpointer nb);

static void
cdda_configurewin_ok_cb(GtkWidget * w, gpointer data)
{
    ConfigDb *db;
    struct driveinfo *drive;
    GList *node;
    gint olddrives, ndrives, i;

    olddrives = g_list_length(cdda_cfg.drives);
    for (node = cdda_cfg.drives; node; node = node->next) {
        drive = node->data;
        g_free(drive->device);
        g_free(drive->directory);
        g_free(drive);
    }
    g_list_free(cdda_cfg.drives);
    cdda_cfg.drives = NULL;

    for (node = drives; node; node = node->next) {
        struct driveconfig *config = node->data;
        const gchar *tmp;

        drive = g_new0(struct driveinfo, 1);
        drive->device =
            g_strdup(gtk_entry_get_text(GTK_ENTRY(config->device)));

        tmp = gtk_entry_get_text(GTK_ENTRY(config->directory));
//              if (strlen(tmp) < 2 || tmp[strlen(tmp) - 1] == '/')
        drive->directory = g_strdup(tmp);
//              else
//                      drive->directory = g_strconcat(tmp, "/", NULL);

//              drive->directory = "CD_AUDIO";

        if (GET_TB(config->mixer_oss))
            drive->mixer = CDDA_MIXER_OSS;
        else if (GET_TB(config->mixer_drive))
            drive->mixer = CDDA_MIXER_DRIVE;
        else
            drive->mixer = CDDA_MIXER_NONE;
        if (GET_TB(config->dae))
            drive->dae = CDDA_READ_DAE;
        else
            drive->dae = CDDA_READ_ANALOG;

        cdda_cfg.drives = g_list_append(cdda_cfg.drives, drive);
    }

    cdda_cfg.title_override = GET_TB(cdi_name_override);
    g_free(cdda_cfg.name_format);
    cdda_cfg.name_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(cdi_name)));

    cdda_cfg.use_cddb = GET_TB(cdi_use_cddb);
    cdda_cddb_set_server(gtk_entry_get_text(GTK_ENTRY(cdi_cddb_server)));

    cdda_cfg.use_cdin = GET_TB(cdi_use_cdin);
    if (strcmp
        (cdda_cfg.cdin_server,
         gtk_entry_get_text(GTK_ENTRY(cdi_cdin_server)))) {
        g_free(cdda_cfg.cdin_server);
        cdda_cfg.cdin_server =
            g_strdup(gtk_entry_get_text(GTK_ENTRY(cdi_cdin_server)));
    }

    db = bmp_cfg_db_open();

    drive = cdda_cfg.drives->data;
    bmp_cfg_db_set_string(db, "CDDA", "device", drive->device);
    bmp_cfg_db_set_string(db, "CDDA", "directory", drive->directory);
//      bmp_cfg_db_set_string(db, "CDDA", "directory", "CD_AUDIO");
    bmp_cfg_db_set_int(db, "CDDA", "mixer", drive->mixer);
    bmp_cfg_db_set_int(db, "CDDA", "readmode", drive->dae);

/*  	bmp_cfg_db_set_bool(db, "CDDA", "use_oss_mixer", cdda_cfg.use_oss_mixer); */

    for (node = cdda_cfg.drives->next, i = 1; node; node = node->next, i++) {
        char label[20];
        drive = node->data;

        sprintf(label, "device%d", i);
        bmp_cfg_db_set_string(db, "CDDA", label, drive->device);

        sprintf(label, "directory%d", i);
//              bmp_cfg_db_set_string(db, "CDDA", label, "CD_AUDIO");
        bmp_cfg_db_set_string(db, "CDDA", label, drive->directory);

        sprintf(label, "mixer%d", i);
        bmp_cfg_db_set_int(db, "CDDA", label, drive->mixer);

        sprintf(label, "readmode%d", i);
        bmp_cfg_db_set_int(db, "CDDA", label, drive->dae);
    }

    ndrives = g_list_length(cdda_cfg.drives);

    for (i = ndrives; i < olddrives; i++)
        /* FIXME: Clear old entries */ ;

    bmp_cfg_db_set_int(db, "CDDA", "num_drives", ndrives);

    bmp_cfg_db_set_bool(db, "CDDA", "title_override",
                        cdda_cfg.title_override);
    bmp_cfg_db_set_string(db, "CDDA", "name_format", cdda_cfg.name_format);
    bmp_cfg_db_set_bool(db, "CDDA", "use_cddb", cdda_cfg.use_cddb);
    bmp_cfg_db_set_string(db, "CDDA", "cddb_server", cdda_cfg.cddb_server);
    bmp_cfg_db_set_int(db, "CDDA", "cddb_protocol_level",
                       cdda_cfg.cddb_protocol_level);
    bmp_cfg_db_set_bool(db, "CDDA", "use_cdin", cdda_cfg.use_cdin);
    bmp_cfg_db_set_string(db, "CDDA", "cdin_server", cdda_cfg.cdin_server);
    bmp_cfg_db_close(db);
}

static void
configurewin_close(GtkButton * w, gpointer data)
{
    GList *node;

    for (node = drives; node; node = node->next)
        g_free(node->data);
    g_list_free(drives);
    drives = NULL;

    gtk_widget_destroy(cdda_configure_win);
}

static void
toggle_set_sensitive_cb(GtkToggleButton * w, gpointer data)
{
    gboolean set = gtk_toggle_button_get_active(w);
    gtk_widget_set_sensitive(GTK_WIDGET(data), set);
}

static void
configurewin_add_page(GtkButton * w, gpointer data)
{
    GtkNotebook *nb = GTK_NOTEBOOK(data);
    GtkWidget *box = configurewin_add_drive(NULL, nb);
    gchar *label = g_strdup_printf(_("Drive %d"), g_list_length(drives));

    gtk_widget_show_all(box);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), box, gtk_label_new(label));
    g_free(label);
}

static void
redo_nb_labels(GtkNotebook * nb)
{
    gint i;
    GtkWidget *child;

    for (i = 0; (child = gtk_notebook_get_nth_page(nb, i)) != NULL; i++) {
        gchar *label = g_strdup_printf(_("Drive %d"), i + 1);

        gtk_notebook_set_tab_label_text(nb, child, label);
        g_free(label);
    }
}


static void
configurewin_remove_page(GtkButton * w, gpointer data)
{
    GList *node;
    GtkNotebook *nb = GTK_NOTEBOOK(data);
    gtk_notebook_remove_page(nb, gtk_notebook_get_current_page(nb));
    for (node = drives; node; node = node->next) {
        struct driveconfig *drive = node->data;

        if (GTK_WIDGET(w) == drive->remove_button) {
            if (node->next)
                redo_nb_labels(nb);
            drives = g_list_remove(drives, drive);
            g_free(drive);
            break;
        }
    }
    if (g_list_length(drives) == 1) {
        struct driveconfig *drive = drives->data;
        gtk_widget_set_sensitive(drive->remove_button, FALSE);
    }
}


static void
configurewin_check_drive(GtkButton * w, gpointer data)
{
    struct driveconfig *drive = data;
    GtkWidget *window, *vbox, *label, *bbox, *closeb;
    const gchar *device, *directory;
    gint fd, dae_track = -1;
    GString *str = g_string_new("");
    struct stat stbuf;

    device = gtk_entry_get_text(GTK_ENTRY(drive->device));
    directory = gtk_entry_get_text(GTK_ENTRY(drive->directory));

    if ((fd = open(device, CDOPENFLAGS) < 0))
        g_string_sprintfa(str, _("Failed to open device %s\n"
                                 "Error: %s\n\n"), device, strerror(errno));
    else {
        cdda_disc_toc_t toc;
        close(fd);
        if (!cdda_get_toc(&toc, device))
            g_string_append(str,
                            _("Failed to read \"Table of Contents\""
                              "\nMaybe no disc in the drive?\n\n"));
        else {
            gint i, data = 0;
            g_string_sprintfa(str, _("Device %s OK.\n"
                                     "Disc has %d tracks"), device,
                              toc.last_track - toc.first_track + 1);
            for (i = toc.first_track; i <= toc.last_track; i++)
                if (toc.track[i].flags.data_track)
                    data++;
                else if (dae_track < 0)
                    dae_track = i;
            if (data > 0)
                g_string_sprintfa(str, _(" (%d data tracks)"), data);
            g_string_sprintfa(str, _("\nTotal length: %d:%d\n"),
                              toc.leadout.minute, toc.leadout.second);
#ifdef CDDA_HAS_READAUDIO
            if (dae_track == -1)
                g_string_sprintfa(str,
                                  _("Digital audio extraction "
                                    "not tested as the disc has "
                                    "no audio tracks\n"));
            else {
                gint fd = open(device, CDOPENFLAGS);
                gint start, end, fr;
                gchar buffer[CD_FRAMESIZE_RAW];
                start = LBA(toc.track[dae_track]);

                if (dae_track == toc.last_track)
                    end = LBA(toc.leadout);
                else
                    end = LBA(toc.track[dae_track + 1]);
                fr = read_audio_data(fd, start + (end - start) / 2,
                                     1, buffer);
                if (fr > 0)
                    g_string_sprintfa(str,
                                      _("Digital audio extraction "
                                        "test: OK\n\n"));
                else
                    g_string_sprintfa(str,
                                      _("Digital audio extraction "
                                        "test failed: %s\n\n"),
                                      strerror(-fr));
            }
#else
            g_string_sprintfa(str, "\n");
#endif
        }
    }
    if (stat(directory, &stbuf) < 0) {
        g_string_sprintfa(str, _("Failed to check directory %s\n"
                                 "Error: %s"), directory, strerror(errno));
    }
    else {
        if (!S_ISDIR(stbuf.st_mode))
            g_string_sprintfa(str,
                              _("Error: %s exist, but is not a directory"),
                              directory);
        else
            g_string_sprintfa(str, _("Directory %s OK."), directory);
    }


    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_transient_for(GTK_WINDOW(window),
                                 GTK_WINDOW(cdda_configure_win));
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    label = gtk_label_new(str->str);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    closeb = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    GTK_WIDGET_SET_FLAGS(closeb, GTK_CAN_DEFAULT);
    g_signal_connect_swapped(G_OBJECT(closeb), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             GTK_OBJECT(window));
    gtk_box_pack_start(GTK_BOX(bbox), closeb, TRUE, TRUE, 0);
    gtk_widget_grab_default(closeb);

    g_string_free(str, TRUE);

    gtk_widget_show_all(window);
}

static GtkWidget *
configurewin_add_drive(struct driveinfo *drive, gpointer nb)
{
    GtkWidget *vbox, *bbox, *dev_frame, *dev_table, *dev_label;
    GtkWidget *dev_dir_label, *check_btn;
    GtkWidget *volume_frame, *volume_box, *volume_none;
    GtkWidget *readmode_frame, *readmode_box, *readmode_analog;
    struct driveconfig *d = g_new0(struct driveconfig, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

    dev_frame = gtk_frame_new(_("Device:"));
    gtk_box_pack_start(GTK_BOX(vbox), dev_frame, FALSE, FALSE, 0);
    dev_table = gtk_table_new(2, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(dev_table), 5);
    gtk_container_add(GTK_CONTAINER(dev_frame), dev_table);
    gtk_table_set_row_spacings(GTK_TABLE(dev_table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(dev_table), 5);

    dev_label = gtk_label_new_with_mnemonic(_("_Device:"));
    gtk_misc_set_alignment(GTK_MISC(dev_label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(dev_table), dev_label, 0, 1, 0, 1,
                     GTK_FILL, 0, 0, 0);

    d->device = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(dev_label), d->device);
    gtk_table_attach(GTK_TABLE(dev_table), d->device, 1, 2, 0, 1,
                     GTK_FILL | GTK_EXPAND, 0, 0, 0);

    dev_dir_label = gtk_label_new_with_mnemonic(_("Dir_ectory:"));
    gtk_misc_set_alignment(GTK_MISC(dev_dir_label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(dev_table), dev_dir_label, 0, 1, 1, 2,
                     GTK_FILL, 0, 0, 0);


    d->directory = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(dev_dir_label), d->directory);
    gtk_table_attach(GTK_TABLE(dev_table), d->directory, 1, 2, 1, 2,
                     GTK_FILL | GTK_EXPAND, 0, 0, 0);


    readmode_frame = gtk_frame_new(_("Play mode:"));
    gtk_box_pack_start(GTK_BOX(vbox), readmode_frame, FALSE, FALSE, 0);

    readmode_box = gtk_vbox_new(5, FALSE);
    gtk_container_add(GTK_CONTAINER(readmode_frame), readmode_box);

    readmode_analog = gtk_radio_button_new_with_label(NULL, _("Analog"));
    gtk_box_pack_start(GTK_BOX(readmode_box), readmode_analog, FALSE,
                       FALSE, 0);

    d->dae =
        gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
                                                    (readmode_analog),
                                                    _
                                                    ("Digital audio extraction"));
    gtk_box_pack_start(GTK_BOX(readmode_box), d->dae, FALSE, FALSE, 0);
#ifndef CDDA_HAS_READAUDIO
    gtk_widget_set_sensitive(readmode_frame, FALSE);
#endif

    /*
     * Volume config
     */

    volume_frame = gtk_frame_new(_("Volume control:"));
    gtk_box_pack_start(GTK_BOX(vbox), volume_frame, FALSE, FALSE, 0);

    volume_box = gtk_vbox_new(5, FALSE);
    gtk_container_add(GTK_CONTAINER(volume_frame), volume_box);

    volume_none = gtk_radio_button_new_with_label(NULL, _("No mixer"));
    gtk_box_pack_start(GTK_BOX(volume_box), volume_none, FALSE, FALSE, 0);

    d->mixer_drive =
        gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
                                                    (volume_none),
                                                    _("CDROM drive"));
    gtk_box_pack_start(GTK_BOX(volume_box), d->mixer_drive, FALSE, FALSE, 0);

    d->mixer_oss =
        gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON
                                                    (volume_none),
                                                    _("OSS mixer"));
    gtk_box_pack_start(GTK_BOX(volume_box), d->mixer_oss, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(readmode_analog), "toggled",
                     G_CALLBACK(toggle_set_sensitive_cb), volume_frame);
#ifndef HAVE_OSS
    gtk_widget_set_sensitive(d->mixer_oss, FALSE);
#endif
    if (drive) {
        gtk_entry_set_text(GTK_ENTRY(d->device), drive->device);
        gtk_entry_set_text(GTK_ENTRY(d->directory), drive->directory);
        if (drive->mixer == CDDA_MIXER_DRIVE)
            SET_TB(d->mixer_drive);
        else if (drive->mixer == CDDA_MIXER_OSS)
            SET_TB(d->mixer_oss);
        if (drive->dae == CDDA_READ_DAE)
            SET_TB(d->dae);
    }

    bbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);

    check_btn = gtk_button_new_with_label(_("Check drive..."));
    GTK_WIDGET_SET_FLAGS(check_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start_defaults(GTK_BOX(bbox), check_btn);
    g_signal_connect(G_OBJECT(check_btn), "clicked",
                     G_CALLBACK(configurewin_check_drive), d);

    d->remove_button = gtk_button_new_with_label(_("Remove drive"));
    GTK_WIDGET_SET_FLAGS(d->remove_button, GTK_CAN_DEFAULT);
    gtk_box_pack_start_defaults(GTK_BOX(bbox), d->remove_button);
    g_signal_connect(G_OBJECT(d->remove_button), "clicked",
                     G_CALLBACK(configurewin_remove_page), nb);


    if (drives == NULL)
        gtk_widget_set_sensitive(d->remove_button, FALSE);
    else {
        struct driveconfig *tmp = drives->data;
        gtk_widget_set_sensitive(tmp->remove_button, TRUE);
    }

    drives = g_list_append(drives, d);

    return vbox;
}

void
cdda_configure(void)
{
    GtkWidget *vbox, *notebook;
    GtkWidget *dev_vbox, *dev_notebook, *add_drive, *add_bbox;
    GtkWidget *cdi_vbox;
    GtkWidget *cdi_cddb_frame, *cdi_cddb_vbox, *cdi_cddb_hbox;
    GtkWidget *cdi_cddb_server_hbox, *cdi_cddb_server_label;
    GtkWidget *cdi_cddb_server_list, *cdi_cddb_debug_win;
    GtkWidget *cdi_cdin_frame, *cdi_cdin_vbox;
    GtkWidget *cdi_cdin_server_hbox, *cdi_cdin_server_label;
    GtkWidget *cdi_name_frame, *cdi_name_vbox, *cdi_name_hbox;
    GtkWidget *cdi_name_label, *cdi_desc;
    GtkWidget *cdi_name_enable_vbox;
    GtkWidget *bbox, *ok, *cancel;

    GList *node;
    gint i = 1;

    if (cdda_configure_win)
        return;

    cdda_configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(cdda_configure_win), "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &cdda_configure_win);
    gtk_window_set_title(GTK_WINDOW(cdda_configure_win),
                         _("CD Audio Player Configuration"));
    gtk_window_set_type_hint(GTK_WINDOW(cdda_configure_win),
                             GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_resizable(GTK_WINDOW(cdda_configure_win), FALSE);
    gtk_window_set_position(GTK_WINDOW(cdda_configure_win),
                            GTK_WIN_POS_MOUSE);
    gtk_container_border_width(GTK_CONTAINER(cdda_configure_win), 10);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(cdda_configure_win), vbox);

    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    /*
     * Device config
     */
    dev_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(dev_vbox), 5);

    dev_notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(dev_notebook), TRUE);
    gtk_box_pack_start(GTK_BOX(dev_vbox), dev_notebook, FALSE, FALSE, 0);

    for (node = cdda_cfg.drives; node; node = node->next) {
        struct driveinfo *drive = node->data;
        gchar *label = g_strdup_printf(_("Drive %d"), i++);
        GtkWidget *w;

        w = configurewin_add_drive(drive, dev_notebook);
        gtk_notebook_append_page(GTK_NOTEBOOK(dev_notebook), w,
                                 gtk_label_new(label));
        g_free(label);

    }

    add_bbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(dev_vbox), add_bbox, FALSE, FALSE, 0);
    add_drive = gtk_button_new_with_label(_("Add drive"));
    g_signal_connect(G_OBJECT(add_drive), "clicked",
                     G_CALLBACK(configurewin_add_page), dev_notebook);
    GTK_WIDGET_SET_FLAGS(add_drive, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(add_bbox), add_drive, FALSE, FALSE, 0);


    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dev_vbox,
                             gtk_label_new(_("Device")));

    /*
     * CD Info config
     */
    cdi_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(cdi_vbox), 5);


    /* CDDB */
    cdi_cddb_frame = gtk_frame_new(_("CDDB:"));
    gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_cddb_frame, FALSE, FALSE, 0);

    cdi_cddb_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_border_width(GTK_CONTAINER(cdi_cddb_vbox), 5);
    gtk_container_add(GTK_CONTAINER(cdi_cddb_frame), cdi_cddb_vbox);

    cdi_cddb_hbox = gtk_hbox_new(FALSE, 10);
    gtk_container_border_width(GTK_CONTAINER(cdi_cddb_hbox), 0);
    gtk_box_pack_start(GTK_BOX(cdi_cddb_vbox),
                       cdi_cddb_hbox, FALSE, FALSE, 0);
    cdi_use_cddb = gtk_check_button_new_with_label(_("Use CDDB"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_use_cddb),
                                 cdda_cfg.use_cddb);
    gtk_box_pack_start(GTK_BOX(cdi_cddb_hbox), cdi_use_cddb, FALSE, FALSE, 0);
    cdi_cddb_server_list = gtk_button_new_with_label(_("Get server list"));
    gtk_box_pack_end(GTK_BOX(cdi_cddb_hbox), cdi_cddb_server_list, FALSE,
                     FALSE, 0);
    cdi_cddb_debug_win = gtk_button_new_with_label(_("Show network window"));
    g_signal_connect(G_OBJECT(cdi_cddb_debug_win), "clicked",
                     G_CALLBACK(cdda_cddb_show_network_window), NULL);
    gtk_box_pack_end(GTK_BOX(cdi_cddb_hbox), cdi_cddb_debug_win, FALSE,
                     FALSE, 0);

    cdi_cddb_server_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(cdi_cddb_vbox),
                       cdi_cddb_server_hbox, FALSE, FALSE, 0);

    cdi_cddb_server_label = gtk_label_new(_("CDDB server:"));
    gtk_box_pack_start(GTK_BOX(cdi_cddb_server_hbox),
                       cdi_cddb_server_label, FALSE, FALSE, 0);

    cdi_cddb_server = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(cdi_cddb_server), cdda_cfg.cddb_server);
    gtk_box_pack_start(GTK_BOX(cdi_cddb_server_hbox), cdi_cddb_server,
                       TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(cdi_cddb_server_list), "clicked",
                     G_CALLBACK(cdda_cddb_show_server_dialog),
                     cdi_cddb_server);

    /*
     * CDindex
     */
    cdi_cdin_frame = gtk_frame_new(_("CD Index:"));
    gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_cdin_frame, FALSE, FALSE, 0);

    cdi_cdin_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_border_width(GTK_CONTAINER(cdi_cdin_vbox), 5);
    gtk_container_add(GTK_CONTAINER(cdi_cdin_frame), cdi_cdin_vbox);

    cdi_use_cdin = gtk_check_button_new_with_label(_("Use CD Index"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_use_cdin),
                                 cdda_cfg.use_cdin);
    gtk_box_pack_start(GTK_BOX(cdi_cdin_vbox), cdi_use_cdin, FALSE, FALSE, 0);

    cdi_cdin_server_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(cdi_cdin_vbox), cdi_cdin_server_hbox, FALSE,
                       FALSE, 0);

    cdi_cdin_server_label = gtk_label_new(_("CD Index server:"));
    gtk_box_pack_start(GTK_BOX(cdi_cdin_server_hbox),
                       cdi_cdin_server_label, FALSE, FALSE, 0);

    cdi_cdin_server = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(cdi_cdin_server), cdda_cfg.cdin_server);
    gtk_box_pack_start(GTK_BOX(cdi_cdin_server_hbox), cdi_cdin_server,
                       TRUE, TRUE, 0);
#ifndef WITH_CDINDEX
    gtk_widget_set_sensitive(cdi_cdin_frame, FALSE);
#endif

    /*
     * Track names
     */
    cdi_name_frame = gtk_frame_new(_("Track names:"));
    gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_name_frame, FALSE, FALSE, 0);

    cdi_name_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(cdi_name_frame), cdi_name_vbox);
    gtk_container_border_width(GTK_CONTAINER(cdi_name_vbox), 5);
    cdi_name_override =
        gtk_check_button_new_with_label(_("Override generic titles"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_name_override),
                                 cdda_cfg.title_override);
    gtk_box_pack_start(GTK_BOX(cdi_name_vbox), cdi_name_override, FALSE,
                       FALSE, 0);

    cdi_name_enable_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(cdi_name_vbox), cdi_name_enable_vbox);
    gtk_widget_set_sensitive(cdi_name_enable_vbox, cdda_cfg.title_override);
    g_signal_connect(G_OBJECT(cdi_name_override), "toggled",
                     G_CALLBACK(toggle_set_sensitive_cb),
                     cdi_name_enable_vbox);

    cdi_name_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(cdi_name_enable_vbox), cdi_name_hbox, FALSE,
                       FALSE, 0);
    cdi_name_label = gtk_label_new(_("Name format:"));
    gtk_box_pack_start(GTK_BOX(cdi_name_hbox), cdi_name_label, FALSE,
                       FALSE, 0);
    cdi_name = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(cdi_name), cdda_cfg.name_format);
    gtk_box_pack_start(GTK_BOX(cdi_name_hbox), cdi_name, TRUE, TRUE, 0);

    cdi_desc = xmms_titlestring_descriptions("patn", 2);
    gtk_box_pack_start(GTK_BOX(cdi_name_enable_vbox), cdi_desc, FALSE,
                       FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cdi_vbox,
                             gtk_label_new(_("CD Info")));

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);


    cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(cancel), "clicked",
                     G_CALLBACK(configurewin_close), NULL);
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

    ok = gtk_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect(G_OBJECT(ok), "clicked",
                     G_CALLBACK(cdda_configurewin_ok_cb), NULL);
    g_signal_connect(G_OBJECT(ok), "clicked",
                     G_CALLBACK(configurewin_close), NULL);
    GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
    gtk_widget_grab_default(ok);


    gtk_widget_show_all(cdda_configure_win);
}
