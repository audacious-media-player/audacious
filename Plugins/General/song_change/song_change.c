/*
 * Audacious: A cross-platform multimedia player.
 * Copyright (c) 2005  Audacious Team
 */
#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>

#include "audacious/plugin.h"
#include "audacious/prefswin.h"
#include "libaudacious/configdb.h"
#include "libaudacious/beepctrl.h"
#include "libaudacious/formatter.h"

static void init(void);
static void cleanup(void);
static GtkWidget *configure(void);
static int timeout_func(gpointer);

static int timeout_tag = 0;
static int previous_song = -1;
static char *cmd_line = NULL;
static char *cmd_line_after = NULL;
static char *cmd_line_end = NULL;
static gboolean possible_pl_end;

static GtkWidget *configure_win = NULL, *configure_vbox;
static GtkWidget *cmd_entry, *cmd_after_entry, *cmd_end_entry;

GeneralPlugin sc_gp =
{
	NULL,			/* handle */
	NULL,			/* filename */
	-1,			/* xmms_session */
	NULL,			/* Description */
	init,
	NULL,
	NULL,
	cleanup,
};

GeneralPlugin *get_gplugin_info(void)
{
	sc_gp.description = g_strdup_printf(_("Song Change %s"), PACKAGE_VERSION);
	return &sc_gp;
}

static void read_config(void)
{
	ConfigDb *db;

	cmd_line = g_strdup("");
	cmd_line_after = g_strdup("");
	cmd_line_end = g_strdup("");

	db = bmp_cfg_db_open();
	bmp_cfg_db_get_string(db, "song_change", "cmd_line", &cmd_line);
	bmp_cfg_db_get_string(db, "song_change", "cmd_line_after", &cmd_line_after);
	bmp_cfg_db_get_string(db, "song_change", "cmd_line_end", &cmd_line_end);
	bmp_cfg_db_close(db);
}

static void cleanup(void)
{
	if (timeout_tag)
		gtk_timeout_remove(timeout_tag);
	timeout_tag = 0;

	g_free(cmd_line);
	g_free(cmd_line_after);
	g_free(cmd_line_end);
	cmd_line = NULL;
	cmd_line_after = NULL;
	cmd_line_end = NULL;
	signal(SIGCHLD, SIG_DFL);

	prefswin_page_destroy(configure_vbox);
}

static void save_and_close(GtkWidget *w, gpointer data)
{
	ConfigDb *db;
	char *cmd, *cmd_after, *cmd_end;

	cmd = g_strdup(gtk_entry_get_text(GTK_ENTRY(cmd_entry)));
	cmd_after = g_strdup(gtk_entry_get_text(GTK_ENTRY(cmd_after_entry)));
	cmd_end = g_strdup(gtk_entry_get_text(GTK_ENTRY(cmd_end_entry)));

	db = bmp_cfg_db_open();
	bmp_cfg_db_set_string(db, "song_change", "cmd_line", cmd);
	bmp_cfg_db_set_string(db, "song_change", "cmd_line_after", cmd_after);
	bmp_cfg_db_set_string(db, "song_change", "cmd_line_end", cmd_end);
	bmp_cfg_db_close(db);

	if (timeout_tag)
	{
		g_free(cmd_line);
		cmd_line = g_strdup(cmd);
		g_free(cmd_line_after);
		cmd_line_after = g_strdup(cmd_after);
		g_free(cmd_line_end);
		cmd_line_end = g_strdup(cmd_end);
	}
	gtk_widget_destroy(configure_win);

	g_free(cmd);
	g_free(cmd_after);
	g_free(cmd_end);
}

#if 0
static void warn_user(void)
{
	GtkWidget *warn_win, *warn_vbox, *warn_desc;
	GtkWidget *warn_bbox, *warn_yes, *warn_no;

	warn_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(warn_win), _("Warning"));
	gtk_window_set_transient_for(GTK_WINDOW(warn_win),
				     GTK_WINDOW(configure_win));
	gtk_window_set_modal(GTK_WINDOW(warn_win), TRUE);

	gtk_container_set_border_width(GTK_CONTAINER(warn_win), 10);

	warn_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(warn_win), warn_vbox);

	warn_desc = gtk_label_new(_(
		"Filename and song title tags should be inside "
		"double quotes (\").  Not doing so might be a "
		"security risk.  Continue anyway?"));
	gtk_label_set_justify(GTK_LABEL(warn_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(warn_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(warn_vbox), warn_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(warn_desc), TRUE);

	warn_bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(warn_bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(warn_bbox), 5);
	gtk_box_pack_start(GTK_BOX(warn_vbox), warn_bbox, FALSE, FALSE, 0);

	warn_yes = gtk_button_new_with_label(_("Yes"));
	gtk_signal_connect(GTK_OBJECT(warn_yes), "clicked",
			   GTK_SIGNAL_FUNC(save_and_close), NULL);
	gtk_signal_connect_object(GTK_OBJECT(warn_yes), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(warn_win));
	GTK_WIDGET_SET_FLAGS(warn_yes, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(warn_bbox), warn_yes, TRUE, TRUE, 0);
	gtk_widget_grab_default(warn_yes);

	warn_no = gtk_button_new_with_label(_("No"));
	gtk_signal_connect_object(GTK_OBJECT(warn_no), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(warn_win));
	GTK_WIDGET_SET_FLAGS(warn_no, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(warn_bbox), warn_no, TRUE, TRUE, 0);

	gtk_widget_show_all(warn_win);
}
#endif

static int check_command(char *command)
{
	const char *dangerous = "fns";
	char *c;
	int qu = 0;

	for (c = command; *c != '\0'; c++)
	{
		if (*c == '"' && (c == command || *(c - 1) != '\\'))
			qu = !qu;
		else if (*c == '%' && !qu && strchr(dangerous, *(c + 1)))
			return -1;
	}
	return 0;
}

static void configure_ok_cb(GtkWidget *w, gpointer data)
{
	char *cmd, *cmd_after, *cmd_end;

	cmd = g_strdup(gtk_entry_get_text(GTK_ENTRY(cmd_entry)));
	cmd_after = g_strdup(gtk_entry_get_text(GTK_ENTRY(cmd_after_entry)));
	cmd_end = g_strdup(gtk_entry_get_text(GTK_ENTRY(cmd_end_entry)));

#if 0
	if (check_command(cmd) < 0 || check_command(cmd_after) < 0
	                           || check_command(cmd_end) < 0)
		warn_user();
	else
#endif
		save_and_close(NULL, NULL);
	g_free(cmd);
	g_free(cmd_after);
	g_free(cmd_end);
}


static GtkWidget *configure(void)
{
	GtkWidget *sep1, *sep2, *sep3;
	GtkWidget *cmd_hbox, *cmd_label;
	GtkWidget *cmd_after_hbox, *cmd_after_label;
	GtkWidget *cmd_end_hbox, *cmd_end_label;
	GtkWidget *cmd_desc, *cmd_after_desc, *cmd_end_desc, *f_desc;
	GtkWidget *configure_bbox, *configure_ok;
	GtkWidget *song_frame, *song_vbox;
	char *temp;
	
	read_config();

	configure_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), configure_vbox);

	song_frame = gtk_frame_new(_("Commands"));
	gtk_box_pack_start(GTK_BOX(configure_vbox), song_frame, FALSE, FALSE, 0);
	song_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(song_vbox), 5);
	gtk_container_add(GTK_CONTAINER(song_frame), song_vbox);
	
	cmd_desc = gtk_label_new(_(
		   "Shell-command to run when Audacious starts a new song."));
	gtk_label_set_justify(GTK_LABEL(cmd_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(cmd_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(song_vbox), cmd_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(cmd_desc), TRUE);

	cmd_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(song_vbox), cmd_hbox, FALSE, FALSE, 0);

	cmd_label = gtk_label_new(_("Command:"));
	gtk_box_pack_start(GTK_BOX(cmd_hbox), cmd_label, FALSE, FALSE, 0);

	cmd_entry = gtk_entry_new();
	if (cmd_line)
		gtk_entry_set_text(GTK_ENTRY(cmd_entry), cmd_line);
	gtk_widget_set_usize(cmd_entry, 200, -1);
	gtk_box_pack_start(GTK_BOX(cmd_hbox), cmd_entry, TRUE, TRUE, 0);

	sep1 = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(song_vbox), sep1, TRUE, TRUE, 0);


	cmd_after_desc = gtk_label_new(_(
		   "Shell-command to run toward the end of a song."));
	gtk_label_set_justify(GTK_LABEL(cmd_after_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(cmd_after_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(song_vbox), cmd_after_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(cmd_after_desc), TRUE);

	cmd_after_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(song_vbox), cmd_after_hbox, FALSE, FALSE, 0);

	cmd_after_label = gtk_label_new(_("Command:"));
	gtk_box_pack_start(GTK_BOX(cmd_after_hbox), cmd_after_label, FALSE, FALSE, 0);

	cmd_after_entry = gtk_entry_new();
	if (cmd_line_after)
		gtk_entry_set_text(GTK_ENTRY(cmd_after_entry), cmd_line_after);
	gtk_widget_set_usize(cmd_after_entry, 200, -1);
	gtk_box_pack_start(GTK_BOX(cmd_after_hbox), cmd_after_entry, TRUE, TRUE, 0);
	sep2 = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(song_vbox), sep2, TRUE, TRUE, 0);


	cmd_end_desc = gtk_label_new(_(
		"Shell-command to run when Audacious reaches the end "
		"of the playlist."));
	gtk_label_set_justify(GTK_LABEL(cmd_end_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(cmd_end_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(song_vbox), cmd_end_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(cmd_end_desc), TRUE);

	cmd_end_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(song_vbox), cmd_end_hbox, FALSE, FALSE, 0);

	cmd_end_label = gtk_label_new(_("Command:"));
	gtk_box_pack_start(GTK_BOX(cmd_end_hbox), cmd_end_label, FALSE, FALSE, 0);

	cmd_end_entry = gtk_entry_new();
	if (cmd_line_end)
		gtk_entry_set_text(GTK_ENTRY(cmd_end_entry), cmd_line_end);
	gtk_widget_set_usize(cmd_end_entry, 200, -1);
	gtk_box_pack_start(GTK_BOX(cmd_end_hbox), cmd_end_entry, TRUE, TRUE, 0);
	sep3 = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(song_vbox), sep3, TRUE, TRUE, 0);

	temp = g_strdup_printf(
		_("You can use the following format strings which "
		  "will be substituted before calling the command "
		  "(not all are useful for the end-of-playlist command).\n\n"
		  "%%F: Frequency (in hertz)\n"
		  "%%c: Number of channels\n"
		  "%%f: filename (full path)\n"
		  "%%l: length (in milliseconds)\n"
		  "%%n or %%s: Song name\n"
		  "%%r: Rate (in bits per second)\n"
		  "%%t: Playlist position (%%02d)\n"
		  "%%p: Currently playing (1 or 0)"));
	f_desc = gtk_label_new(temp);
	g_free(temp);

	gtk_label_set_justify(GTK_LABEL(f_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(f_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(song_vbox), f_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(f_desc), TRUE);

	configure_bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(configure_bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(configure_bbox), 5);
	gtk_box_pack_start(GTK_BOX(configure_vbox), configure_bbox, FALSE, FALSE, 0);

	configure_ok = gtk_button_new_from_stock(_("gtk-apply"));
	gtk_signal_connect(GTK_OBJECT(configure_ok), "clicked", GTK_SIGNAL_FUNC(configure_ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(configure_ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(configure_bbox), configure_ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(configure_ok);

	gtk_widget_show_all(configure_vbox);

	return configure_vbox;
}

static void init(void)
{
	read_config();

	previous_song = -1;
	timeout_tag = gtk_timeout_add(100, timeout_func, NULL);

	configure_vbox = configure();
	prefswin_page_new(configure_vbox, "Song Change", DATA_DIR "/images/songchange.png");
}


static void bury_child(int signal)
{
	waitpid(-1, NULL, WNOHANG);
}

static void execute_command(char *cmd)
{
	char *argv[4] = {"/bin/sh", "-c", NULL, NULL};
	int i;
	argv[2] = cmd;
	signal(SIGCHLD, bury_child);
	if (fork() == 0)
	{
		/* We don't want this process to hog the audio device etc */
		for (i = 3; i < 255; i++)
			close(i);
		execv("/bin/sh", argv);
	}
}

/*
 * escape_shell_chars()
 *
 * Escapes characters that are special to the shell inside double quotes.
 */

static char* escape_shell_chars(const char *string)
{
	const char *special = "$`\"\\"; /* Characters to escape */
	const char *in = string;
	char *out;
	char *escaped;
	int num = 0;

	while (*in != '\0')
		if (strchr(special, *in++))
			num++;

	escaped = g_malloc(strlen(string) + num + 1);

	in = string;
	out = escaped;

	while (*in != '\0')
	{
		if (strchr(special, *in))
			*out++ = '\\';
		*out++ = *in++;
	}
	*out = '\0';

	return escaped;
}



/* Format codes:
 *
 *   F - frequency (in hertz)
 *   c - number of channels
 *   f - filename (full path)
 *   l - length (in milliseconds)
 *   n - name
 *   r - rate (in bits per second)
 *   s - name (since everyone's used to it)
 *   t - playlist position (%02d)
 *   p - currently playing (1 or 0)
 */
/* do_command(): do @cmd after replacing the format codes
   @cmd: command to run
   @current_file: file name of current song
   @pos: playlist_pos */
void do_command(char *cmd, const char *current_file, int pos)
{
	int length, rate, freq, nch;
	char *str, *shstring = NULL, *temp, numbuf[16];
	gboolean playing;
	Formatter *formatter;

	if (cmd && strlen(cmd) > 0)
	{
		formatter = xmms_formatter_new();
		str = xmms_remote_get_playlist_title(sc_gp.xmms_session, pos);
		if (str)
		{
			temp = escape_shell_chars(str);
			xmms_formatter_associate(formatter, 's', temp);
			xmms_formatter_associate(formatter, 'n', temp);
			g_free(str);
			g_free(temp);
		}
		else
		{
			xmms_formatter_associate(formatter, 's', "");
			xmms_formatter_associate(formatter, 'n', "");
		}

		if (current_file)
		{
			temp = escape_shell_chars(current_file);
			xmms_formatter_associate(formatter, 'f', temp);
			g_free(temp);
		}
		else
			xmms_formatter_associate(formatter, 'f', "");
		sprintf(numbuf, "%02d", pos + 1);
		xmms_formatter_associate(formatter, 't', numbuf);
		length = xmms_remote_get_playlist_time(sc_gp.xmms_session, pos);
		if (length != -1)
		{
			sprintf(numbuf, "%d", length);
			xmms_formatter_associate(formatter, 'l', numbuf);
		}
		else
			xmms_formatter_associate(formatter, 'l', "0");
		xmms_remote_get_info(sc_gp.xmms_session, &rate, &freq, &nch);
		sprintf(numbuf, "%d", rate);
		xmms_formatter_associate(formatter, 'r', numbuf);
		sprintf(numbuf, "%d", freq);
		xmms_formatter_associate(formatter, 'F', numbuf);
		sprintf(numbuf, "%d", nch);
		xmms_formatter_associate(formatter, 'c', numbuf);
		playing = xmms_remote_is_playing(sc_gp.xmms_session);
		sprintf(numbuf, "%d", playing);
		xmms_formatter_associate(formatter, 'p', numbuf);
		shstring = xmms_formatter_format(formatter, cmd);
		xmms_formatter_destroy(formatter);

		if (shstring)
		{
			execute_command(shstring);
			/* FIXME: This can possibly be freed too early */
			g_free(shstring);
		}
	}
}


static int timeout_func(gpointer data)
{
	int pos;
	gboolean playing;
	static char *previous_file = NULL;
	static char *previous_track = NULL;
	static gboolean cmd_after_already_run = FALSE;
	char *current_file;
	char *current_track;

	GDK_THREADS_ENTER();

	playing = xmms_remote_is_playing(sc_gp.xmms_session);
	pos = xmms_remote_get_playlist_pos(sc_gp.xmms_session);
	current_file = xmms_remote_get_playlist_file(sc_gp.xmms_session, pos);
	current_track = xmms_remote_get_playlist_title(sc_gp.xmms_session, pos);
	
	if ((pos != previous_song ||
	     (!previous_file && current_file) ||
	     (previous_file && !current_file) ||
	     (previous_file && current_file &&
	      strcmp(previous_file, current_file)) ||
	     (!previous_track && current_track) ||
	     (previous_track && !current_track) ||
	     (previous_track && current_track &&
	      strcmp(previous_track, current_track))) &&
	    xmms_remote_get_output_time(sc_gp.xmms_session) > 0)
	{
		do_command(cmd_line, current_file, pos);
		g_free(previous_file);
		g_free(previous_track);
		previous_file = g_strdup(current_file);
		previous_track = g_strdup(current_track);
		previous_song = pos;
		cmd_after_already_run = FALSE;
	}
	if (!cmd_after_already_run &&
	    ((xmms_remote_get_playlist_time(sc_gp.xmms_session,pos) -
	      xmms_remote_get_output_time(sc_gp.xmms_session)) < 100))
	{
		do_command(cmd_line_after, current_file, pos);
		cmd_after_already_run = TRUE;
	}

	if (playing)
	{
		int playlist_length = xmms_remote_get_playlist_length(sc_gp.xmms_session);
		if (pos + 1 == playlist_length)
			possible_pl_end = TRUE;
		else
			possible_pl_end = FALSE;
	}
	else if (possible_pl_end)
	{
		if (pos == 0)
			do_command(cmd_line_end, current_file, pos);
		possible_pl_end = FALSE;
		g_free(previous_file);
		g_free(previous_track);
		previous_file = NULL;
		previous_track = NULL;
	}

	g_free(current_file);
	current_file = NULL;

	GDK_THREADS_LEAVE();

	return TRUE;
}
