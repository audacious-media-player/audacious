/*
   AdPlug/XMMS - AdPlug XMMS Plugin
   Copyright (C) 2002, 2003 Simon Peter <dn.tlp@gmx.net>

   AdPlug/XMMS is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This plugin is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this plugin; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <algorithm>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include "adplug.h"
#include "emuopl.h"
#include "silentopl.h"
#include "players.h"
#include "audacious/plugin.h"
#include "libaudacious/util.h"
#include "libaudacious/configdb.h"
extern "C" {
#include "audacious/output.h"
}


/***** Defines *****/

// Version string
#define ADPLUG_NAME	"AdPlug"

// Sound buffer size in samples
#define SNDBUFSIZE	512

// AdPlug's 8 and 16 bit audio formats
#define FORMAT_8	FMT_U8
#define FORMAT_16	FMT_S16_NE

// Default file name of AdPlug's database file
#define ADPLUGDB_FILE		"adplug.db"

// Default AdPlug user's configuration subdirectory
#define ADPLUG_CONFDIR		".adplug"

/***** Global variables *****/

extern "C" InputPlugin	adplug_ip;
static gboolean		audio_error = FALSE;

// Configuration (and defaults)
static struct {
  gint		freq;
  gboolean	bit16, stereo, endless;
  CPlayers	players;
} cfg = { 44100l, true, false, false, CAdPlug::players };

// Player variables
static struct {
  CPlayer		*p;
  CAdPlugDatabase	*db;
  unsigned int		subsong, songlength;
  int			seek;
  char			filename[PATH_MAX];
  char			*songtitle;
  float			time_ms;
  bool			playing;
  GThread		*play_thread;
  GtkLabel		*infobox;
  GtkDialog		*infodlg;
} plr = { 0, 0, 0, 0, -1, "", NULL, 0.0f, false, 0, NULL, NULL };

/***** Debugging *****/

#ifdef DEBUG

#include <stdarg.h>

static void dbg_printf(const char *fmt, ...)
{
  va_list argptr;

  va_start(argptr, fmt);
  vfprintf(stderr, fmt, argptr);
  va_end(argptr);
}

#else

static void dbg_printf(const char *fmt, ...)
{ }

#endif

/***** [Dialog]: Utility functions *****/

static GtkWidget *make_framed(GtkWidget *what, const gchar *label)
{
  GtkWidget *framebox = gtk_frame_new(label);

  gtk_container_add(GTK_CONTAINER(framebox), what);
  return framebox;
}

static GtkWidget *print_left(const gchar *text)
{
  GtkLabel *label = GTK_LABEL(gtk_label_new(text));

  gtk_label_set_justify(label, GTK_JUSTIFY_LEFT);
  gtk_misc_set_padding(GTK_MISC(label), 2, 2);
  return GTK_WIDGET(label);
}

static void MessageBox(const char *title, const char *text, const char *button)
{
  char *tmptitle = (char *)malloc(strlen(title) + 1),
    *tmptxt = (char *)malloc(strlen(text) + 1),
    *tmpbutton = (char *)malloc(strlen(button) + 1);

  strcpy(tmptitle, title); strcpy(tmptxt, text); strcpy(tmpbutton, button);

  GtkWidget *msgbox = xmms_show_message(tmptitle, tmptxt, tmpbutton, FALSE,
					GTK_SIGNAL_FUNC(gtk_widget_destroyed), &msgbox);

  free(tmptitle); free(tmptxt); free(tmpbutton);
}

/***** Dialog boxes *****/

static void adplug_about(void)
{
  std::ostringstream text;

  text << ADPLUG_NAME "\n"
    "Copyright (C) 2002, 2003 Simon Peter <dn.tlp@gmx.net>\n\n"
    "This plugin is released under the terms and conditions of the GNU LGPL.\n"
    "See http://www.gnu.org/licenses/lgpl.html for details."
    "\n\nThis plugin uses the AdPlug library, which is copyright (C) Simon Peter, et al.\n"
    "Linked AdPlug library version: " << CAdPlug::get_version() << std::ends;

  MessageBox("About " ADPLUG_NAME, text.str().c_str(), "Ugh!");
}

static void close_config_box_ok(GtkButton *button, GPtrArray *rblist)
{
  // Apply configuration settings
  cfg.bit16 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ptr_array_index(rblist, 0)));
  cfg.stereo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ptr_array_index(rblist, 1)));

  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ptr_array_index(rblist, 2)))) cfg.freq = 11025;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ptr_array_index(rblist, 3)))) cfg.freq = 22050;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ptr_array_index(rblist, 4)))) cfg.freq = 44100;
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ptr_array_index(rblist, 5)))) cfg.freq = 48000;

  cfg.endless = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_ptr_array_index(rblist, 6)));

  cfg.players = *(CPlayers *)g_ptr_array_index(rblist, 7);
  delete (CPlayers *)g_ptr_array_index(rblist, 7);

  g_ptr_array_free(rblist, FALSE);
}

static void close_config_box_cancel(GtkButton *button, GPtrArray *rblist)
{
  delete (CPlayers *)g_ptr_array_index(rblist, 7);
  g_ptr_array_free(rblist, FALSE);
}

static void config_fl_row_select(GtkCList *fl, gint row, gint col,
				 GdkEventButton *event, CPlayers *pl)
{
  pl->push_back((CPlayerDesc *)gtk_clist_get_row_data(fl, row));
  pl->unique();
}

static void config_fl_row_unselect(GtkCList *fl, gint row, gint col,
				   GdkEventButton *event, CPlayers *pl)
{
  pl->remove((CPlayerDesc *)gtk_clist_get_row_data(fl, row));
}

static void adplug_config(void)
{
  GtkDialog *config_dlg = GTK_DIALOG(gtk_dialog_new());
  GtkNotebook *notebook = GTK_NOTEBOOK(gtk_notebook_new());
  GtkTable *table;
  GtkTooltips *tooltips = gtk_tooltips_new();
  GPtrArray *rblist = g_ptr_array_new();

  gtk_window_set_title(GTK_WINDOW(config_dlg), "AdPlug :: Configuration");
  gtk_window_set_policy(GTK_WINDOW(config_dlg), FALSE, FALSE, TRUE); // Window is auto sized
  gtk_window_set_modal(GTK_WINDOW(config_dlg), TRUE);
  gtk_container_add(GTK_CONTAINER(config_dlg->vbox), GTK_WIDGET(notebook));

  // Add Ok & Cancel buttons
  {
    GtkWidget *button;

    button = gtk_button_new_with_label("Ok");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(close_config_box_ok),
		       (gpointer)rblist);
    gtk_signal_connect_object_after(GTK_OBJECT(button), "clicked",
				    GTK_SIGNAL_FUNC(gtk_widget_destroy),
				    GTK_OBJECT(config_dlg));
    gtk_container_add(GTK_CONTAINER(config_dlg->action_area), button);

    button = gtk_button_new_with_label("Cancel");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       GTK_SIGNAL_FUNC(close_config_box_cancel),
		       (gpointer)rblist);
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(config_dlg));
    gtk_container_add(GTK_CONTAINER(config_dlg->action_area), button);
  }

  /***** Page 1: General *****/

  table = GTK_TABLE(gtk_table_new(1, 2, TRUE));
  gtk_table_set_row_spacings(table, 5); gtk_table_set_col_spacings(table, 5);
  gtk_notebook_append_page(notebook, GTK_WIDGET(table), print_left("General"));

  // Add "Sound quality" section
  {
    GtkTable *sqt = GTK_TABLE(gtk_table_new(2, 2, FALSE));
    GtkVBox *fvb;
    GtkRadioButton *rb;

    gtk_table_set_row_spacings(sqt, 5);
    gtk_table_set_col_spacings(sqt, 5);
    gtk_table_attach_defaults(table, make_framed(GTK_WIDGET(sqt), "Sound quality"),
			      0, 1, 0, 1);

    // Add "Resolution" section
    fvb = GTK_VBOX(gtk_vbox_new(TRUE, 0));
    gtk_table_attach_defaults(sqt, make_framed(GTK_WIDGET(fvb), "Resolution"),
			      0, 1, 0, 1);
    rb = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(NULL, "8bit"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), !cfg.bit16);
    gtk_container_add(GTK_CONTAINER(fvb), GTK_WIDGET(rb));
    rb = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(rb, "16bit"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), cfg.bit16);
    gtk_container_add(GTK_CONTAINER(fvb), GTK_WIDGET(rb));
    g_ptr_array_add(rblist, (gpointer)rb);

    // Add "Channels" section
    fvb = GTK_VBOX(gtk_vbox_new(TRUE, 0));
    gtk_table_attach_defaults(sqt, make_framed(GTK_WIDGET(fvb), "Channels"),
			      0, 1, 1, 2);
    rb = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(NULL, "Mono"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), !cfg.stereo);
    gtk_container_add(GTK_CONTAINER(fvb), GTK_WIDGET(rb));
    rb = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(rb, "Stereo"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), cfg.stereo);
    gtk_container_add(GTK_CONTAINER(fvb), GTK_WIDGET(rb));
    gtk_tooltips_set_tip(tooltips, GTK_WIDGET(rb),
			 "Setting stereo is not recommended, unless you need to. "
			 "This won't add any stereo effects to the sound - OPL2 "
			 "is just mono - but eats up more CPU power!", NULL);
    g_ptr_array_add(rblist, (gpointer)rb);

    // Add "Frequency" section
    fvb = GTK_VBOX(gtk_vbox_new(TRUE, 0));
    gtk_table_attach_defaults(sqt, make_framed(GTK_WIDGET(fvb), "Frequency"),
			      1, 2, 0, 2);
    rb = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(NULL, "11025"));
    if(cfg.freq == 11025) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), TRUE);
    gtk_container_add(GTK_CONTAINER(fvb), GTK_WIDGET(rb));
    g_ptr_array_add(rblist, (gpointer)rb);
    rb = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(rb, "22050"));
    if(cfg.freq == 22050) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), TRUE);
    gtk_container_add(GTK_CONTAINER(fvb), GTK_WIDGET(rb));
    g_ptr_array_add(rblist, (gpointer)rb);
    rb = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(rb, "44100"));
    if(cfg.freq == 44100) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), TRUE);
    gtk_container_add(GTK_CONTAINER(fvb), GTK_WIDGET(rb));
    g_ptr_array_add(rblist, (gpointer)rb);
    rb = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label_from_widget(rb, "48000"));
    if(cfg.freq == 48000) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb), TRUE);
    gtk_container_add(GTK_CONTAINER(fvb), GTK_WIDGET(rb));
    g_ptr_array_add(rblist, (gpointer)rb);
  }

  // Add "Playback" section
  {
    GtkVBox *vb = GTK_VBOX(gtk_vbox_new(FALSE, 0));
    GtkCheckButton *cb;

    gtk_table_attach_defaults(table, make_framed(GTK_WIDGET(vb), "Playback"),
			      1, 2, 0, 1);

    cb = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Detect songend"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), !cfg.endless);
    gtk_container_add(GTK_CONTAINER(vb), GTK_WIDGET(cb));
    gtk_tooltips_set_tip(tooltips, GTK_WIDGET(cb),
			 "If enabled, XMMS will detect a song's ending, stop "
			 "it and advance in the playlist. If disabled, XMMS "
			 "won't take notice of a song's ending and loop it all "
			 "over again and again.", NULL);
    g_ptr_array_add(rblist, (gpointer)cb);
  }

  /***** Page 2: Formats *****/

  table = GTK_TABLE(gtk_table_new(1, 1, TRUE));
  gtk_notebook_append_page(notebook, GTK_WIDGET(table), print_left("Formats"));

  // Add "Format selection" section
  {
    GtkHBox *vb = GTK_HBOX(gtk_hbox_new(FALSE, 0));
    gtk_table_attach_defaults(table, make_framed(GTK_WIDGET(vb), "Format selection"),
			      0, 1, 0, 1);
    // Add scrollable list
    {
      gchar *rowstr[] = {"Format", "Extension"};
      GtkEventBox *eventbox = GTK_EVENT_BOX(gtk_event_box_new());
      GtkScrolledWindow *formatswnd = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
      GtkCList *fl = GTK_CLIST(gtk_clist_new_with_titles(2, rowstr));
      CPlayers::const_iterator i;
      unsigned int j;
      gtk_clist_set_selection_mode(fl, GTK_SELECTION_MULTIPLE);

      // Build list
      for(i = CAdPlug::players.begin(); i != CAdPlug::players.end(); i++) {
	gint rownum;

	gchar *rws[2];
	rws[0] = g_strdup((*i)->filetype.c_str());
	rws[1] = g_strdup((*i)->get_extension(0));
	for(j = 1; (*i)->get_extension(j); j++)
	  rws[1] = g_strjoin(", ", rws[1], (*i)->get_extension(j), NULL);
	rownum = gtk_clist_append(fl, rws);
	g_free(rws[0]); g_free(rws[1]);
	gtk_clist_set_row_data(fl, rownum, (gpointer)(*i));
	if(find(cfg.players.begin(), cfg.players.end(), *i) != cfg.players.end())
	  gtk_clist_select_row(fl, rownum, 0);
      }

      gtk_clist_columns_autosize(fl);
      gtk_scrolled_window_set_policy(formatswnd, GTK_POLICY_AUTOMATIC,
				     GTK_POLICY_AUTOMATIC);
      gpointer pl = (gpointer)new CPlayers(cfg.players);
      gtk_signal_connect(GTK_OBJECT(fl), "select-row",
			 GTK_SIGNAL_FUNC(config_fl_row_select), pl);
      gtk_signal_connect(GTK_OBJECT(fl), "unselect-row",
			 GTK_SIGNAL_FUNC(config_fl_row_unselect), pl);
      gtk_container_add(GTK_CONTAINER(formatswnd), GTK_WIDGET(fl));
      gtk_container_add(GTK_CONTAINER(eventbox), GTK_WIDGET(formatswnd));
      gtk_container_add(GTK_CONTAINER(vb), GTK_WIDGET(eventbox));
      gtk_tooltips_set_tip(tooltips, GTK_WIDGET(eventbox),
			   "Selected file types will be recognized and played "
			   "back by this plugin. Deselected types will be "
			   "ignored to make room for other plugins to play "
			   "these files.", NULL);
      g_ptr_array_add(rblist, pl);
    }
  }

  // Show window
  gtk_widget_show_all(GTK_WIDGET(config_dlg));
}

static void add_instlist(GtkCList *instlist, const char *t1, const char *t2)
{
  gchar *rowstr[2];

  rowstr[0] = g_strdup(t1); rowstr[1] = g_strdup(t2);
  gtk_clist_append(instlist, rowstr);
  g_free(rowstr[0]); g_free(rowstr[1]);
}

static CPlayer *factory(const std::string &filename, Copl *newopl)
{
  CPlayer			*p;
  CPlayers::const_iterator	i;
  unsigned int			j;

  dbg_printf("factory(\"%s\",opl): ", filename.c_str());
  return CAdPlug::factory(filename, newopl, cfg.players);
}

static void adplug_stop(void);
static void adplug_play(char *filename);

#if 0
static void subsong_slider(GtkAdjustment *adj)
{
  adplug_stop();
  plr.subsong = (unsigned int)adj->value - 1;
  adplug_play(plr.filename);
}

static void close_infobox(GtkDialog *infodlg)
{
  // Forget our references to the instance of the "currently playing song" info
  // box. But only if we're really destroying that one... ;)
  if(infodlg == plr.infodlg) {
    plr.infobox = NULL;
    plr.infodlg = NULL;
  }
}

static void adplug_info_box(char *filename)
{
  CSilentopl tmpopl;
  CPlayer *p = (strcmp(filename, plr.filename) || !plr.p) ?
    factory(filename, &tmpopl) : plr.p;

  if(!p) return; // bail out if no player could be created
  if(p == plr.p && plr.infodlg) return; // only one info box for active song

  std::ostringstream infotext;
  unsigned int i;
  GtkDialog *infobox = GTK_DIALOG(gtk_dialog_new());
  GtkButton *okay_button = GTK_BUTTON(gtk_button_new_with_label("Ok"));
  GtkPacker *packer = GTK_PACKER(gtk_packer_new());
  GtkHBox *hbox = GTK_HBOX(gtk_hbox_new(TRUE, 2));

  // Build file info box
  gtk_window_set_title(GTK_WINDOW(infobox), "AdPlug :: File Info");
  gtk_window_set_policy(GTK_WINDOW(infobox), FALSE, FALSE, TRUE); // Window is auto sized
  gtk_container_add(GTK_CONTAINER(infobox->vbox), GTK_WIDGET(packer));
  gtk_packer_set_default_border_width(packer, 2);
  gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE);
  gtk_signal_connect_object(GTK_OBJECT(okay_button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(infobox));
  gtk_signal_connect(GTK_OBJECT(infobox), "destroy",
		     GTK_SIGNAL_FUNC(close_infobox), 0);
  gtk_container_add(GTK_CONTAINER(infobox->action_area), GTK_WIDGET(okay_button));

  // Add filename section
  gtk_packer_add_defaults(packer, make_framed(print_left(filename), "Filename"),
			  GTK_SIDE_TOP, GTK_ANCHOR_CENTER, GTK_FILL_X);

  // Add "Song info" section
  infotext << "Title: " << p->gettitle() << std::endl <<
    "Author: " << p->getauthor() << std::endl <<
    "File Type: " << p->gettype() << std::endl <<
    "Subsongs: " << p->getsubsongs() << std::endl <<
    "Instruments: " << p->getinstruments();
  if(plr.p == p)
    infotext << std::ends;
  else {
    infotext << std::endl << "Orders: " << p->getorders() << std::endl <<
      "Patterns: " << p->getpatterns() << std::ends;
  }
  gtk_container_add(GTK_CONTAINER(hbox),
		    make_framed(print_left(infotext.str().c_str()), "Song"));

  // Add "Playback info" section if currently playing
  if(plr.p == p) {
    plr.infobox = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_justify(plr.infobox, GTK_JUSTIFY_LEFT);
    gtk_misc_set_padding(GTK_MISC(plr.infobox), 2, 2);
    gtk_container_add(GTK_CONTAINER(hbox),
		      make_framed(GTK_WIDGET(plr.infobox), "Playback"));
  }
  gtk_packer_add_defaults(packer, GTK_WIDGET(hbox), GTK_SIDE_TOP,
			  GTK_ANCHOR_CENTER, GTK_FILL_X);

  // Add instrument names section
  if(p->getinstruments()) {
    GtkScrolledWindow *instwnd = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    GtkCList *instnames;
    gchar tmpstr[10];

    {
      gchar *rowstr[] = {"#","Instrument name"};
      instnames = GTK_CLIST(gtk_clist_new_with_titles(2, rowstr));
    }
    gtk_clist_set_column_justification(instnames, 0, GTK_JUSTIFY_RIGHT);

    for(i=0;i<p->getinstruments();i++) {
      sprintf(tmpstr, "%d", i + 1);
      add_instlist(instnames, tmpstr, p->getinstrument(i).c_str());
    }

    gtk_clist_columns_autosize(instnames);
    gtk_scrolled_window_set_policy(instwnd, GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(instwnd), GTK_WIDGET(instnames));
    gtk_packer_add(packer, GTK_WIDGET(instwnd), GTK_SIDE_TOP,
		   GTK_ANCHOR_CENTER, GTK_FILL_X, 0, 0, 0, 0, 50);
  }

  // Add "Song message" section
  if(!p->getdesc().empty()) {
    GtkScrolledWindow *msgwnd = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    GtkText *msg = GTK_TEXT(gtk_text_new(NULL, NULL));
    gint pos = 0;

    gtk_scrolled_window_set_policy(msgwnd, GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_text_set_editable(msg, FALSE);
    gtk_text_set_word_wrap(msg, TRUE);
    //    gtk_text_set_line_wrap(msg, TRUE);
    gtk_editable_insert_text(GTK_EDITABLE(msg), p->getdesc().c_str(),
			     p->getdesc().length(), &pos);

    gtk_container_add(GTK_CONTAINER(msgwnd), GTK_WIDGET(msg));
    gtk_packer_add(packer, make_framed(GTK_WIDGET(msgwnd), "Song message"),
		   GTK_SIDE_TOP, GTK_ANCHOR_CENTER, GTK_FILL_X, 2, 0, 0, 200, 50);
  }

  // Add subsong slider section
  if(p == plr.p && p->getsubsongs() > 1) {
    GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(plr.subsong + 1, 1,
							   p->getsubsongs() + 1,
							   1, 5, 1));
    GtkHScale *slider = GTK_HSCALE(gtk_hscale_new(adj));

    gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
		       GTK_SIGNAL_FUNC(subsong_slider), NULL);
    gtk_range_set_update_policy(GTK_RANGE(slider), GTK_UPDATE_DISCONTINUOUS);
    gtk_scale_set_digits(GTK_SCALE(slider), 0);
    gtk_packer_add_defaults(packer, make_framed(GTK_WIDGET(slider), "Subsong selection"),
			    GTK_SIDE_TOP, GTK_ANCHOR_CENTER, GTK_FILL_X);
  }

  // Show dialog box
  gtk_widget_show_all(GTK_WIDGET(infobox));
  if(p == plr.p) { // Remember widget, so we could destroy it later
    plr.infodlg = infobox;
  } else // Delete temporary player
    delete p;
}

/***** Main player (!! threaded !!) *****/

static void update_infobox(void)
{
  std::ostringstream infotext;

  // Recreate info string
  infotext << "Order: " << plr.p->getorder() << " / " << plr.p->getorders() <<
    std::endl << "Pattern: " << plr.p->getpattern() << " / " <<
    plr.p->getpatterns() << std::endl << "Row: " << plr.p->getrow() <<
    std::endl << "Speed: " << plr.p->getspeed() << std::endl << "Timer: " <<
    plr.p->getrefresh() << "Hz" << std::ends;

  GDK_THREADS_ENTER();
  gtk_label_set_text(plr.infobox, infotext.str().c_str());
  GDK_THREADS_LEAVE();
}
#endif

// Define sampsize macro (only usable inside play_loop()!)
#define sampsize ((bit16 ? 2 : 1) * (stereo ? 2 : 1))

static void *play_loop(void *filename)
/* Main playback thread. Takes the filename to play as argument. */
{
  dbg_printf("play_loop(\"%s\"): ", (char *)filename);
  CEmuopl opl(cfg.freq, cfg.bit16, cfg.stereo);
  long toadd = 0, i, towrite;
  char *sndbuf, *sndbufpos;
  bool playing = true,		// Song self-end indicator.
    bit16 = cfg.bit16,		// Duplicate config, so it doesn't affect us if
    stereo = cfg.stereo;	// the user changes it while we're playing.
  unsigned long freq = cfg.freq;

  // Try to load module
  dbg_printf("factory, ");
  if(!(plr.p = factory((char *)filename, &opl))) {
    dbg_printf("error!\n");
    MessageBox("AdPlug :: Error", "File could not be opened!", "Ok");
    plr.playing = false;
    g_thread_exit(NULL);
  }

  // Cache song length
  dbg_printf("length, ");
  plr.songlength = plr.p->songlength(plr.subsong);

  // cache song title
  dbg_printf("title, ");
  if(!plr.p->gettitle().empty()) {
    plr.songtitle = (char *)malloc(plr.p->gettitle().length() + 1);
    strcpy(plr.songtitle, plr.p->gettitle().c_str());
  }

  // reset to first subsong on new file
  dbg_printf("subsong, ");
  if(strcmp((char *)filename, plr.filename)) {
    strcpy(plr.filename, (char *)filename);
    plr.subsong = 0;
  }

  // Allocate audio buffer
  dbg_printf("buffer, ");
  sndbuf = (char *)malloc(SNDBUFSIZE * sampsize);

  // Set XMMS main window information
  dbg_printf("xmms, ");
  adplug_ip.set_info(plr.songtitle, plr.songlength, freq * sampsize * 8,
		     freq, stereo ? 2 : 1);

  // Rewind player to right subsong
  dbg_printf("rewind, ");
  plr.p->rewind(plr.subsong);

  // main playback loop
  dbg_printf("loop.\n");
  while((playing || cfg.endless) && plr.playing) {
    // seek requested ?
    if(plr.seek != -1) {
      // backward seek ?
      if(plr.seek < plr.time_ms) {
        plr.p->rewind(plr.subsong);
        plr.time_ms = 0.0f;
      }

      // seek to requested position
      while((plr.time_ms < plr.seek) && plr.p->update())
        plr.time_ms += 1000 / plr.p->getrefresh();

      // Reset output plugin and some values
      adplug_ip.output->flush((int)plr.time_ms);
      plr.seek = -1;
    }

    // fill sound buffer
    towrite = SNDBUFSIZE; sndbufpos = sndbuf;
    while (towrite > 0) {
      while (toadd < 0) {
        toadd += freq;
	playing = plr.p->update();
        plr.time_ms += 1000 / plr.p->getrefresh();
      }
      i = MIN(towrite, (long)(toadd / plr.p->getrefresh() + 4) & ~3);
      opl.update((short *)sndbufpos, i);
      sndbufpos += i * sampsize; towrite -= i;
      toadd -= (long)(plr.p->getrefresh() * i);
    }

    // write sound buffer
    while(adplug_ip.output->buffer_free() < SNDBUFSIZE * sampsize) xmms_usleep(10000);
    produce_audio(adplug_ip.output->written_time(),
			  bit16 ? FORMAT_16 : FORMAT_8,
			  stereo ? 2 : 1, SNDBUFSIZE * sampsize, sndbuf, NULL);

#if 0
    // update infobox, if necessary
    if(plr.infobox && plr.playing) update_infobox();
#endif
  }

  // playback finished - deinit
  dbg_printf("play_loop(\"%s\"): ", (char *)filename);
  if(!playing) { // wait for output plugin to finish if song has self-ended
    dbg_printf("wait, ");
    while(adplug_ip.output->buffer_playing()) xmms_usleep(10000);
  } else { // or else, flush its output buffers
    dbg_printf("flush, ");
    adplug_ip.output->buffer_free(); adplug_ip.output->buffer_free();
  }

  // free everything and exit
  dbg_printf("free");
  delete plr.p; plr.p = 0;
  if(plr.songtitle) { free(plr.songtitle); plr.songtitle = NULL; }
  free(sndbuf);
  plr.playing = false; // important! XMMS won't get a self-ended song without it.
  dbg_printf(".\n");
  g_thread_exit(NULL);
  return(NULL);
}

// sampsize macro not useful anymore.
#undef sampsize

/***** Informational *****/

static int adplug_is_our_file(char *filename)
{
  CSilentopl tmpopl;
  CPlayer *p = factory(filename,&tmpopl);

  dbg_printf("adplug_is_our_file(\"%s\"): returned ",filename);

  if(p) {
    delete p;
    dbg_printf("TRUE\n");
    return TRUE;
  }

  dbg_printf("FALSE\n");
  return FALSE;
}

static int adplug_get_time(void)
{
  if(audio_error) { dbg_printf("adplug_get_time(): returned -2\n"); return -2; }
  if(!plr.playing) { dbg_printf("adplug_get_time(): returned -1\n"); return -1; }
  return adplug_ip.output->output_time();
}

static void adplug_song_info(char *filename, char **title, int *length)
{
  CSilentopl tmpopl;
  CPlayer *p = factory(filename, &tmpopl);

  dbg_printf("adplug_song_info(\"%s\", \"%s\", %d): ", filename, *title, *length);

  if(p) {
    // allocate and set title string
    if(p->gettitle().empty())
      *title = 0;
    else {
      *title = (char *)malloc(p->gettitle().length() + 1);
      strcpy(*title, p->gettitle().c_str());
    }

    // get song length
    *length = p->songlength(plr.subsong);

    // delete temporary player object
    delete p;
  }

  dbg_printf("title = \"%s\", length = %d\n", *title, *length);
}

/***** Player control *****/

static void adplug_play(char *filename)
{
  dbg_printf("adplug_play(\"%s\"): ", filename);
  audio_error = FALSE;

#if 0
  // On new song, re-open "Song info" dialog, if open
  dbg_printf("dialog, ");
  if(plr.infobox && strcmp(filename, plr.filename))
    gtk_widget_destroy(GTK_WIDGET(plr.infodlg));
#endif

  // open output plugin
  dbg_printf("open, ");
  if (!adplug_ip.output->open_audio(cfg.bit16 ? FORMAT_16 : FORMAT_8, cfg.freq, cfg.stereo ? 2 : 1)) {
    audio_error = TRUE;
    return;
  }

  // Initialize global player data (this is here to prevent a race condition
  // between adplug_get_time() returning the playback state and adplug_loop()
  // initializing the playback state)
  dbg_printf("init, ");
  plr.playing = true; plr.time_ms = 0.0f; plr.seek = -1;

  // start player thread
  dbg_printf("create");
  plr.play_thread = g_thread_create(play_loop, filename, TRUE, NULL);
  dbg_printf(".\n");
}

static void adplug_stop(void)
{
  dbg_printf("adplug_stop(): join, ");
  plr.playing = false; g_thread_join(plr.play_thread); // stop player thread
  dbg_printf("close"); adplug_ip.output->close_audio();
  dbg_printf(".\n");
}

static void adplug_pause(short paused)
{
  dbg_printf("adplug_pause(%d)\n", paused);
  adplug_ip.output->pause(paused);
}

static void adplug_seek(int time)
{
  dbg_printf("adplug_seek(%d)\n", time);
  plr.seek = time * 1000; // time is in seconds, but we count in ms
}

/***** Configuration file handling *****/

#define CFG_VERSION "AdPlug"

static void adplug_init(void)
{
  dbg_printf("adplug_init(): open, ");
  ConfigDb *db = bmp_cfg_db_open();

  // Read configuration
  dbg_printf("read, ");
  bmp_cfg_db_get_bool(db, CFG_VERSION, "16bit", (gboolean *)&cfg.bit16);
  bmp_cfg_db_get_bool(db, CFG_VERSION, "Stereo", (gboolean *)&cfg.stereo);
  bmp_cfg_db_get_int(db, CFG_VERSION, "Frequency", (gint *)&cfg.freq);
  bmp_cfg_db_get_bool(db, CFG_VERSION, "Endless", (gboolean *)&cfg.endless);

  // Read file type exclusion list
  dbg_printf("exclusion, ");
  {
    gchar *cfgstr = "", *exclude;
    gboolean cfgread;

    cfgread = bmp_cfg_db_get_string(db, CFG_VERSION, "Exclude", &cfgstr);
    exclude = (char *)malloc(strlen(cfgstr) + 2); strcpy(exclude, cfgstr);
    exclude[strlen(exclude) + 1] = '\0';
    if(cfgread) free(cfgstr);
    g_strdelimit(exclude, ":", '\0');
    for(gchar *p = exclude; *p; p += strlen(p) + 1)
      cfg.players.remove(cfg.players.lookup_filetype(p));
    free(exclude);
  }
  bmp_cfg_db_close(db);

  // Load database from disk and hand it to AdPlug
  dbg_printf("database");
  plr.db = new CAdPlugDatabase;

  {
    const char *homedir = getenv("HOME");

    if(homedir) {
      char *userdb = (char *)malloc(strlen(homedir) + strlen(ADPLUG_CONFDIR) +
				    strlen(ADPLUGDB_FILE) + 3);
      strcpy(userdb, homedir); strcat(userdb, "/" ADPLUG_CONFDIR "/");
      strcat(userdb, ADPLUGDB_FILE);
      plr.db->load(userdb);		// load user's database
      dbg_printf(" (userdb=\"%s\")", userdb);
    }
  }
  CAdPlug::set_database(plr.db);
  dbg_printf(".\n");
}

static void adplug_quit(void)
{
  dbg_printf("adplug_quit(): open, ");
  ConfigDb *db = bmp_cfg_db_open();

  // Close database
  dbg_printf("db, ");
  if(plr.db) delete plr.db;

  // Write configuration
  dbg_printf("write, ");
  bmp_cfg_db_set_bool(db, CFG_VERSION, "16bit", cfg.bit16);
  bmp_cfg_db_set_bool(db, CFG_VERSION, "Stereo", cfg.stereo);
  bmp_cfg_db_set_int(db, CFG_VERSION, "Frequency", cfg.freq);
  bmp_cfg_db_set_bool(db, CFG_VERSION, "Endless", cfg.endless);

  dbg_printf("exclude, ");
  std::string exclude;
  for(CPlayers::const_iterator i = CAdPlug::players.begin();
      i != CAdPlug::players.end(); i++)
    if(find(cfg.players.begin(), cfg.players.end(), *i) == cfg.players.end()) {
      if(!exclude.empty()) exclude += ":";
      exclude += (*i)->filetype;
    }
  gchar *cfgval = g_strdup(exclude.c_str());
  bmp_cfg_db_set_string(db, CFG_VERSION, "Exclude", cfgval);
  free(cfgval);

  dbg_printf("close");
  bmp_cfg_db_close(db);
  dbg_printf(".\n");
}

/***** Plugin (exported) *****/

InputPlugin adplug_ip =
  {
    NULL,                       // handle (filled by XMMS)
    NULL,                       // filename (filled by XMMS)
    ADPLUG_NAME,        // plugin description
    adplug_init,                // plugin functions...
    adplug_about,
    adplug_config,
    adplug_is_our_file,
    NULL, // scan_dir (look in Input/cdaudio/cdaudio.c)
    adplug_play,
    adplug_stop,
    adplug_pause,
    adplug_seek,
    NULL, // set_eq
    adplug_get_time,
    NULL,                       // get_volume (handled by output plugin)
    NULL,                       // set_volume (...)
    adplug_quit,
    NULL,                       // OBSOLETE - DO NOT USE!
    NULL,                       // add_vis_pcm (filled by XMMS)
    NULL,                       // set_info (filled by XMMS)
    NULL,                       // set_info_text (filled by XMMS)
    adplug_song_info,
    NULL,                       // adplug_info_box was here (but it used deprecated GTK+ functions)
    NULL                        // output plugin (filled by XMMS)
  };

extern "C" InputPlugin *get_iplugin_info(void)
{
  return &adplug_ip;
}
