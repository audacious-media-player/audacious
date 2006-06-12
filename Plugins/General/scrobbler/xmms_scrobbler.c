#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <audacious/plugin.h>
#include <audacious/prefswin.h>
#include <libaudacious/configdb.h>
#include <libaudacious/beepctrl.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <sys/time.h>

#include "tags/include/tags.h"
#include "scrobbler.h"
#include "gtkstuff.h"
#include "config.h"
#include "fmt.h"
#include "tags/include/unicode.h"
#include "configure.h"

#define XS_CS xmms_scrobbler.xmms_session

typedef struct submit_t
{
	int dosubmit, pos_c, len;
} submit_t;

static void init(void);
static void cleanup(void);
static void *xs_thread(void *);
static void *hs_thread(void *);
static int going;
static gint sc_id;

static GThread *pt_scrobbler;
static GMutex *m_scrobbler;
static GThread *pt_handshake;

static GeneralPlugin xmms_scrobbler =
{
	NULL,
	NULL,
	-1,
	NULL,
	init,
	about_show,
	NULL,
	cleanup
};

static void init(void)
{
	char *username = NULL, *password = NULL;
	ConfigDb *cfgfile;
	going = 1;
	GError **moo = NULL;
	GtkWidget *cfgdlg = create_cfgdlg();

        sc_id = prefswin_page_new(cfgdlg, "Last.FM", DATA_DIR "/images/audioscrobbler.png");

	if ((cfgfile = bmp_cfg_db_open()) != NULL) {
		bmp_cfg_db_get_string(cfgfile, "audioscrobbler", "username",
				&username);
		bmp_cfg_db_get_string(cfgfile, "audioscrobbler", "password",
				&password);
		bmp_cfg_db_close(cfgfile);
	}
	if ((!username || !password) || (!*username || !*password)) {
		pdebug("username/password not found - not starting",
			DEBUG);
		going = 0;
		return;
	}
	sc_init(username, password);
	g_free(username);
	g_free(password);
	m_scrobbler = g_mutex_new();
	if ((pt_scrobbler = g_thread_create(xs_thread, m_scrobbler, TRUE, moo)) == NULL) {
		pdebug(fmt_vastr("Error creating scrobbler thread: %s", moo), DEBUG);
		going = 0;
		return;
	}
	if ((pt_handshake = g_thread_create(hs_thread, m_scrobbler, TRUE, NULL)) == NULL) {
		pdebug("Error creating handshake thread", DEBUG);
		going = 0;
		return;
	}
	pdebug("plugin started", DEBUG);
}

static void cleanup(void)
{
    g_free (xmms_scrobbler.description);
    xmms_scrobbler.description = NULL;

	if (!going)
		return;
	pdebug("about to lock mutex", DEBUG);
	g_mutex_lock(m_scrobbler);
	pdebug("locked mutex", DEBUG);
	going = 0;
	g_mutex_unlock(m_scrobbler);
	pdebug("joining threads", DEBUG);
	g_thread_join(pt_scrobbler);

	g_thread_join(pt_handshake);

        prefswin_page_destroy(sc_id);

	sc_cleaner();
}

static char ishttp(const char *a)
{
	char *tmp, *bp;
	int status = 0;

	if (!a || !*a)
		return 0;

	tmp = strdup(a);
	for (bp = tmp; *bp; bp++)
		*bp = toupper(*bp);
	if (strstr(tmp, "HTTP://"))
		status = -1;
	free(tmp);
	return status;
}

/* Following code thanks to nosuke 
 *
 * It should probably be cleaned up
 */
static submit_t get_song_status(void)
{
	static int pos_c, playlistlen_c, playtime_c, time_c,
		pos_p = 0, playlistlen_p = 0, playtime_p = 0,
		playtime_i = 0, time_i = 0,
		playtime_ofs = 0;
	static char *file_c = NULL, *file_p = NULL; 

	static enum playstatus {
		ps_stop, ps_play, ps_pause
	} ps_c, ps_p = ps_stop;

	static int submitted = 0, changed, seeked, repeat,
		filechanged, rewind, len = 0;

	static enum state {
		start, stop, pause, restart, playing, pausing, stopping
	} playstate;

	submit_t dosubmit;

	struct timeval timetmp;

	/* clear dosubmit */
	dosubmit.dosubmit = dosubmit.pos_c = dosubmit.len = 0;

	/* current music number */
	pos_c = xmms_remote_get_playlist_pos(XS_CS);
	/* current file name */
	file_c = xmms_remote_get_playlist_file(XS_CS, pos_c); 
	/* total number */
	playlistlen_c = xmms_remote_get_playlist_length(XS_CS);
	/* current playtime */
	playtime_c = xmms_remote_get_output_time(XS_CS); 
	/* total length */
	len = xmms_remote_get_playlist_time(XS_CS, pos_c); 

	/* current time (ms) */
	gettimeofday(&timetmp, NULL);
	time_c = timetmp.tv_sec * 1000 + timetmp.tv_usec / 1000; 

	/* current status */
	if( xmms_remote_is_paused(XS_CS) ) {
		ps_c = ps_pause;
	}else if( xmms_remote_is_playing(XS_CS) ) {
		ps_c = ps_play;
	}else{
		ps_c = ps_stop;
	}

	/* repeat setting */
	repeat = xmms_remote_is_repeat(XS_CS);

	/*
#ifdef MAKE_XMMS
	// advance setting (required xmms-1.2.11 or over)
	advance = xmms_remote_is_advance(XS_CS);
#else
	advance = 1;
#endif
	*/

	if( ps_p == ps_stop && ps_c == ps_stop )        playstate = stopping;
	else if( ps_p == ps_stop && ps_c == ps_play )   playstate = start;
	/* else if( ps_p == ps_stop && ps_c == ps_pause ) ; */
	else if( ps_p == ps_play && ps_c == ps_play )   playstate = playing;
	else if( ps_p == ps_play && ps_c == ps_stop )   playstate = stop;
	else if( ps_p == ps_play && ps_c == ps_pause )  playstate = pause;
	else if( ps_p == ps_pause && ps_c == ps_pause ) playstate = pausing;
	else if( ps_p == ps_pause && ps_c == ps_play )  playstate = restart;
	else if( ps_p == ps_pause && ps_c == ps_stop )  playstate = stop;
	else playstate = stopping;

	/* filename has changed */
	if( !(file_p == NULL && file_c == NULL) &&
	    ((file_p == NULL && file_c != NULL) ||
		 (file_p != NULL && file_c == NULL) ||
		 (file_p != NULL && file_c != NULL && strcmp(file_c, file_p))) ){
		filechanged = 1;
		pdebug("*** filechange ***", SUB_DEBUG);
	}else{
		filechanged = 0;
	}
	if( file_c == NULL ){ len = 0; }

	/* whole rewind has occurred (maybe) */
	if( len != 0 && len - (playtime_p - playtime_c) < 3000 ){
		rewind = 1;
		pdebug("*** rewind ***", SUB_DEBUG);
	}else{
		rewind = 0;
	}


	changed = 0;
	seeked = 0;

	switch( playstate ){
	case start:
	  pdebug("*** START ***", SUB_DEBUG);
	  break;
	case stop:
	  pdebug("*** STOP ***", SUB_DEBUG);
	  len = 0;
	  break;
	case pause: 
	  pdebug("*** PAUSE ***", SUB_DEBUG);
	  playtime_ofs += playtime_c - playtime_i; /* save playtime */
	  break;
	case restart: 
	  pdebug("*** RESTART ***", SUB_DEBUG);
	  playtime_i  = playtime_c; /* restore playtime */
	  break;
	case playing:
	  if( (playtime_c < playtime_p) || /* back */
		  ( (playtime_c - playtime_i) - (time_c - time_i) > 3000 )
	          /* forward */
		  ) { 
		seeked = 1;
	  }

	  if( filechanged || /* filename has changed */
		  ( !filechanged && /* filename has not changed... */
			/* (( rewind && (repeat && (!advance ||
	                   (pos_c == 0 && playlistlen_c == 1 )))) || */
			/* looping with only one file */
			(( pos_c == 0 && playlistlen_c == 1 && repeat
				&& rewind ) || 
			 /* looping? */
			 ( pos_p == pos_c && rewind ) ||
			 
			 ( pos_p != pos_c && seeked ) ||
			 /* skip from current music to next music, 
			    which has the same filename as previous one */
			 ( pos_p < pos_c && playtime_c < playtime_p ) || 
			 /* current song has removed from playlist 
			    but the next (following) song has the same
	                    filename */
			 ( playlistlen_p > playlistlen_c
				&& playtime_c < playtime_p )))){
		pdebug("*** CHANGE ***",SUB_DEBUG);
		pdebug(fmt_vastr(" filechanged = %d",filechanged),SUB_DEBUG);
		pdebug(fmt_vastr(" pos_c = %d",pos_c),SUB_DEBUG);
		pdebug(fmt_vastr(" pos_p = %d",pos_p),SUB_DEBUG);
		pdebug(fmt_vastr(" rewind = %d", rewind),SUB_DEBUG);
		pdebug(fmt_vastr(" seeked = %d", seeked),SUB_DEBUG);
		pdebug(fmt_vastr(" playtime_c = %d", playtime_c),SUB_DEBUG);
		pdebug(fmt_vastr(" playtime_p = %d", playtime_p),SUB_DEBUG);
		pdebug(fmt_vastr(" playlistlen_c = %d", playlistlen_p),
			SUB_DEBUG);
		pdebug(fmt_vastr(" playlistlen_p = %d", playlistlen_p),
			SUB_DEBUG);
		changed = 1;
		seeked = 0;

		if (file_p != NULL)
		{
			g_free(file_p);
			file_p = NULL;
		}
	  }else if( seeked ) { 
		seeked = 1;
		pdebug("*** SEEK ***", SUB_DEBUG);
	  }

	  break;
	case pausing: 
	  if(playtime_c != playtime_p){
		pdebug("*** SEEK ***", SUB_DEBUG);
		seeked = 1;
	  }
	  break;
	case stopping:
	  len = 0;
	  break;
	default:
	  pdebug("*** unknown state tranfer!!! ***", SUB_DEBUG);
	  break;
	}

	
	if( playstate == start || changed || (seeked && !submitted) ){
	  /* reset counter */
	  pdebug(" <<< reset counter >>>", SUB_DEBUG);

	  submitted = 0;
	  playtime_ofs = 0;
	  playtime_i = playtime_c;
	  time_i = time_c;

	}else{
	  /* check playtime for submitting */
	  if( !submitted ){
		if( len > 30 * 1000 &&
			/* len < 30 *60 * 1000 &&  // crazy rule!!! */
			( 
			 (playtime_ofs + playtime_c - playtime_i > len / 2) ||
			 (playtime_ofs + playtime_c - playtime_i > 240 * 1000) 
			 /* (playtime_c - playtime_i > 10 * 1000)// for debug */
			 )){
		  pdebug("*** submitting requirements are satisfied.",
			SUB_DEBUG);
		  pdebug(fmt_vastr("    len = %d, playtime = %d",
			len / 1000, (playtime_c - playtime_i)/1000 ),
			SUB_DEBUG);
		  submitted = 1;
		  dosubmit.dosubmit = 1;
	          dosubmit.pos_c = pos_c;
	          dosubmit.len = len;
		}
	  }
	}

	g_free(file_p);

	/* keep current value for next iteration */
	ps_p = ps_c;
	file_p = file_c;
	playtime_p = playtime_c;
	pos_p = pos_c;
	playlistlen_p = playlistlen_c;

	return dosubmit;
}

static void *xs_thread(void *data __attribute__((unused)))
{
	int run = 1, i;
	char *charpos, *dirname;
	gboolean direxists;
	submit_t dosubmit;
	
	while (run) {
		/* Error catching */
		if(sc_catch_error())
		{
			errorbox_show(sc_fetch_error());
			sc_clear_error();
		}

		/* Check for ability to submit */
		dosubmit = get_song_status();

		if(dosubmit.dosubmit) {
			char *fname, /**title, *artist,*/ *tmp = NULL; /**sep*/
			int track = 0;
			metatag_t *meta;

			pdebug("Submitting song.", DEBUG);
			
			meta = metatag_new();

			fname = xmms_remote_get_playlist_file(0,dosubmit.pos_c);
			if (ishttp(fname)) {
				g_free(fname);
				continue;
			}
			charpos = strrchr(fname, '.');
			if(charpos != NULL &&
				!fmt_strncasecmp(charpos + 1, "cda", 3))
			{
				ConfigDb *cfgfile;

				if ((cfgfile = bmp_cfg_db_open())
					!= NULL)
				{
					char *direntry = calloc(32, 1);

					dirname = fname;
					tmp = strrchr(fname, '.');
					*tmp = '\0';
					track = (char)atoi(tmp - 2);
					pdebug(fmt_vastr("Track: %d", track),
						DEBUG);
					tmp = strrchr(dirname, '/');
					*(tmp + 1) = '\0';
					direxists = bmp_cfg_db_get_string(
						cfgfile, "CDDA",
						"directory", &fname);
					for(i = 0; direxists == TRUE
						&& strcmp(dirname, fname) == 0;)
					{
						i++;
						snprintf(direntry, 31,
							"directory%d", i);
						g_free(fname);
						direxists =
							bmp_cfg_db_get_string(
							  cfgfile, "CDDA",
							  direntry, &fname);
					}
					if(i > 0)
					{
						snprintf(direntry, 31,
							"device%d", i);
					}
					else
					{
						snprintf(direntry, 31,
							"device");
					}
					g_free(fname);
					bmp_cfg_db_get_string(cfgfile, "CDDA",
						direntry, &fname);
					bmp_cfg_db_close(cfgfile);
					free(direntry);
					pdebug(fmt_vastr("CD Device: %s",
						fname), DEBUG);
				}
			}

			pdebug(fmt_vastr("get_tag_data, %s", fname), DEBUG);
			get_tag_data(meta, fname, track);

			if(meta->artist != NULL && meta->title != NULL)
			{
				pdebug(fmt_vastr(
					"submitting artist: %s, title: %s",
					meta->artist, meta->title), DEBUG);
				sc_addentry(m_scrobbler, meta,
					dosubmit.len/1000);
			}
			else
				pdebug("couldn't determine artist - "
						"title, not submitting",
						DEBUG);
			/* g_free(tmp); */
			g_free(fname);
			metatag_delete(meta);
		}
		g_mutex_lock(m_scrobbler);
		run = going;
		g_mutex_unlock(m_scrobbler);
		usleep(100000);
	}
	pdebug("scrobbler thread: exiting", DEBUG);
	g_thread_exit(NULL);

	return NULL;
}

static void *hs_thread(void *data __attribute__((unused)))
{
	int run = 1;
	
	while(run)
	{
		if(sc_idle(m_scrobbler))
		{
			pdebug("Giving up due to fatal error", DEBUG);
			g_mutex_lock(m_scrobbler);
			going = 0;
			g_mutex_lock(m_scrobbler);
		}
		g_mutex_lock(m_scrobbler);
		run = going;
		g_mutex_unlock(m_scrobbler);
		sleep(1);
	}
	pdebug("handshake thread: exiting", DEBUG);
	g_thread_exit(NULL);

	return NULL;
}

GeneralPlugin *get_gplugin_info(void)
{
	xmms_scrobbler.description = g_strdup_printf(_("Scrobbler Plugin"));
	return &xmms_scrobbler;
}
