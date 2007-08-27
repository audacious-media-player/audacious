#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include "configdb.h"
#include "playback.h"
#define DEBUG 1
typedef struct {
    gchar *username,
          *session;
}last_fm;

typedef struct
{
    gchar *artist,
          *title;
}
track_data;

last_fm *login_data=NULL;
Playlist *current_playlist=NULL;
GtkWidget *lastfm_url_entry, *lastfm_label,*artist_label,*title_label,*album_label, *gui_window=NULL;

gpointer open_url(gpointer url)
{       
    gchar*s=g_new0(gchar,20);
    VFSFile* fd = vfs_fopen((gchar*)url,"r");
    if(fd)
    {        
        vfs_fgets(s,20,fd);
        g_print("Got data: '%s'\n",s);
        vfs_fclose(fd);
    }
    g_free(s);
    g_free(url);
    return NULL;
}
void command(gchar *comm)
{
    /* get the session from mowgli*/
    if(login_data->session)
    {
        g_free(login_data->session);
        login_data->session=NULL;
    }
    login_data->session = g_strdup(mowgli_global_storage_get("lastfm_session_id"));
    gchar *url=g_strdup_printf("http://ws.audioscrobbler.com/radio/control.php?session=%s&command=%s&debug=0",login_data->session,comm);
    g_thread_create(open_url,url,FALSE,NULL);
    if(!g_str_has_prefix(comm,"love")) 
        g_thread_create((gpointer)playback_initiate,NULL,FALSE,NULL);
    return;

}
static void change_track_data_cb(gpointer track, gpointer unused)
{
    gchar **inf,
          **t,
          *alb,
          *tr;
    tr=g_strdup(((track_data*)track)->title);

    if(tr==NULL)
        return ;
    if(g_strrstr(tr,"last.fm")==NULL) 
    {
        gtk_entry_set_text(GTK_ENTRY(lastfm_url_entry),"Not last.fm stream");
        return;
    }

#if DEBUG
    g_print("New Track: %s \n",tr);
#endif
    if(g_str_has_prefix(tr,"lastfm://")) 
        return; 
    if(g_strrstr(tr,"Neighbour")!=NULL) 
    {
        gchar *temp=g_strdup_printf("lastfm://user/%s/neighbours", login_data->username);
        gtk_entry_set_text(GTK_ENTRY(lastfm_url_entry),temp);
        g_free(temp);
    } 
    if(g_strrstr(tr,"Personal")!=NULL) 
    {
        gchar *temp=g_strdup_printf("lastfm://user/%s/personal", login_data->username);
        gtk_entry_set_text(GTK_ENTRY(lastfm_url_entry),temp);
        g_free(temp);
    } 

    inf = g_strsplit(tr," - ",2);
    if(inf[0]==NULL || inf[1]==NULL) 
        return;
    gchar* artist_markup=g_strdup_printf(_("<b>Artist:</b> %s") ,inf[0]);
    gtk_label_set_markup(GTK_LABEL(artist_label),artist_markup);
    g_free(artist_markup);

    t = g_strsplit(inf[1], g_strrstr(inf[1],"("),2);
    if(t[0]==NULL) return;
    alb = g_strdup(mowgli_global_storage_get("lastfm_album"));

    gchar* title_markup=g_strdup_printf(_("<b>Title:</b> %s") ,t[0]);
    gtk_label_set_markup( GTK_LABEL(title_label),title_markup);
    g_free(title_markup);

    gchar* album_markup=g_strdup_printf(_("<b>Album:</b> %s") ,alb);
    gtk_label_set_markup( GTK_LABEL(album_label),album_markup);
    g_free(album_markup);

    g_strfreev(inf);
    g_strfreev(t);
    g_free(alb);
    g_free(tr);

    return ;
}

static void show_login_error_dialog(void)
{
    const gchar *markup =
        _("<b><big>Couldn't find your lastfm login data.</big></b>\n\n"
          "Check if your Scrobbler's plugin login settings are configured properly.\n");

    GtkWidget *dialog =
        gtk_message_dialog_new_with_markup(NULL,
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           _(markup));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}



void init_last_fm(void)
{
    login_data = g_new0(last_fm,1);
    login_data->username = NULL;
    login_data->session = NULL;
    ConfigDb *cfg = NULL;

    if ((cfg = bmp_cfg_db_open()) != NULL)
    {       
        login_data->username=NULL;
        bmp_cfg_db_get_string(cfg, "audioscrobbler","username",
                              &login_data->username);
        if(login_data->username==NULL)
            show_login_error_dialog();
        g_free(cfg);
    }

    current_playlist = g_new0(Playlist,1);
    current_playlist = playlist_get_active();
    track_data * tr=g_new0(track_data,1);
    tr->title =NULL;
    hook_associate( "playlist set info" , change_track_data_cb ,tr);

}

/*event callback functions*/

gboolean love_press_callback(GtkWidget *love)
{
    command("love");
    return FALSE;
}

gboolean skip_press_callback(GtkWidget *skip)
{
    command("skip");
    return FALSE;
}
gboolean ban_press_callback(GtkWidget *ban)
{
    command("ban");
    return FALSE;
}


gboolean add_press_callback(GtkWidget *love)
{
    gchar *text=NULL;
    gint poz=0;    
    text = g_strdup(gtk_entry_get_text(GTK_ENTRY(lastfm_url_entry)));
    if(playback_get_playing()==TRUE) 
        playback_stop();

    poz = playlist_get_length(current_playlist);
    playlist_add_url(current_playlist, text);
    sleep(1);
    playlist_set_position(current_playlist, poz);
    playback_initiate();
    g_free(text);
    return FALSE;
}

gboolean neighbours_press_callback(GtkWidget *love)
{
    gchar *temp=g_strdup_printf("lastfm://user/%s/neighbours", login_data->username);
    gtk_entry_set_text(GTK_ENTRY(lastfm_url_entry),temp);
    g_free(temp);
    return FALSE;
}

gboolean personal_press_callback(GtkWidget *love)
{
    gchar *per=g_strdup_printf("lastfm://user/%s/personal", login_data->username);
    gtk_entry_set_text(GTK_ENTRY(lastfm_url_entry),per);
    g_free(per);
    return FALSE;
}

gboolean delete_window_callback(GtkWidget *window)
{
    gtk_widget_destroy(window);
    window=NULL;
    return FALSE;
}
static gboolean keypress_callback(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
    switch (event->keyval)
    {
        case GDK_Escape:  
            gtk_widget_hide_all(widget);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}


GtkWidget *ui_lastfm_create(void)
{
    GtkWidget *window;
    GtkWidget *box1,*box2,*vboxw,*vbox1,*vbox2,*v_hbox1,*v_hbox2,*labelbox;
    GtkWidget *love,*ban,*skip,*add,*neighbours,*personal;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(window), _("Audacious last.fm radio tuner"));
    lastfm_label = gtk_label_new_with_mnemonic(_("Station:"));
    title_label = gtk_label_new(NULL);
    artist_label = gtk_label_new(NULL);
    album_label = gtk_label_new(NULL);
    lastfm_url_entry = gtk_entry_new();
    gtk_editable_set_editable (GTK_EDITABLE(lastfm_url_entry),TRUE);
    gchar* artist_markup=g_strdup(_("<b>Artist:</b>"));
    gtk_label_set_markup(GTK_LABEL(artist_label),artist_markup);
    g_free(artist_markup);

    gchar* title_markup=g_strdup(_("<b>Title:</b>"));
    gtk_label_set_markup( GTK_LABEL(title_label),title_markup);
    g_free(title_markup);

    gchar* album_markup=g_strdup_printf(_("<b>Album:</b>"));
    gtk_label_set_markup( GTK_LABEL(album_label),album_markup);
    g_free(album_markup);

    love = gtk_button_new_with_label (_("Love"));
    ban = gtk_button_new_with_label (_("Ban"));
    skip = gtk_button_new_with_label (_("Skip"));
    add = gtk_button_new_with_label (_("Tune in"));

    neighbours = gtk_button_new_with_label(_("Neighbours' radio"));
    personal = gtk_button_new_with_label(_("Personal radio"));

    box1 = gtk_hbox_new(FALSE,1);
    box2 = gtk_hbox_new(FALSE,1);

    vboxw = gtk_vbox_new(FALSE,1);

    vbox1 = gtk_vbox_new(FALSE,1);
    vbox2 = gtk_vbox_new(FALSE,1);

    v_hbox1 = gtk_vbox_new(FALSE,1);
    v_hbox2 = gtk_hbox_new(FALSE,1);
    labelbox = gtk_vbox_new(FALSE,1);

    gtk_misc_set_alignment(GTK_MISC(artist_label),0,0);
    gtk_misc_set_alignment(GTK_MISC(title_label),0,0);
    gtk_misc_set_alignment(GTK_MISC(album_label),0,0);

    gtk_box_pack_start(GTK_BOX(vboxw),box1,FALSE,TRUE,1);
    gtk_box_pack_end(GTK_BOX(vboxw),box2,TRUE,TRUE,1);

    gtk_box_pack_start(GTK_BOX(box1),lastfm_label,FALSE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(box1),lastfm_url_entry,TRUE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(box1),add,FALSE,TRUE,1);

    gtk_box_set_spacing(GTK_BOX(box1),2);
    gtk_container_set_border_width(GTK_CONTAINER(box1),2);

    gtk_box_pack_start(GTK_BOX(box2),vbox1,FALSE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(box2),vbox2,TRUE,TRUE,1);

    gtk_box_pack_start(GTK_BOX(vbox1),neighbours,FALSE,TRUE,3);
    gtk_box_pack_start(GTK_BOX(vbox1),personal,FALSE,TRUE,3);
    gtk_box_set_spacing(GTK_BOX(vbox1),2);
    gtk_container_set_border_width(GTK_CONTAINER(vbox1),2);

    gtk_box_pack_start(GTK_BOX(vbox2),v_hbox1,TRUE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(vbox2),v_hbox2,TRUE,TRUE,1);

    gtk_box_pack_start(GTK_BOX(v_hbox1),artist_label,TRUE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(v_hbox1),title_label,TRUE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(v_hbox1),album_label,TRUE,TRUE,1);
    gtk_box_set_spacing(GTK_BOX(v_hbox1),2);
    gtk_container_set_border_width(GTK_CONTAINER(v_hbox1),2);


    gtk_box_pack_start(GTK_BOX(v_hbox2),love,TRUE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(v_hbox2),skip,TRUE,TRUE,1);
    gtk_box_pack_start(GTK_BOX(v_hbox2),ban,TRUE,TRUE,1);
    gtk_box_set_spacing(GTK_BOX(v_hbox1),2);
    gtk_container_set_border_width(GTK_CONTAINER(v_hbox1),2);



    gtk_container_add (GTK_CONTAINER (window), vboxw);

    g_signal_connect (G_OBJECT (love), "button_press_event",G_CALLBACK (love_press_callback), NULL);    
    g_signal_connect (G_OBJECT (add), "button_press_event",G_CALLBACK (add_press_callback), NULL); 
    g_signal_connect (G_OBJECT (ban), "button_press_event",G_CALLBACK (ban_press_callback), NULL);
    g_signal_connect (G_OBJECT (skip), "button_press_event",G_CALLBACK (skip_press_callback), NULL);
    g_signal_connect (G_OBJECT (neighbours), "button_press_event",G_CALLBACK (neighbours_press_callback), NULL);
    g_signal_connect (G_OBJECT (personal), "button_press_event",G_CALLBACK (personal_press_callback), NULL);
    g_signal_connect (G_OBJECT (window), "delete_event",G_CALLBACK (gtk_widget_hide_all), window);
    g_signal_connect (G_OBJECT (window), "key_press_event",G_CALLBACK(keypress_callback), NULL);

    gtk_widget_set_size_request(GTK_WIDGET(window),400,124);
    gtk_window_set_resizable(GTK_WINDOW(window),FALSE);
    gtk_widget_show_all(gui_window);   
    return window;
}
#if 0
static void no_lastfm_plugin_dialog(void)
{
    const gchar *markup =
        N_("<b><big>The lastfm radio plugin could not be found.</big></b>\n\n"
           "Check if the AudioScrobbler plugin was compiled in\n");

    GtkWidget *dialog =
        gtk_message_dialog_new_with_markup(NULL,
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           _(markup));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
#endif


void ui_lastfm(void)
{
    /*if(!mowgli_global_storage_get("lastfm_loaded"))
      {
      no_lastfm_plugin_dialog();    
      return;
      }
      */
    init_last_fm();
    if(!gui_window)
    {
        if((gui_window = ui_lastfm_create())!=NULL);
        gtk_widget_show_all(gui_window); 

        #if 0
        /*here should be set the artist, title and album labels at the first run, because it isn't captured by the hook*/
        gchar* current_title = playlist_get_songtitle(current_playlist,playlist_get_position(current_playlist));
        if(current_title)
            change_track_data_cb(current_title,NULL);
#endif
    }
    else
        gtk_widget_show_all(gui_window);   

}

