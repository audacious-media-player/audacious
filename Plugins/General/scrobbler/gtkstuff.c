#include "libaudacious/util.h"
#include "libaudacious/configdb.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "md5.h"

static GtkWidget 	*eduname,
			*edpwd;
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

static char *hexify(char *pass, int len)
{
	static char buf[33];
	char *bp = buf;
	char hexchars[] = "0123456789abcdef";
	int i;
	
	memset(buf, 0, sizeof(buf));
	
	for(i = 0; i < len; i++) {
		*(bp++) = hexchars[(pass[i] >> 4) & 0x0f];
		*(bp++) = hexchars[pass[i] & 0x0f];
	}
	*bp = 0;
	return buf;
}

static void saveconfig(GtkWidget *wid, gpointer data)
{
	ConfigDb *cfgfile;

	const char *pwd = gtk_entry_get_text(GTK_ENTRY(edpwd));
	const char *uid = gtk_entry_get_text(GTK_ENTRY(eduname));

	if ((cfgfile = bmp_cfg_db_open())) {
	
		md5_state_t md5state;
		unsigned char md5pword[16];
	
		bmp_cfg_db_set_string(cfgfile, "audioscrobbler", "username", (char *)uid);

		if (pwd != NULL && pwd[0] != '\0') {
			md5_init(&md5state);
			md5_append(&md5state, (unsigned const char *)pwd, strlen(pwd));
			md5_finish(&md5state, md5pword);
			bmp_cfg_db_set_string(cfgfile, "audioscrobbler", "password",
					(char *)hexify(md5pword, sizeof(md5pword)));
		}
		bmp_cfg_db_close(cfgfile);
	}
	gtk_widget_destroy(GTK_WIDGET(data));
}

void configure_dialog(void)
{
	static GtkWidget *cnfdlg;
	GtkWidget	*btnok,
			*btncancel,
			*vbox,
			*hbox,
			*unhbox,
			*pwhbox,
			*lblun,
			*lblpw,
			*frame;
			
	ConfigDb 	*cfgfile;
	
	if (cnfdlg)
		return;

	cnfdlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(cnfdlg),
			GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title(GTK_WINDOW(cnfdlg),
			"Scrobbler configuration");

	gtk_signal_connect(GTK_OBJECT(cnfdlg), "destroy",
			GTK_SIGNAL_FUNC(gtk_widget_destroyed), &cnfdlg);
	
	vbox = gtk_vbox_new(FALSE, 0);

	unhbox = gtk_hbox_new(FALSE, 0);
	eduname = gtk_entry_new();
	lblun = gtk_label_new("Username");
	gtk_box_pack_start(GTK_BOX(unhbox), lblun, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(unhbox), eduname, FALSE, FALSE, 3);
	
	pwhbox = gtk_hbox_new(FALSE, 0);
	edpwd = gtk_entry_new();
	lblpw = gtk_label_new("Password");
	gtk_entry_set_visibility(GTK_ENTRY(edpwd), FALSE);
	gtk_box_pack_start(GTK_BOX(pwhbox), lblpw, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(pwhbox), edpwd, FALSE, FALSE, 3);
	
	gtk_box_pack_start(GTK_BOX(vbox), unhbox, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), pwhbox, FALSE, FALSE, 3);
	
	hbox = gtk_hbox_new(FALSE, 0);

	btnok = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(btnok), "clicked",
			GTK_SIGNAL_FUNC(saveconfig), GTK_OBJECT(cnfdlg));
	
	btncancel = gtk_button_new_with_label("Cancel");
	gtk_signal_connect_object(GTK_OBJECT(btncancel), "clicked",
			GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(cnfdlg));
	gtk_box_pack_start(GTK_BOX(hbox), btnok, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), btncancel, FALSE, FALSE, 3);
	
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	frame = gtk_frame_new(" The plugin will have to be restarted for changes to take effect! ");
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_add(GTK_CONTAINER(cnfdlg), frame);

	if ((cfgfile = bmp_cfg_db_open())) {
		gchar *username = NULL;
		bmp_cfg_db_get_string(cfgfile, "audioscrobbler", "username",
			&username);
		if (username) {
			gtk_entry_set_text(GTK_ENTRY(eduname), username);
			g_free(username);
		}
		bmp_cfg_db_close(cfgfile);
	}
	
	gtk_widget_show_all(cnfdlg);
}
