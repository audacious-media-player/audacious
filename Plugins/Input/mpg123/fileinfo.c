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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "mpg123.h"

#ifdef HAVE_ID3LIB
# include <id3.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "audacious/util.h"
#include <libaudacious/util.h>
#include <libaudacious/vfs.h>
#include <libaudacious/xentry.h>

#include "mp3.xpm"

static GtkWidget *window = NULL;
static GtkWidget *filename_entry, *id3_frame;
static GtkWidget *title_entry, *artist_entry, *album_entry, *year_entry,
    *tracknum_entry, *comment_entry;
static GtkWidget *genre_combo;
#ifdef HAVE_ID3LIB
static GtkWidget * totaltracks_entry;
#endif
static GtkWidget *mpeg_level, *mpeg_bitrate, *mpeg_samplerate, *mpeg_flags,
    *mpeg_error, *mpeg_copy, *mpeg_orig, *mpeg_emph, *mpeg_frames,
    *mpeg_filesize;
static GtkWidget *mpeg_level_val, *mpeg_bitrate_val, *mpeg_samplerate_val,
    *mpeg_error_val, *mpeg_copy_val, *mpeg_orig_val, *mpeg_emph_val,
    *mpeg_frames_val, *mpeg_filesize_val;

GtkWidget *vbox, *hbox, *left_vbox, *table;
GtkWidget *mpeg_frame, *mpeg_box;
GtkWidget *label, *filename_vbox;
GtkWidget *bbox;
GtkWidget *remove_id3, *cancel, *save;
GtkWidget *boxx;
#if 0
GtkWidget *revert;
#endif

VFSFile *fh;
struct id3v1tag_t tag;
const gchar *emphasis[4];
const gchar *bool_label[2];


static GList *genre_list = NULL;
static gchar *current_filename = NULL;

extern gchar *mpg123_filename;
extern gint mpg123_bitrate, mpg123_frequency, mpg123_layer, mpg123_lsf,
    mpg123_mode;
extern gboolean mpg123_stereo, mpg123_mpeg25;

glong info_rate;

void fill_entries(GtkWidget * w, gpointer data);

#define MAX_STR_LEN 100

#ifndef HAVE_ID3LIB

static void
set_entry_tag(GtkEntry * entry, gchar * tag, gint length)
{
    gint stripped_len;
    gchar *text, *text_utf8;

    stripped_len = mpg123_strip_spaces(tag, length);
    text = g_strdup_printf("%-*.*s", stripped_len, stripped_len, tag);

    if ((text_utf8 = str_to_utf8(text))) {
        gtk_entry_set_text(entry, text_utf8);
        g_free(text_utf8);
    }
    else {
        gtk_entry_set_text(entry, "");
    }

    g_free(text);
}

static void
get_entry_tag(GtkEntry * entry, gchar * tag, gint length)
{
    gchar *text = str_to_utf8(gtk_entry_get_text(entry));
    memset(tag, ' ', length);
    memcpy(tag, text, strlen(text) > length ? length : strlen(text));
}

static gint
find_genre_id(const gchar * text)
{
    gint i;

    for (i = 0; i < GENRE_MAX; i++) {
        if (!strcmp(mpg123_id3_genres[i], text))
            return i;
    }
    if (text[0] == '\0')
        return 0xff;
    return 0;
}

static void
press_save(GtkWidget * w, gpointer data)
{
    gtk_button_clicked(GTK_BUTTON(save));
}

#else

GtkWidget * copy_album_tags_but, * paste_album_tags_but;

struct album_tags_t {
  char * performer;
  char * album;
  char * year;
  char * total_tracks;
};

struct album_tags_t album_tags = { NULL, NULL, NULL, NULL };

#define FREE_AND_ZERO(x) do { g_free(x); x = NULL; } while (0)

static void free_album_tags()
{
  FREE_AND_ZERO(album_tags.performer);
  FREE_AND_ZERO(album_tags.album);
  FREE_AND_ZERO(album_tags.year);
  FREE_AND_ZERO(album_tags.total_tracks);
}

static inline char * entry_text_dup_or_null(GtkWidget * e)
{
  const char * text = gtk_entry_get_text(GTK_ENTRY(e));
  if (strlen(text) > 0)
    return g_strdup(text);
  else
    return NULL;
}

static inline void 
update_paste_sensitive()
{
  gtk_widget_set_sensitive(GTK_WIDGET(paste_album_tags_but), 
			   album_tags.performer ||
			   album_tags.album ||
			   album_tags.year ||
			   album_tags.total_tracks);

}

static void validate_zeropad_tracknums()
{
  const char * tn_str, * tt_str, * end;
  char buf[5];
  int tn, tt;

  tn_str = gtk_entry_get_text(GTK_ENTRY(tracknum_entry));
  tt_str = gtk_entry_get_text(GTK_ENTRY(totaltracks_entry));

  end = tt_str;
  tt = strtol(tt_str,(char**)&end,10);
  if (end != tt_str) {
    sprintf(buf,"%02d",tt);
    gtk_entry_set_text(GTK_ENTRY(totaltracks_entry),buf);
  } else {
    gtk_entry_set_text(GTK_ENTRY(totaltracks_entry),"");
    tt = 1000; /* any tracknum is valid */
  }

  end = tn_str;
  tn = strtol(tn_str,(char**)&end,10);
  if (end != tn_str && tn <= tt) {
    sprintf(buf,"%02d",tn);
    gtk_entry_set_text(GTK_ENTRY(tracknum_entry),buf);
  } else
    gtk_entry_set_text(GTK_ENTRY(tracknum_entry),"");

}

static void 
copy_album_tags()
{
  validate_zeropad_tracknums();
  free_album_tags();
  album_tags.performer = entry_text_dup_or_null(artist_entry);
  album_tags.album = entry_text_dup_or_null(album_entry);
  album_tags.year = entry_text_dup_or_null(year_entry);
  album_tags.total_tracks = entry_text_dup_or_null(totaltracks_entry);
  update_paste_sensitive();
}

static void 
paste_album_tags()
{
  if (album_tags.performer)
    gtk_entry_set_text(GTK_ENTRY(artist_entry),album_tags.performer);
  if (album_tags.album)
    gtk_entry_set_text(GTK_ENTRY(album_entry),album_tags.album);
  if (album_tags.year)
    gtk_entry_set_text(GTK_ENTRY(year_entry),album_tags.year);
  if (album_tags.total_tracks)
    gtk_entry_set_text(GTK_ENTRY(totaltracks_entry),album_tags.total_tracks);
}

#endif

static gint
genre_comp_func(gconstpointer a, gconstpointer b)
{
    return strcasecmp(a, b);
}

static gboolean
fileinfo_keypress_cb(GtkWidget * widget,
                     GdkEventKey * event,
                     gpointer data)
{
    if (!event)
        return FALSE;

    switch (event->keyval) {
    case GDK_Escape:
        gtk_widget_destroy(window);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

#ifdef HAVE_ID3LIB
/* some helper id3(v2) functions */

static void str_to_id3v2_frame(const char * str, ID3Tag * tag, ID3_FrameID frame_id)
{
  ID3Frame * frame = ID3Tag_FindFrameWithID(tag,frame_id);
  ID3Field * text_field;
  gboolean new_frame = frame?FALSE:TRUE;

  if (new_frame) {
    frame = ID3Frame_NewID(frame_id);
  }

  text_field = ID3Frame_GetField(frame,ID3FN_TEXT);
  ID3Field_SetASCII(text_field, str);

  if (new_frame) 
    ID3Tag_AddFrame(tag,frame);
}

static void genre_combo_to_tag(GtkWidget * combo, ID3Tag * tag)
{
  int idx = -1, i;
  const char * genre = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
  for(i=0;i<ID3_NR_OF_V1_GENRES;i++)
    if (!strcmp(genre,ID3_v1_genre_description[i])) {
      idx = i; break;
    }
  if (idx>-1) {
    char code[7];
    snprintf(code,7,"(%d)",idx);
    str_to_id3v2_frame(code,tag,ID3FID_CONTENTTYPE);
  }  
}

static void id3v2_frame_to_entry(GtkWidget * entry,ID3Tag * tag, ID3_FrameID frame_id)
{
  ID3Frame * frame = ID3Tag_FindFrameWithID(tag,frame_id);
  ID3Field * text_field;
  if (frame) {
    char buf[4096];
    text_field = ID3Frame_GetField(frame,ID3FN_TEXT);
    ID3Field_GetASCII(text_field,buf,4096);
    gtk_entry_set_text(GTK_ENTRY(entry),buf);
  } else
    gtk_entry_set_text(GTK_ENTRY(entry),"");    
}

static void id3v2_frame_to_text_view(GtkWidget * entry,ID3Tag * tag, ID3_FrameID frame_id)
{
  ID3Frame * frame = ID3Tag_FindFrameWithID(tag,frame_id);
  ID3Field * text_field;
  if (frame) {
    char buf[4096];
    text_field = ID3Frame_GetField(frame,ID3FN_TEXT);
    ID3Field_GetASCII(text_field,buf,4096);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(entry)),buf,-1);
  } else
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(entry)),"",-1);
}

static void id3v2_tracknum_to_entries(GtkWidget * tracknum_entry,
				      GtkWidget * totaltracks_entry,
				      ID3Tag * tag)
{
  ID3Frame * frame = ID3Tag_FindFrameWithID(tag,ID3FID_TRACKNUM);
  ID3Field * text_field;
  if (frame) {
    char buf[4096];
    char * slash;
    text_field = ID3Frame_GetField(frame,ID3FN_TEXT);
    ID3Field_GetASCII(text_field,buf,4096);
    slash = strchr(buf,'/');
    if (slash) {
      slash[0] = 0;
      gtk_entry_set_text(GTK_ENTRY(tracknum_entry),buf);
      gtk_entry_set_text(GTK_ENTRY(totaltracks_entry),slash+1);
    } else {
      gtk_entry_set_text(GTK_ENTRY(tracknum_entry),buf);
      gtk_entry_set_text(GTK_ENTRY(totaltracks_entry),"");
    }
  } else {
    gtk_entry_set_text(GTK_ENTRY(tracknum_entry),"");    
    gtk_entry_set_text(GTK_ENTRY(totaltracks_entry),"");    
  }
}

/* 
   if has v2 - link with v2, if not - attempt to link with v1 
   use this only for reading - always save v2 
*/
size_t ID3Tag_LinkPreferV2(ID3Tag *tag, const char *fileName)
{
  size_t r;

  r = ID3Tag_Link(tag,fileName);
  if (ID3Tag_HasTagType(tag,ID3TT_ID3V2)) {
    ID3Tag_Clear(tag);
    r = ID3Tag_LinkWithFlags(tag,fileName,ID3TT_ID3V2);
  }
  return r;
}

#endif /* HAVE_ID3LIB */

#ifdef HAVE_ID3LIB

static void
save_cb(GtkWidget * w, gpointer data)
{
  ID3Tag * id3tag;
  const char * tracks_str, * trackno_str, * endptr;
  int trackno, tracks; 

  if (str_has_prefix_nocase(current_filename, "http://"))
    return;

  validate_zeropad_tracknums();
  
  id3tag = ID3Tag_New();
  ID3Tag_LinkWithFlags(id3tag, current_filename, ID3TT_ID3);

  str_to_id3v2_frame(gtk_entry_get_text(GTK_ENTRY(title_entry)),id3tag,ID3FID_TITLE);
  str_to_id3v2_frame(gtk_entry_get_text(GTK_ENTRY(artist_entry)),id3tag,ID3FID_LEADARTIST);
  str_to_id3v2_frame(gtk_entry_get_text(GTK_ENTRY(album_entry)),id3tag,ID3FID_ALBUM);
  {
    GtkTextIter start, end;
    GtkTextBuffer * buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(comment_entry));
    gtk_text_buffer_get_start_iter(buffer,&start);
    gtk_text_buffer_get_end_iter(buffer,&end);
    str_to_id3v2_frame(gtk_text_buffer_get_text(buffer,&start,&end,FALSE),id3tag,ID3FID_COMMENT);
  }
  str_to_id3v2_frame(gtk_entry_get_text(GTK_ENTRY(year_entry)),id3tag,ID3FID_YEAR);

  /* saving trackno -> may be with album tracks number */
  trackno_str = gtk_entry_get_text(GTK_ENTRY(tracknum_entry));
  endptr = trackno_str;
  trackno = strtol(trackno_str,(char**)&endptr,10);
  if (endptr != trackno_str) {
    char buf[10];
    tracks_str = gtk_entry_get_text(GTK_ENTRY(totaltracks_entry));
    endptr = tracks_str;
    tracks = strtol(tracks_str,(char**)&endptr,10);
    if (endptr != tracks_str) 
      snprintf(buf,10,"%02d/%02d",trackno,tracks);
    else
      snprintf(buf,10,"%02d",trackno);
    str_to_id3v2_frame(buf,id3tag,ID3FID_TRACKNUM);
  } else 
    str_to_id3v2_frame("",id3tag,ID3FID_TRACKNUM);
  

  genre_combo_to_tag(genre_combo,id3tag);
  gtk_widget_set_sensitive(GTK_WIDGET(w), FALSE);

  ID3Tag_Update(id3tag);

  ID3Tag_Delete(id3tag);
}

#else /* ! HAVE_ID3LIB */

static void
save_cb(GtkWidget * widget,
        gpointer data)
{
    VFSFile *file;
    gchar *msg = NULL;

    if (str_has_prefix_nocase(current_filename, "http://"))
        return;

    if ((file = vfs_fopen(current_filename, "r+b")) != NULL) {
        gint tracknum;

        vfs_fseek(file, -128, SEEK_END);
        vfs_fread(&tag, 1, sizeof(struct id3v1tag_t), file);

        if (g_str_has_prefix(tag.tag, "TAG"))
            vfs_fseek(file, -128L, SEEK_END);
        else
            vfs_fseek(file, 0L, SEEK_END);

        tag.tag[0] = 'T';
        tag.tag[1] = 'A';
        tag.tag[2] = 'G';

        get_entry_tag(GTK_ENTRY(title_entry), tag.title, 30);
        get_entry_tag(GTK_ENTRY(artist_entry), tag.artist, 30);
        get_entry_tag(GTK_ENTRY(album_entry), tag.album, 30);
        get_entry_tag(GTK_ENTRY(year_entry), tag.year, 4);

        tracknum = atoi(gtk_entry_get_text(GTK_ENTRY(tracknum_entry)));
        if (tracknum > 0) {
            get_entry_tag(GTK_ENTRY(comment_entry), tag.u.v1_1.comment, 28);
            tag.u.v1_1.__zero = 0;
            tag.u.v1_1.track_number = MIN(tracknum, 255);
        }
        else
            get_entry_tag(GTK_ENTRY(comment_entry), tag.u.v1_0.comment, 30);

        tag.genre = find_genre_id(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO
                                                      (genre_combo)->entry)));
        if (vfs_fwrite(&tag, 1, sizeof(tag), file) != sizeof(tag))
            msg = g_strdup_printf(_("%s\nUnable to write to file: %s"),
                                  _("Couldn't write tag!"), strerror(errno));
        vfs_fclose(file);
    }
    else
        msg = g_strdup_printf(_("%s\nUnable to open file: %s"),
                              _("Couldn't write tag!"), strerror(errno));
    if (msg) {
        GtkWidget *mwin = xmms_show_message(_("File Info"), msg, _("Ok"),
                                            FALSE, NULL, NULL);
        gtk_window_set_transient_for(GTK_WINDOW(mwin), GTK_WINDOW(window));
        g_free(msg);
    }
    else {
        gtk_widget_set_sensitive(GTK_WIDGET(data), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
    }
}

#endif /* HAVE_ID3LIB */

static void
label_set_text(GtkWidget * label, gchar * str, ...)
{
    va_list args;
    gchar tempstr[MAX_STR_LEN];

    va_start(args, str);
    g_vsnprintf(tempstr, MAX_STR_LEN, str, args);
    va_end(args);

    gtk_label_set_text(GTK_LABEL(label), tempstr);
}

#ifdef HAVE_ID3LIB

static void
remove_id3_cb(GtkWidget * w, gpointer data)
{
  ID3Tag * id3tag;

  if (str_has_prefix_nocase(current_filename, "http://"))
    return;
  
  id3tag = ID3Tag_New();
  ID3Tag_LinkWithFlags(id3tag, current_filename, ID3TT_ID3);

  ID3Tag_Strip(id3tag,ID3TT_ALL);
  ID3Tag_Update(id3tag);

  ID3Tag_Delete(id3tag);
  gtk_entry_set_text(GTK_ENTRY(title_entry), "");
  gtk_entry_set_text(GTK_ENTRY(artist_entry), "");
  gtk_entry_set_text(GTK_ENTRY(album_entry), "");
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(comment_entry)), "",-1);
  gtk_entry_set_text(GTK_ENTRY(year_entry), "");
  gtk_entry_set_text(GTK_ENTRY(album_entry), "");
  gtk_entry_set_text(GTK_ENTRY(tracknum_entry), "");
  gtk_entry_set_text(GTK_ENTRY(totaltracks_entry), "");
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(genre_combo)->entry), "");
  gtk_widget_set_sensitive(GTK_WIDGET(w), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(data), FALSE);
}

#else

static void
remove_id3_cb(GtkWidget * w, gpointer data)
{
    VFSFile *file;
    gint len;
    struct id3v1tag_t tag;
    gchar *msg = NULL;

    if (str_has_prefix_nocase(current_filename, "http://"))
        return;

    if ((file = vfs_fopen(current_filename, "rb+")) != NULL) {
        vfs_fseek(file, -128, SEEK_END);
        len = vfs_ftell(file);

        vfs_fread(&tag, 1, sizeof(struct id3v1tag_t), file);

        if (g_str_has_prefix(tag.tag, "TAG")) {
            if (vfs_truncate(file, len))
                msg = g_strdup_printf(_("%s\n"
                                        "Unable to truncate file: %s"),
                                      _("Couldn't remove tag!"),
                                      strerror(errno));
        }
        else
            msg = strdup(_("No tag to remove!"));

        vfs_fclose(file);
    }
    else
        msg = g_strdup_printf(_("%s\nUnable to open file: %s"),
                              _("Couldn't remove tag!"), strerror(errno));
    if (msg) {
        GtkWidget *mwin = xmms_show_message(_("File Info"), msg, _("Ok"),
                                            FALSE, NULL, NULL);
        gtk_window_set_transient_for(GTK_WINDOW(mwin), GTK_WINDOW(window));
        g_free(msg);
    }
    else {
        gtk_entry_set_text(GTK_ENTRY(title_entry), "");
        gtk_entry_set_text(GTK_ENTRY(artist_entry), "");
        gtk_entry_set_text(GTK_ENTRY(album_entry), "");
        gtk_entry_set_text(GTK_ENTRY(comment_entry), "");
        gtk_entry_set_text(GTK_ENTRY(year_entry), "");
        gtk_entry_set_text(GTK_ENTRY(album_entry), "");
        gtk_entry_set_text(GTK_ENTRY(tracknum_entry), "");
        gtk_widget_set_sensitive(GTK_WIDGET(w), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data), FALSE);
    }
}

#endif

static void
set_mpeg_level_label(gboolean mpeg25, gint lsf, gint layer)
{
    if (mpeg25)
        label_set_text(mpeg_level_val, "MPEG-2.5 Layer %d", layer);
    else
        label_set_text(mpeg_level_val, "MPEG-%d Layer %d", lsf + 1, layer);
}

static const gchar *
channel_mode_name(gint mode)
{
    static const gchar *channel_mode[] = { N_("Stereo"), N_("Joint stereo"),
        N_("Dual channel"), N_("Single channel")
    };
    if (mode < 0 || mode > 3)
        return "";

    return gettext(channel_mode[mode]);
}

static void
file_info_http(gchar * filename)
{
    gtk_widget_set_sensitive(id3_frame, FALSE);
    if (mpg123_filename && !strcmp(filename, mpg123_filename) &&
        mpg123_bitrate != 0) {
        set_mpeg_level_label(mpg123_mpeg25, mpg123_lsf, mpg123_layer);
        label_set_text(mpeg_bitrate_val, _("%d KBit/s"), mpg123_bitrate);
        label_set_text(mpeg_samplerate_val, _("%ld Hz"), mpg123_frequency);
        label_set_text(mpeg_flags, "%s", channel_mode_name(mpg123_mode));
    }
}

static void
change_buttons(GtkObject * object)
{
    gtk_widget_set_sensitive(GTK_WIDGET(object), TRUE);
#if 0
    gtk_widget_set_sensitive(GTK_WIDGET(revert),TRUE);
#endif
}

void
mpg123_file_info_box(gchar * filename)
{
    gint i;
    gchar *title, *filename_utf8;

    emphasis[0] = _("None");
    emphasis[1] = _("50/15 ms");
    emphasis[2] = "";
    emphasis[3] = _("CCIT J.17");
    bool_label[0] = _("No");
    bool_label[1] = _("Yes");

    if (!window) {
        GtkWidget *pixmapwid;
        GdkPixbuf *pixbuf;
        PangoAttrList *attrs;
        PangoAttribute *attr;
        GtkWidget *test_table = gtk_table_new(2, 10, FALSE);
        GtkWidget *urk, *blark;
#ifdef HAVE_ID3LIB
	GtkWidget * tracknum_box, * comment_frame;
#endif

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_type_hint(GTK_WINDOW(window),
                                 GDK_WINDOW_TYPE_HINT_DIALOG);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        g_signal_connect(G_OBJECT(window), "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &window);
        gtk_container_set_border_width(GTK_CONTAINER(window), 10);

        vbox = gtk_vbox_new(FALSE, 10);
        gtk_container_add(GTK_CONTAINER(window), vbox);


        filename_vbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), filename_vbox, FALSE, TRUE, 0);

        pixbuf = gdk_pixbuf_new_from_xpm_data((const gchar **)
                                              gnome_mime_audio_xpm);
        pixmapwid = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
        gtk_misc_set_alignment(GTK_MISC(pixmapwid), 0, 0);
        gtk_box_pack_start(GTK_BOX(filename_vbox), pixmapwid, FALSE, FALSE,
                           0);

        label = gtk_label_new(NULL);

        attrs = pango_attr_list_new();

        attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        attr->start_index = 0;
        attr->end_index = -1;
        pango_attr_list_insert(attrs, attr);

        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_label_set_text(GTK_LABEL(label), _("Name:"));
        gtk_box_pack_start(GTK_BOX(filename_vbox), label, FALSE, FALSE, 0);

        filename_entry = gtk_entry_new();
        gtk_editable_set_editable(GTK_EDITABLE(filename_entry), FALSE);
        gtk_box_pack_start(GTK_BOX(filename_vbox), filename_entry, TRUE,
                           TRUE, 0);

        hbox = gtk_hbox_new(FALSE, 10);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

        left_vbox = gtk_table_new(2, 4, FALSE);
        gtk_box_pack_start(GTK_BOX(hbox), left_vbox, FALSE, FALSE, 0);

        /* MPEG-Info window */

        mpeg_frame = gtk_frame_new(_(" MPEG Info "));
        gtk_table_attach(GTK_TABLE(left_vbox), mpeg_frame, 0, 2, 0, 1,
                         GTK_FILL, GTK_FILL, 0, 4);

        mpeg_box = gtk_vbox_new(FALSE, 5);
        gtk_container_add(GTK_CONTAINER(mpeg_frame), mpeg_box);
        gtk_container_set_border_width(GTK_CONTAINER(mpeg_box), 10);
        gtk_box_set_spacing(GTK_BOX(mpeg_box), 0);

        /* MPEG Layer Info */

        /* FIXME: Obvious... */
        gtk_container_set_border_width(GTK_CONTAINER(test_table), 0);
        gtk_container_add(GTK_CONTAINER(mpeg_box), test_table);

        mpeg_level = gtk_label_new(_("MPEG Level:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_level), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_level), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_level), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_level, 0, 1, 0, 1,
                         GTK_FILL, GTK_FILL, 5, 2);

        mpeg_level_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_level_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_level_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_level_val, 1, 2, 0, 1,
                         GTK_FILL, GTK_FILL, 10, 2);

        mpeg_bitrate = gtk_label_new(_("Bit rate:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_bitrate), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_bitrate), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_bitrate), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_bitrate, 0, 1, 1, 2,
                         GTK_FILL, GTK_FILL, 5, 2);

        mpeg_bitrate_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_bitrate_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_bitrate_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_bitrate_val, 1, 2, 1,
                         2, GTK_FILL, GTK_FILL, 10, 2);

        mpeg_samplerate = gtk_label_new(_("Sample rate:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_samplerate), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_samplerate), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_samplerate), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_samplerate, 0, 1, 2,
                         3, GTK_FILL, GTK_FILL, 5, 2);

        mpeg_samplerate_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_samplerate_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_samplerate_val),
                              GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_samplerate_val, 1, 2,
                         2, 3, GTK_FILL, GTK_FILL, 10, 2);

        mpeg_frames = gtk_label_new(_("Frames:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_frames), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_frames), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_frames), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_frames, 0, 1, 3, 4,
                         GTK_FILL, GTK_FILL, 5, 2);

        mpeg_frames_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_frames_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_frames_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_frames_val, 1, 2, 3,
                         4, GTK_FILL, GTK_FILL, 10, 2);

        mpeg_filesize = gtk_label_new(_("File size:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_filesize), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_filesize), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_filesize), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_filesize, 0, 1, 4, 5,
                         GTK_FILL, GTK_FILL, 5, 2);

        mpeg_filesize_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_filesize_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_filesize_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_filesize_val, 1, 2, 4,
                         5, GTK_FILL, GTK_FILL, 10, 2);

        urk = gtk_label_new("");
        blark = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(urk), 1, 0.5);
        gtk_misc_set_alignment(GTK_MISC(blark), 0, 0);

        gtk_table_attach(GTK_TABLE(test_table), urk, 0, 1, 5, 6, GTK_FILL,
                         GTK_FILL, 5, 5);
        gtk_table_attach(GTK_TABLE(test_table), blark, 1, 2, 5, 6,
                         GTK_FILL, GTK_FILL, 10, 5);

        mpeg_error = gtk_label_new(_("Error Protection:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_error), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_error), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_error), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_error, 0, 1, 6, 7,
                         GTK_FILL, GTK_FILL, 5, 0);

        mpeg_error_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_error_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_error_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_error_val, 1, 2, 6, 7,
                         GTK_FILL, GTK_FILL, 10, 2);

        mpeg_copy = gtk_label_new(_("Copyright:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_copy), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_copy), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_copy), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_copy, 0, 1, 7, 8,
                         GTK_FILL, GTK_FILL, 5, 2);

        mpeg_copy_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_copy_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_copy_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_copy_val, 1, 2, 7, 8,
                         GTK_FILL, GTK_FILL, 10, 2);

        mpeg_orig = gtk_label_new(_("Original:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_orig), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_orig), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_orig), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_orig, 0, 1, 8, 9,
                         GTK_FILL, GTK_FILL, 5, 2);

        mpeg_orig_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_orig_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_orig_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_orig_val, 1, 2, 8, 9,
                         GTK_FILL, GTK_FILL, 10, 2);

        mpeg_emph = gtk_label_new(_("Emphasis:"));
        gtk_misc_set_alignment(GTK_MISC(mpeg_emph), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(mpeg_emph), GTK_JUSTIFY_RIGHT);
        gtk_label_set_attributes(GTK_LABEL(mpeg_emph), attrs);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_emph, 0, 1, 9, 10,
                         GTK_FILL, GTK_FILL, 5, 2);

        mpeg_emph_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(mpeg_emph_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(mpeg_emph_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), mpeg_emph_val, 1, 2, 9, 10,
                         GTK_FILL, GTK_FILL, 10, 2);


        id3_frame = gtk_frame_new(_(" ID3 Tag "));
        gtk_table_attach(GTK_TABLE(left_vbox), id3_frame, 2, 4, 0, 1,
                         GTK_FILL, GTK_FILL, 0, 4);

        table = gtk_table_new(7, 5, FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(table), 5);
        gtk_container_add(GTK_CONTAINER(id3_frame), table);

        label = gtk_label_new(_("Title:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL,
                         GTK_FILL, 5, 5);

#ifdef HAVE_ID3LIB
	title_entry = gtk_entry_new();
#else
        title_entry = gtk_entry_new_with_max_length(30);
#endif
        gtk_table_attach(GTK_TABLE(table), title_entry, 1, 6, 0, 1,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Artist:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL,
                         GTK_FILL, 5, 5);

#ifdef HAVE_ID3LIB
	artist_entry = gtk_entry_new();
#else
        artist_entry = gtk_entry_new_with_max_length(30);
#endif
        gtk_table_attach(GTK_TABLE(table), artist_entry, 1, 6, 1, 2,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Album:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL,
                         GTK_FILL, 5, 5);

#ifdef HAVE_ID3LIB
	album_entry = gtk_entry_new();
#else
        album_entry = gtk_entry_new_with_max_length(30);
#endif
        gtk_table_attach(GTK_TABLE(table), album_entry, 1, 6, 2, 3,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Comment:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL,
                         GTK_FILL, 5, 5);

#ifdef HAVE_ID3LIB
	comment_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(comment_frame),GTK_SHADOW_IN);
	comment_entry = gtk_text_view_new();
	gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(comment_entry),FALSE);
	gtk_container_add(GTK_CONTAINER(comment_frame),comment_entry);
        gtk_table_attach(GTK_TABLE(table), comment_frame, 1, 6, 3, 4,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);
#else
        comment_entry = gtk_entry_new_with_max_length(30);
        gtk_table_attach(GTK_TABLE(table), comment_entry, 1, 6, 3, 4,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);
#endif

        label = gtk_label_new(_("Year:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL,
                         GTK_FILL, 5, 5);

        year_entry = gtk_entry_new_with_max_length(4);
        gtk_entry_set_width_chars(GTK_ENTRY(year_entry),4);
        gtk_table_attach(GTK_TABLE(table), year_entry, 1, 2, 4, 5,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Track number:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 2, 3, 4, 5, GTK_FILL,
                         GTK_FILL, 5, 5);

#ifdef HAVE_ID3LIB
	tracknum_box = gtk_hbox_new(FALSE,0);
	tracknum_entry = gtk_entry_new_with_max_length(2);
        gtk_entry_set_width_chars(GTK_ENTRY(tracknum_entry),2);
	totaltracks_entry = gtk_entry_new_with_max_length(2);
        gtk_entry_set_width_chars(GTK_ENTRY(totaltracks_entry),2);
	gtk_box_pack_start(GTK_BOX(tracknum_box),
			   tracknum_entry, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(tracknum_box),
			   gtk_label_new(" / "), FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(tracknum_box),
			   totaltracks_entry, TRUE, TRUE, 1);
        gtk_table_attach(GTK_TABLE(table), 
			 tracknum_box,
			 3, 4, 4, 5,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);
	
#else
        tracknum_entry = gtk_entry_new_with_max_length(3);
        gtk_widget_set_usize(tracknum_entry, 40, -1);
        gtk_table_attach(GTK_TABLE(table), tracknum_entry, 3, 4, 4, 5,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);
#endif

        label = gtk_label_new(_("Genre:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, GTK_FILL,
                         GTK_FILL, 5, 5);

        pango_attr_list_unref(attrs);

        genre_combo = gtk_combo_new();
        gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(genre_combo)->entry),
                               FALSE);
        if (!genre_list) {
            for (i = 0; i < GENRE_MAX; i++)
                genre_list =
                    g_list_prepend(genre_list,
                                   (gchar *) mpg123_id3_genres[i]);
            genre_list = g_list_prepend(genre_list, "");
            genre_list = g_list_sort(genre_list, genre_comp_func);
        }
        gtk_combo_set_popdown_strings(GTK_COMBO(genre_combo), genre_list);

        gtk_table_attach(GTK_TABLE(table), genre_combo, 1, 6, 5, 6,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        boxx = gtk_hbutton_box_new();
        gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);

        remove_id3 = gtk_button_new_from_stock(GTK_STOCK_DELETE);
        gtk_container_add(GTK_CONTAINER(boxx), remove_id3);

#if 0
        revert = gtk_button_new_from_stock(GTK_STOCK_REVERT_TO_SAVED);
        gtk_container_add(GTK_CONTAINER(boxx), revert);
#endif

#ifdef HAVE_ID3LIB
	copy_album_tags_but = gtk_button_new_with_label(_("Copy album tags"));
	paste_album_tags_but = gtk_button_new_with_label(_("Paste album tags"));

        gtk_container_add(GTK_CONTAINER(boxx), copy_album_tags_but);
        gtk_container_add(GTK_CONTAINER(boxx), paste_album_tags_but);

        g_signal_connect(G_OBJECT(copy_album_tags_but), "clicked",
                         G_CALLBACK(copy_album_tags), NULL);
        g_signal_connect(G_OBJECT(paste_album_tags_but), "clicked",
                         G_CALLBACK(paste_album_tags), NULL);

	gtk_widget_set_sensitive(GTK_WIDGET(paste_album_tags_but), FALSE);
#endif
        save = gtk_button_new_from_stock(GTK_STOCK_SAVE);
        gtk_container_add(GTK_CONTAINER(boxx), save);

        g_signal_connect(G_OBJECT(remove_id3), "clicked",
                         G_CALLBACK(remove_id3_cb), save);
        g_signal_connect(G_OBJECT(save), "clicked", G_CALLBACK(save_cb),
                         remove_id3);
#if 0
        g_signal_connect(G_OBJECT(revert), "clicked", G_CALLBACK(fill_entries),
                         NULL);
#endif


        gtk_table_attach(GTK_TABLE(table), boxx, 0, 5, 6, 7, GTK_FILL, 0,
                         0, 8);

        bbox = gtk_hbutton_box_new();
        gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
        gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
        gtk_table_attach(GTK_TABLE(left_vbox), bbox, 0, 4, 1, 2, GTK_FILL,
                         0, 0, 8);

        cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
        g_signal_connect_swapped(G_OBJECT(cancel), "clicked",
                                 G_CALLBACK(gtk_widget_destroy),
                                 G_OBJECT(window));
        GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
        gtk_box_pack_start(GTK_BOX(bbox), cancel, FALSE, FALSE, 0);
        gtk_widget_grab_default(cancel);


        gtk_table_set_col_spacing(GTK_TABLE(left_vbox), 1, 10);


        g_signal_connect_swapped(G_OBJECT(title_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(artist_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(album_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(year_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
#ifdef HAVE_ID3LIB
        g_signal_connect_swapped(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(comment_entry))), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(totaltracks_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
#else
        g_signal_connect_swapped(G_OBJECT(comment_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
#endif
        g_signal_connect_swapped(G_OBJECT(tracknum_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(GTK_COMBO(genre_combo)->entry), "changed",
                                 G_CALLBACK(change_buttons), save);

	/* Nonsence, should i remove this altogether? 
	   causes changes to be saved as you type - 
	   makes save /revert buttons pointless
        g_signal_connect(G_OBJECT(title_entry), "activate",
                         G_CALLBACK(press_save), NULL);
        g_signal_connect(G_OBJECT(artist_entry), "activate",
                         G_CALLBACK(press_save), NULL);
        g_signal_connect(G_OBJECT(album_entry), "activate",
                         G_CALLBACK(press_save), NULL);
        g_signal_connect(G_OBJECT(year_entry), "activate",
                         G_CALLBACK(press_save), NULL);
        g_signal_connect(G_OBJECT(comment_entry), "activate",
                         G_CALLBACK(press_save), NULL);
        g_signal_connect(G_OBJECT(tracknum_entry), "activate",
                         G_CALLBACK(press_save), NULL);
	*/
        g_signal_connect(G_OBJECT(window), "key_press_event",
                         G_CALLBACK(fileinfo_keypress_cb), NULL);
    }

    g_free(current_filename);
    current_filename = g_strdup(filename);

    filename_utf8 = filename_to_utf8(filename);

    title = g_strdup_printf(_("%s - Audacious"), g_basename(filename_utf8));
    gtk_window_set_title(GTK_WINDOW(window), title);
    g_free(title);

    gtk_entry_set_text(GTK_ENTRY(filename_entry), filename_utf8);
    g_free(filename_utf8);

    gtk_editable_set_position(GTK_EDITABLE(filename_entry), -1);

    gtk_entry_set_text(GTK_ENTRY(artist_entry), "");
    gtk_entry_set_text(GTK_ENTRY(album_entry), "");
    gtk_entry_set_text(GTK_ENTRY(year_entry), "");
    gtk_entry_set_text(GTK_ENTRY(tracknum_entry), "");
#ifdef HAVE_ID3LIB
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(comment_entry)),"",-1);
#else
    gtk_entry_set_text(GTK_ENTRY(comment_entry), "");
#endif
    gtk_list_select_item(GTK_LIST(GTK_COMBO(genre_combo)->list),
                         g_list_index(genre_list, ""));

    gtk_label_set_text(GTK_LABEL(mpeg_level), _("MPEG Level:"));
    gtk_label_set_text(GTK_LABEL(mpeg_level_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(mpeg_bitrate), _("Bit rate:"));
    gtk_label_set_text(GTK_LABEL(mpeg_bitrate_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(mpeg_samplerate), _("Sample rate:"));
    gtk_label_set_text(GTK_LABEL(mpeg_samplerate_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(mpeg_error), _("Error Protection:"));
    gtk_label_set_text(GTK_LABEL(mpeg_error_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(mpeg_copy), _("Copyright:"));
    gtk_label_set_text(GTK_LABEL(mpeg_copy_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(mpeg_orig), _("Original:"));
    gtk_label_set_text(GTK_LABEL(mpeg_orig_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(mpeg_emph), _("Emphasis:"));
    gtk_label_set_text(GTK_LABEL(mpeg_emph_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(mpeg_frames), _("Frames:"));
    gtk_label_set_text(GTK_LABEL(mpeg_frames_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(mpeg_filesize), _("File size:"));
    gtk_label_set_text(GTK_LABEL(mpeg_filesize_val), _("N/A"));

    if (str_has_prefix_nocase(filename, "http://")) {
        file_info_http(filename);
        return;
    }

    gtk_widget_set_sensitive(id3_frame,
                             vfs_is_writeable(filename));

    fill_entries(NULL, NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(save), FALSE);
#if 0
    gtk_widget_set_sensitive(GTK_WIDGET(revert), FALSE);
#endif
    gtk_widget_show_all(window);
}

#ifdef HAVE_ID3LIB

void
fill_entries(GtkWidget * w, gpointer data)
{
  VFSFile *fh;
  ID3Tag * id3tag;

  if (str_has_prefix_nocase(current_filename, "http://"))
    return;
  
  id3tag = ID3Tag_New();
  ID3Tag_LinkPreferV2(id3tag, current_filename);

  id3v2_frame_to_entry(title_entry, id3tag, ID3FID_TITLE);
  id3v2_frame_to_entry(artist_entry, id3tag, ID3FID_LEADARTIST);
  id3v2_frame_to_entry(album_entry, id3tag, ID3FID_ALBUM);
  id3v2_frame_to_text_view(comment_entry, id3tag, ID3FID_COMMENT);
  id3v2_frame_to_entry(year_entry, id3tag, ID3FID_YEAR);
  id3v2_tracknum_to_entries(tracknum_entry, totaltracks_entry, id3tag);
  {
    ID3Frame * frame = ID3Tag_FindFrameWithID(id3tag, ID3FID_CONTENTTYPE);
	    
    if (frame) {
      int genre_idx = -1;
      char genre[64];
      const char * genre2;
      ID3Field * text_field = ID3Frame_GetField(frame,ID3FN_TEXT);
      ID3Field_GetASCII(text_field,genre,64);

      /* attempt to find corresponding genre */
      g_strstrip(genre);
      sscanf(genre,"(%d)",&genre_idx);
      if ((genre2 = ID3_V1GENRE2DESCRIPTION(genre_idx)))
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(genre_combo)->entry),
			   genre2);
    }
  }

  gtk_widget_set_sensitive(GTK_WIDGET(remove_id3), TRUE);
  gtk_widget_set_sensitive(GTK_WIDGET(save), FALSE);

  update_paste_sensitive();

  ID3Tag_Delete(id3tag);  

  if ((fh = vfs_fopen(current_filename, "rb")) != NULL) {
    guint32 head;
    guchar tmp[4];
    struct frame frm;
    gboolean id3_found = FALSE;

    if (vfs_fread(tmp, 1, 4, fh) != 4) {
      vfs_fclose(fh);
      return;
    }
    head =
      ((guint32) tmp[0] << 24) | ((guint32) tmp[1] << 16) |
      ((guint32) tmp[2] << 8) | (guint32) tmp[3];
    while (!mpg123_head_check(head)) {
      head <<= 8;
      if (vfs_fread(tmp, 1, 1, fh) != 1) {
	vfs_fclose(fh);
	return;
      }
      head |= tmp[0];
    }
    if (mpg123_decode_header(&frm, head)) {
      guchar *buf;
      gdouble tpf;
      gint pos;
      xing_header_t xing_header;
      guint32 num_frames;

      buf = g_malloc(frm.framesize + 4);
      vfs_fseek(fh, -4, SEEK_CUR);
      vfs_fread(buf, 1, frm.framesize + 4, fh);
      tpf = mpg123_compute_tpf(&frm);
      set_mpeg_level_label(frm.mpeg25, frm.lsf, frm.lay);
      pos = vfs_ftell(fh);
      vfs_fseek(fh, 0, SEEK_END);
      if (mpg123_get_xing_header(&xing_header, buf)) {
	num_frames = xing_header.frames;
	label_set_text(mpeg_bitrate_val,
		       _("Variable,\navg. bitrate: %d KBit/s"),
		       (gint) ((xing_header.bytes * 8) /
			       (tpf * xing_header.frames * 1000)));
      }
      else {
	num_frames =
	  ((vfs_ftell(fh) - pos -
	    (id3_found ? 128 : 0)) / mpg123_compute_bpf(&frm)) + 1;
	label_set_text(mpeg_bitrate_val, _("%d KBit/s"),
		       tabsel_123[frm.lsf][frm.lay -
					   1][frm.bitrate_index]);
      }
      label_set_text(mpeg_samplerate_val, _("%ld Hz"),
		     mpg123_freqs[frm.sampling_frequency]);
      label_set_text(mpeg_error_val, _("%s"),
		     bool_label[frm.error_protection]);
      label_set_text(mpeg_copy_val, _("%s"), bool_label[frm.copyright]);
      label_set_text(mpeg_orig_val, _("%s"), bool_label[frm.original]);
      label_set_text(mpeg_emph_val, _("%s"), emphasis[frm.emphasis]);
      label_set_text(mpeg_frames_val, _("%d"), num_frames);
      label_set_text(mpeg_filesize_val, _("%lu Bytes"), vfs_ftell(fh));
      g_free(buf);
    }
    vfs_fclose(fh);
  }

}

#else /* ! HAVE_ID3LIB */

void
fill_entries(GtkWidget * w, gpointer data)
{
    if ((fh = vfs_fopen(current_filename, "rb")) != NULL) {
        guint32 head;
        guchar tmp[4];
        struct frame frm;
        gboolean id3_found = FALSE;

        vfs_fseek(fh, -sizeof(tag), SEEK_END);
        if (vfs_fread(&tag, 1, sizeof(tag), fh) == sizeof(tag)) {
            if (!strncmp(tag.tag, "TAG", 3)) {
                id3_found = TRUE;
                set_entry_tag(GTK_ENTRY(title_entry), tag.title, 30);
                set_entry_tag(GTK_ENTRY(artist_entry), tag.artist, 30);
                set_entry_tag(GTK_ENTRY(album_entry), tag.album, 30);
                set_entry_tag(GTK_ENTRY(year_entry), tag.year, 4);
                /* Check for v1.1 tags */
                if (tag.u.v1_1.__zero == 0) {
                    gchar *temp =
                        g_strdup_printf("%d", tag.u.v1_1.track_number);
                    set_entry_tag(GTK_ENTRY(comment_entry),
                                  tag.u.v1_1.comment, 28);
                    gtk_entry_set_text(GTK_ENTRY(tracknum_entry), temp);
                    g_free(temp);
                }
                else {
                    set_entry_tag(GTK_ENTRY(comment_entry),
                                  tag.u.v1_0.comment, 30);
                    gtk_entry_set_text(GTK_ENTRY(tracknum_entry), "");
                }

                gtk_list_select_item(GTK_LIST
                                     (GTK_COMBO(genre_combo)->list),
                                     g_list_index(genre_list, (gchar *)
                                                  mpg123_id3_genres[tag.
                                                                    genre]));
                gtk_widget_set_sensitive(GTK_WIDGET(remove_id3), TRUE);
                gtk_widget_set_sensitive(GTK_WIDGET(save), FALSE);
#if 0
                gtk_widget_set_sensitive(GTK_WIDGET(revert), FALSE);
#endif
            }
            else {
                gtk_entry_set_text(GTK_ENTRY(title_entry), "");
                gtk_entry_set_text(GTK_ENTRY(artist_entry), "");
                gtk_entry_set_text(GTK_ENTRY(album_entry), "");
                gtk_entry_set_text(GTK_ENTRY(comment_entry), "");
                gtk_entry_set_text(GTK_ENTRY(year_entry), "");
                gtk_entry_set_text(GTK_ENTRY(album_entry), "");
                gtk_entry_set_text(GTK_ENTRY(tracknum_entry), "");
                gtk_widget_set_sensitive(GTK_WIDGET(remove_id3), FALSE);
                gtk_widget_set_sensitive(GTK_WIDGET(save), FALSE);
#if 0
                gtk_widget_set_sensitive(GTK_WIDGET(revert), FALSE);
#endif
            }
        }
        vfs_rewind(fh);
        if (vfs_fread(tmp, 1, 4, fh) != 4) {
            vfs_fclose(fh);
            return;
        }
        head =
            ((guint32) tmp[0] << 24) | ((guint32) tmp[1] << 16) |
            ((guint32) tmp[2] << 8) | (guint32) tmp[3];
        while (!mpg123_head_check(head)) {
            head <<= 8;
            if (vfs_fread(tmp, 1, 1, fh) != 1) {
                vfs_fclose(fh);
                return;
            }
            head |= tmp[0];
        }
        if (mpg123_decode_header(&frm, head)) {
            guchar *buf;
            gdouble tpf;
            gint pos;
            xing_header_t xing_header;
            guint32 num_frames;

            buf = g_malloc(frm.framesize + 4);
            vfs_fseek(fh, -4, SEEK_CUR);
            vfs_fread(buf, 1, frm.framesize + 4, fh);
            tpf = mpg123_compute_tpf(&frm);
            set_mpeg_level_label(frm.mpeg25, frm.lsf, frm.lay);
            pos = vfs_ftell(fh);
            vfs_fseek(fh, 0, SEEK_END);
            if (mpg123_get_xing_header(&xing_header, buf)) {
                num_frames = xing_header.frames;
                label_set_text(mpeg_bitrate_val,
                               _("Variable,\navg. bitrate: %d KBit/s"),
                               (gint) ((xing_header.bytes * 8) /
                                       (tpf * xing_header.frames * 1000)));
            }
            else {
                num_frames =
                    ((vfs_ftell(fh) - pos -
                      (id3_found ? 128 : 0)) / mpg123_compute_bpf(&frm)) + 1;
                label_set_text(mpeg_bitrate_val, _("%d KBit/s"),
                               tabsel_123[frm.lsf][frm.lay -
                                                   1][frm.bitrate_index]);
            }
            label_set_text(mpeg_samplerate_val, _("%ld Hz"),
                           mpg123_freqs[frm.sampling_frequency]);
            label_set_text(mpeg_error_val, _("%s"),
                           bool_label[frm.error_protection]);
            label_set_text(mpeg_copy_val, _("%s"), bool_label[frm.copyright]);
            label_set_text(mpeg_orig_val, _("%s"), bool_label[frm.original]);
            label_set_text(mpeg_emph_val, _("%s"), emphasis[frm.emphasis]);
            label_set_text(mpeg_frames_val, _("%d"), num_frames);
            label_set_text(mpeg_filesize_val, _("%lu Bytes"), vfs_ftell(fh));
            g_free(buf);
        }
        vfs_fclose(fh);
    }
}

#endif
