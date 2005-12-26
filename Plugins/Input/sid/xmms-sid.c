/*
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   Main source file

   Programmed and designed by Matti 'ccr' Hamalainen <ccr@tnsp.org>
   (C) Copyright 1999-2005 Tecnic Software productions (TNSP)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "xmms-sid.h"
#include "xs_support.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <stdarg.h>

#include <audacious/plugin.h>
#include <libaudacious/util.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "xs_config.h"
#include "xs_length.h"
#include "xs_stil.h"
#include "xs_filter.h"
#include "xs_fileinfo.h"
#include "xs_interface.h"
#include "xs_glade.h"

/*
 * Include player engines
 */
#ifdef HAVE_SIDPLAY1
#include "xs_sidplay1.h"
#endif
#ifdef HAVE_SIDPLAY2
#include "xs_sidplay2.h"
#endif


/*
 * List of players and links to their functions
 */
t_xs_player xs_playerlist[] = {
#ifdef HAVE_SIDPLAY1
	{XS_ENG_SIDPLAY1,
	 xs_sidplay1_isourfile,
	 xs_sidplay1_init, xs_sidplay1_close,
	 xs_sidplay1_initsong, xs_sidplay1_fillbuffer,
	 xs_sidplay1_loadsid, xs_sidplay1_deletesid,
	 xs_sidplay1_getsidinfo
	},
#endif
#ifdef HAVE_SIDPLAY2
	{XS_ENG_SIDPLAY2,
	 xs_sidplay2_isourfile,
	 xs_sidplay2_init, xs_sidplay2_close,
	 xs_sidplay2_initsong, xs_sidplay2_fillbuffer,
	 xs_sidplay2_loadsid, xs_sidplay2_deletesid,
	 xs_sidplay2_getsidinfo
	},
#endif
};

const gint xs_nplayerlist = (sizeof(xs_playerlist) / sizeof(t_xs_player));


/*
 * Global variables
 */
t_xs_status xs_status;
extern GStaticMutex xs_status_mutex;
extern GStaticMutex xs_cfg_mutex;
GStaticMutex xs_subctrl_mutex = G_STATIC_MUTEX_INIT;

static GThread *xs_decode_thread;

static GtkWidget *xs_subctrl = NULL;
static GtkObject *xs_subctrl_adj = NULL;

void xs_subctrl_close(void);
void xs_subctrl_update(void);


/*
 * Error messages
 */
void XSERR(const char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "XMMS-SID: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

#ifndef DEBUG_NP
void XSDEBUG(const char *fmt, ...)
{
#ifdef DEBUG
	va_list ap;
	fprintf(stderr, "XSDEBUG: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
#endif
}
#endif

/*
 * Initialization functions
 */
void xs_reinit(void)
{
	gint iPlayer;
	gboolean isInitialized;

	/* Stop playing, if we are */
	g_static_mutex_lock(&xs_status_mutex);
	if (xs_status.isPlaying) {
		g_static_mutex_unlock(&xs_status_mutex);
		xs_stop();
	} else {
		g_static_mutex_unlock(&xs_status_mutex);
	}

	/* Initialize status and sanitize configuration */
	xs_memset(&xs_status, 0, sizeof(xs_status));

	if (xs_cfg.audioFrequency < 8000)
		xs_cfg.audioFrequency = 8000;

	if (xs_cfg.oversampleFactor < XS_MIN_OVERSAMPLE)
		xs_cfg.oversampleFactor = XS_MIN_OVERSAMPLE;
	else if (xs_cfg.oversampleFactor > XS_MAX_OVERSAMPLE)
		xs_cfg.oversampleFactor = XS_MAX_OVERSAMPLE;

	if (xs_cfg.audioChannels != XS_CHN_MONO)
		xs_cfg.oversampleEnable = FALSE;

	xs_status.audioFrequency = xs_cfg.audioFrequency;
	xs_status.audioBitsPerSample = xs_cfg.audioBitsPerSample;
	xs_status.audioChannels = xs_cfg.audioChannels;
	xs_status.audioFormat = -1;
	xs_status.oversampleEnable = xs_cfg.oversampleEnable;
	xs_status.oversampleFactor = xs_cfg.oversampleFactor;

	/* Try to initialize emulator engine */
	XSDEBUG("initializing emulator engine #%i...\n", xs_cfg.playerEngine);

	iPlayer = 0;
	isInitialized = FALSE;
	while ((iPlayer < xs_nplayerlist) && !isInitialized) {
		if (xs_playerlist[iPlayer].plrIdent == xs_cfg.playerEngine) {
			if (xs_playerlist[iPlayer].plrInit(&xs_status)) {
				isInitialized = TRUE;
				xs_status.sidPlayer = (t_xs_player *) & xs_playerlist[iPlayer];
			}
		}
		iPlayer++;
	}

	XSDEBUG("init#1: %s, %i\n", (isInitialized) ? "OK" : "FAILED", iPlayer);

	iPlayer = 0;
	while ((iPlayer < xs_nplayerlist) && !isInitialized) {
		if (xs_playerlist[iPlayer].plrInit(&xs_status)) {
			isInitialized = TRUE;
			xs_status.sidPlayer = (t_xs_player *) & xs_playerlist[iPlayer];
			xs_cfg.playerEngine = xs_playerlist[iPlayer].plrIdent;
		} else
			iPlayer++;
	}

	XSDEBUG("init#2: %s, %i\n", (isInitialized) ? "OK" : "FAILED", iPlayer);

	/* Get settings back, in case the chosen emulator backend changed them */
	xs_cfg.audioFrequency = xs_status.audioFrequency;
	xs_cfg.audioBitsPerSample = xs_status.audioBitsPerSample;
	xs_cfg.audioChannels = xs_status.audioChannels;
	xs_cfg.oversampleEnable = xs_status.oversampleEnable;

	/* Initialize song-length database */
	xs_songlen_close();
	if (xs_cfg.songlenDBEnable && (xs_songlen_init() != 0)) {
		XSERR("Error initializing song-length database!\n");
	}

	/* Initialize STIL database */
	xs_stil_close();
	if (xs_cfg.stilDBEnable && (xs_stil_init() != 0)) {
		XSERR("Error initializing STIL database!\n");
	}
}


/*
 * Initialize XMMS-SID
 */
void xs_init(void)
{
	XSDEBUG("xs_init()\n");

	/* Initialize and get configuration */
	xs_memset(&xs_cfg, 0, sizeof(xs_cfg));
	xs_init_configuration();
	xs_read_configuration();

	/* Initialize subsystems */
	xs_reinit();

	XSDEBUG("OK\n");
}


/*
 * Shut down XMMS-SID
 */
void xs_close(void)
{
	XSDEBUG("xs_close(): shutting down...\n");

	/* Stop playing, free structures */
	xs_stop();

	xs_tuneinfo_free(xs_status.tuneInfo);
	xs_status.tuneInfo = NULL;
	xs_status.sidPlayer->plrDeleteSID(&xs_status);
	xs_status.sidPlayer->plrClose(&xs_status);

	xs_songlen_close();
	xs_stil_close();

	XSDEBUG("shutdown finished.\n");
}


/*
 * Check whether the given file is handled by this plugin
 */
gint xs_is_our_file(gchar * pcFilename)
{
	gchar *pcExt;
	assert(xs_status.sidPlayer);

	/* Check the filename */
	if (pcFilename == NULL)
		return FALSE;

	/* Try to detect via detection routine, if required */
	if (xs_cfg.detectMagic && xs_status.sidPlayer->plrIsOurFile(pcFilename))
		return TRUE;

	/* Detect just by checking filename extension */
	pcExt = xs_strrchr(pcFilename, '.');
	if (pcExt) {
		pcExt++;
		switch (xs_cfg.playerEngine) {
		case XS_ENG_SIDPLAY1:
		case XS_ENG_SIDPLAY2:
			if (!g_strcasecmp(pcExt, "psid"))
				return TRUE;
			if (!g_strcasecmp(pcExt, "sid"))
				return TRUE;
			if (!g_strcasecmp(pcExt, "dat"))
				return TRUE;
			if (!g_strcasecmp(pcExt, "inf"))
				return TRUE;
			if (!g_strcasecmp(pcExt, "info"))
				return TRUE;
			break;
		}
	}

	return FALSE;
}


/*
 * Main playing thread loop
 */
void *xs_playthread(void *argPointer)
{
	t_xs_status myStatus;
	t_xs_tuneinfo *myTune;
	gboolean audioOpen = FALSE, doPlay = FALSE, isFound = FALSE;
	gboolean playedTune[XS_STIL_MAXENTRY + 1];
	gint audioGot, songLength, i;
	gchar *audioBuffer = NULL, *oversampleBuffer = NULL;

	(void) argPointer;

	/* Initialize */
	XSDEBUG("entering player thread\n");
	g_static_mutex_lock(&xs_status_mutex);
	memcpy(&myStatus, &xs_status, sizeof(t_xs_status));
	myTune = xs_status.tuneInfo;
	g_static_mutex_unlock(&xs_status_mutex);

	xs_memset(&playedTune, 0, sizeof(playedTune));

	/* Allocate audio buffer */
	audioBuffer = (gchar *) g_malloc(XS_AUDIOBUF_SIZE);
	if (audioBuffer == NULL) {
		XSERR("Couldn't allocate memory for audio data buffer!\n");
		goto xs_err_exit;
	}

	if (myStatus.oversampleEnable) {
		oversampleBuffer = (gchar *) g_malloc(XS_AUDIOBUF_SIZE * myStatus.oversampleFactor);
		if (oversampleBuffer == NULL) {
			XSERR("Couldn't allocate memory for audio oversampling buffer!\n");
			goto xs_err_exit;
		}
	}

	/*
	 * Main player loop: while not stopped, loop here - play subtunes
	 */
	audioOpen = FALSE;
	doPlay = TRUE;
	while (xs_status.isPlaying && doPlay) {
		/* Automatic sub-tune change logic */
		g_static_mutex_lock(&xs_cfg_mutex);
		g_static_mutex_lock(&xs_status_mutex);
		assert(xs_status.currSong >= 1);
		assert(xs_status.currSong <= XS_STIL_MAXENTRY);
		myStatus.isPlaying = TRUE;

		if (xs_cfg.subAutoEnable && (myStatus.currSong == xs_status.currSong)) {
			/* Check if currently selected sub-tune has been played already */
			if (playedTune[myStatus.currSong]) {
				/* Find a tune that has not been played */
				XSDEBUG("tune #%i already played, finding next match ...\n", myStatus.currSong);
				isFound = FALSE;
				i = 0;
				while (!isFound && (++i <= myTune->nsubTunes)) {
					if (xs_cfg.subAutoMinOnly) {
						/* A tune with minimum length must be found */
						if (!playedTune[i]
						    && myTune->subTunes[i].tuneLength >= xs_cfg.subAutoMinTime)
							isFound = TRUE;
					} else {
						/* Any unplayed tune is okay */
						if (!playedTune[i])
							isFound = TRUE;
					}
				}

				if (isFound) {
					/* Set the new sub-tune */
					XSDEBUG("found #%i\n", i);
					xs_status.currSong = i;
				} else
					/* This is the end */
					doPlay = FALSE;

				g_static_mutex_unlock(&xs_status_mutex);
				g_static_mutex_unlock(&xs_cfg_mutex);
				continue;	/* This is ugly, but ... */
			}
		}

		/* Tell that we are initializing, update sub-tune controls */
		myStatus.currSong = xs_status.currSong;
		playedTune[myStatus.currSong] = TRUE;
		g_static_mutex_unlock(&xs_status_mutex);
		g_static_mutex_unlock(&xs_cfg_mutex);

		XSDEBUG("subtune #%i selected, initializing...\n", myStatus.currSong);

		GDK_THREADS_ENTER();
		xs_subctrl_update();
		GDK_THREADS_LEAVE();

		/* Check minimum playtime */
		songLength = myTune->subTunes[myStatus.currSong - 1].tuneLength;
		if (xs_cfg.playMinTimeEnable && (songLength >= 0)) {
			if (songLength < xs_cfg.playMinTime)
				songLength = xs_cfg.playMinTime;
		}

		/* Initialize song */
		if (!myStatus.sidPlayer->plrInitSong(&myStatus)) {
			XSERR("Couldn't initialize SID-tune '%s' (sub-tune #%i)!\n",
			      myTune->sidFilename, myStatus.currSong);
			goto xs_err_exit;
		}


		/* Open the audio output */
		if (!xs_plugin_ip.output->
		    open_audio(myStatus.audioFormat, myStatus.audioFrequency, myStatus.audioChannels)) {
			XSERR("Couldn't open XMMS audio output (fmt=%x, freq=%i, nchan=%i)!\n", myStatus.audioFormat,
			      myStatus.audioFrequency, myStatus.audioChannels);

			g_static_mutex_lock(&xs_status_mutex);
			xs_status.isError = TRUE;
			g_static_mutex_unlock(&xs_status_mutex);
			goto xs_err_exit;
		}

		audioOpen = TRUE;

		/* Set song information for current subtune */
		xs_plugin_ip.set_info(myTune->subTunes[myStatus.currSong - 1].tuneTitle,
				      (songLength > 0) ? (songLength * 1000) : -1,
				      (myTune->subTunes[myStatus.currSong - 1].tuneSpeed >
				       0) ? (myTune->subTunes[myStatus.currSong - 1].tuneSpeed * 1000) : -1,
				      myStatus.audioFrequency, myStatus.audioChannels);


		XSDEBUG("playing\n");

		/*
		 * Play the subtune
		 */
		while (xs_status.isPlaying && myStatus.isPlaying && (xs_status.currSong == myStatus.currSong)) {
			/* Render audio data */
			if (myStatus.oversampleEnable) {
				/* Perform oversampled rendering */
				audioGot = myStatus.sidPlayer->plrFillBuffer(&myStatus,
									     oversampleBuffer,
									     (XS_AUDIOBUF_SIZE *
									      myStatus.oversampleFactor));

				audioGot /= myStatus.oversampleFactor;

				/* Execute rate-conversion with filtering */
				if (xs_filter_rateconv(audioBuffer, oversampleBuffer,
						       myStatus.audioFormat, myStatus.oversampleFactor, audioGot) < 0) {
					XSERR("Oversampling rate-conversion pass failed.\n");
					g_static_mutex_lock(&xs_status_mutex);
					xs_status.isError = TRUE;
					g_static_mutex_unlock(&xs_status_mutex);
					goto xs_err_exit;
				}
			} else
				audioGot = myStatus.sidPlayer->plrFillBuffer(&myStatus, audioBuffer, XS_AUDIOBUF_SIZE);

			/* I <3 visualice/haujobb */
			produce_audio(xs_plugin_ip.output->written_time(),
				 myStatus.audioFormat, myStatus.audioChannels, audioGot, audioBuffer, NULL);

			/* Wait a little */
			while (xs_status.isPlaying &&
			       (xs_status.currSong == myStatus.currSong) &&
			       (xs_plugin_ip.output->buffer_free() < audioGot))
				xmms_usleep(500);

			/* Check if we have played enough */
			if (xs_cfg.playMaxTimeEnable) {
				if (xs_cfg.playMaxTimeUnknown) {
					if ((songLength < 0) &&
					    (xs_plugin_ip.output->output_time() >= (xs_cfg.playMaxTime * 1000)))
						myStatus.isPlaying = FALSE;
				} else {
					if (xs_plugin_ip.output->output_time() >= (xs_cfg.playMaxTime * 1000))
						myStatus.isPlaying = FALSE;
				}
			}

			if (songLength >= 0) {
				if (xs_plugin_ip.output->output_time() >= (songLength * 1000))
					myStatus.isPlaying = FALSE;
			}
		}

		XSDEBUG("subtune ended/stopped\n");

		/* Close audio output plugin */
		if (audioOpen) {
			XSDEBUG("close audio #1\n");
			xs_plugin_ip.output->close_audio();
			audioOpen = FALSE;
		}

		/* Now determine if we continue by selecting other subtune or something */
		if (!myStatus.isPlaying && !xs_cfg.subAutoEnable)
			doPlay = FALSE;
	}

      xs_err_exit:
	/* Close audio output plugin */
	if (audioOpen) {
		XSDEBUG("close audio #2\n");
		xs_plugin_ip.output->close_audio();
	}

	g_free(audioBuffer);
	g_free(oversampleBuffer);

	/* Set playing status to false (stopped), thus when
	 * XMMS next calls xs_get_time(), it can return appropriate
	 * value "not playing" status and XMMS knows to move to
	 * next entry in the playlist .. or whatever it wishes.
	 */
	g_static_mutex_lock(&xs_status_mutex);
	xs_status.isPlaying = FALSE;
	g_static_mutex_unlock(&xs_status_mutex);

	/* Exit the playing thread */
	XSDEBUG("exiting thread, bye.\n");
	g_thread_exit(NULL);
	return(NULL);
}


/*
 * Start playing the given file
 * Here we load the tune and initialize the playing thread.
 * Usually you would also initialize the output-plugin, but
 * this is XMMS-SID and we do it on the player thread instead.
 */
void xs_play_file(gchar * pcFilename)
{
	assert(xs_status.sidPlayer);

	XSDEBUG("play '%s'\n", pcFilename);

	/* Get tune information */
	if ((xs_status.tuneInfo = xs_status.sidPlayer->plrGetSIDInfo(pcFilename)) == NULL)
		return;

	/* Initialize the tune */
	if (!xs_status.sidPlayer->plrLoadSID(&xs_status, pcFilename)) {
		xs_tuneinfo_free(xs_status.tuneInfo);
		xs_status.tuneInfo = NULL;
		return;
	}

	XSDEBUG("load ok\n");

	/* Set general status information */
	xs_status.isPlaying = TRUE;
	xs_status.isError = FALSE;
	xs_status.currSong = xs_status.tuneInfo->startTune;

	/* Start the playing thread! */
	xs_decode_thread = g_thread_create((GThreadFunc)xs_playthread, NULL, TRUE, NULL);
	if (xs_decode_thread == NULL) {
		XSERR("Couldn't start playing thread!\n");
		xs_tuneinfo_free(xs_status.tuneInfo);
		xs_status.tuneInfo = NULL;
		xs_status.sidPlayer->plrDeleteSID(&xs_status);
	}

	/* Okay, here the playing thread has started up and we
	 * return from here to XMMS. Rest is up to XMMS's GUI
	 * and playing thread.
	 */
	XSDEBUG("systems should be up?\n");
}


/*
 * Stop playing
 * Here we set the playing status to stop and wait for playing
 * thread to shut down. In any "correctly" done plugin, this is
 * also the function where you close the output-plugin, but since
 * XMMS-SID has special behaviour (audio opened/closed in the
 * playing thread), we don't do that here.
 *
 * Finally tune and other memory allocations are free'd.
 */
void xs_stop(void)
{
	XSDEBUG("STOP_REQ\n");

	/* Close the sub-tune control window, if any */
	xs_subctrl_close();

	/* Lock xs_status and stop playing thread */
	g_static_mutex_lock(&xs_status_mutex);
	if (xs_status.isPlaying) {
		/* Stop playing */
		XSDEBUG("stopping...\n");
		xs_status.isPlaying = FALSE;
		g_static_mutex_unlock(&xs_status_mutex);
		g_thread_join(xs_decode_thread);
	} else {
		g_static_mutex_unlock(&xs_status_mutex);
	}

	/* Status is now stopped, update the sub-tune
	 * controller in fileinfo window (if open)
	 */
	xs_fileinfo_update();

	/* Free tune information */
	xs_status.sidPlayer->plrDeleteSID(&xs_status);
	xs_tuneinfo_free(xs_status.tuneInfo);
	xs_status.tuneInfo = NULL;
}


/*
 * Pause/unpause the playing
 */
void xs_pause(short pauseState)
{
	g_static_mutex_lock(&xs_status_mutex);
	/* FIXME FIX ME todo: pause should disable sub-tune controls */
	g_static_mutex_unlock(&xs_status_mutex);

	xs_subctrl_close();
	xs_fileinfo_update();
	xs_plugin_ip.output->pause(pauseState);
}


/*
 * Pop-up subtune selector
 */
void xs_subctrl_setsong(void)
{
	gint n;

	g_static_mutex_lock(&xs_status_mutex);
	g_static_mutex_lock(&xs_subctrl_mutex);

	if (xs_status.tuneInfo && xs_status.isPlaying) {
		n = (gint) GTK_ADJUSTMENT(xs_subctrl_adj)->value;
		if ((n >= 1) && (n <= xs_status.tuneInfo->nsubTunes))
			xs_status.currSong = n;
	}

	g_static_mutex_unlock(&xs_subctrl_mutex);
	g_static_mutex_unlock(&xs_status_mutex);
}


void xs_subctrl_prevsong(void)
{
	g_static_mutex_lock(&xs_status_mutex);

	if (xs_status.tuneInfo && xs_status.isPlaying) {
		if (xs_status.currSong > 1)
			xs_status.currSong--;
	}

	g_static_mutex_unlock(&xs_status_mutex);

	xs_subctrl_update();
}


void xs_subctrl_nextsong(void)
{
	g_static_mutex_lock(&xs_status_mutex);

	if (xs_status.tuneInfo && xs_status.isPlaying) {
		if (xs_status.currSong < xs_status.tuneInfo->nsubTunes)
			xs_status.currSong++;
	}

	g_static_mutex_unlock(&xs_status_mutex);

	xs_subctrl_update();
}


void xs_subctrl_update(void)
{
	GtkAdjustment *tmpAdj;

	g_static_mutex_lock(&xs_status_mutex);
	g_static_mutex_lock(&xs_subctrl_mutex);

	/* Check if control window exists, we are currently playing and have a tune */
	if (xs_subctrl) {
		if (xs_status.tuneInfo && xs_status.isPlaying) {
			tmpAdj = GTK_ADJUSTMENT(xs_subctrl_adj);

			tmpAdj->value = xs_status.currSong;
			tmpAdj->lower = 1;
			tmpAdj->upper = xs_status.tuneInfo->nsubTunes;
			g_static_mutex_unlock(&xs_status_mutex);
			g_static_mutex_unlock(&xs_subctrl_mutex);
			gtk_adjustment_value_changed(tmpAdj);
		} else {
			g_static_mutex_unlock(&xs_status_mutex);
			g_static_mutex_unlock(&xs_subctrl_mutex);
			xs_subctrl_close();
		}
	} else {
		g_static_mutex_unlock(&xs_subctrl_mutex);
		g_static_mutex_unlock(&xs_status_mutex);
	}

	xs_fileinfo_update();
}


void xs_subctrl_close(void)
{
	g_static_mutex_lock(&xs_subctrl_mutex);

	if (xs_subctrl) {
		gtk_widget_destroy(xs_subctrl);
		xs_subctrl = NULL;
	}

	g_static_mutex_unlock(&xs_subctrl_mutex);
}


gboolean xs_subctrl_keypress(GtkWidget * win, GdkEventKey * ev)
{
	(void) win;

	if (ev->keyval == GDK_Escape)
		xs_subctrl_close();

	return FALSE;
}


void xs_subctrl_open(void)
{
	GtkWidget *frame25, *hbox15, *subctrl_prev, *subctrl_current, *subctrl_next;

	g_static_mutex_lock(&xs_subctrl_mutex);
	if (!xs_status.tuneInfo || !xs_status.isPlaying || xs_subctrl || (xs_status.tuneInfo->nsubTunes <= 1)) {
		g_static_mutex_unlock(&xs_subctrl_mutex);
		return;
	}

	/* Create the pop-up window */
	xs_subctrl = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint (GTK_WINDOW(xs_subctrl), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_widget_set_name(xs_subctrl, "xs_subctrl");
	gtk_object_set_data(GTK_OBJECT(xs_subctrl), "xs_subctrl", xs_subctrl);

	gtk_window_set_title(GTK_WINDOW(xs_subctrl), "Subtune Control");
	gtk_window_set_position(GTK_WINDOW(xs_subctrl), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(xs_subctrl), 0);
	gtk_window_set_policy(GTK_WINDOW(xs_subctrl), FALSE, FALSE, FALSE);

	gtk_signal_connect(GTK_OBJECT(xs_subctrl), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &xs_subctrl);

	gtk_signal_connect(GTK_OBJECT(xs_subctrl), "focus_out_event", GTK_SIGNAL_FUNC(xs_subctrl_close), NULL);

	gtk_widget_realize(xs_subctrl);
	gdk_window_set_decorations(xs_subctrl->window, (GdkWMDecoration) 0);


	/* Create the control widgets */
	frame25 = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(xs_subctrl), frame25);
	gtk_container_set_border_width(GTK_CONTAINER(frame25), 2);
	gtk_frame_set_shadow_type(GTK_FRAME(frame25), GTK_SHADOW_OUT);

	hbox15 = gtk_hbox_new(FALSE, 4);
	gtk_container_add(GTK_CONTAINER(frame25), hbox15);

	subctrl_prev = gtk_button_new_with_label(" < ");
	gtk_widget_set_name(subctrl_prev, "subctrl_prev");
	gtk_box_pack_start(GTK_BOX(hbox15), subctrl_prev, FALSE, FALSE, 0);

	xs_subctrl_adj = gtk_adjustment_new(xs_status.currSong, 1, xs_status.tuneInfo->nsubTunes, 1, 1, 0);
	gtk_signal_connect(GTK_OBJECT(xs_subctrl_adj), "value_changed", GTK_SIGNAL_FUNC(xs_subctrl_setsong), NULL);

	subctrl_current = gtk_hscale_new(GTK_ADJUSTMENT(xs_subctrl_adj));
	gtk_widget_set_name(subctrl_current, "subctrl_current");
	gtk_box_pack_start(GTK_BOX(hbox15), subctrl_current, FALSE, TRUE, 0);
	gtk_scale_set_digits(GTK_SCALE(subctrl_current), 0);
	gtk_range_set_update_policy(GTK_RANGE(subctrl_current), GTK_UPDATE_DELAYED);
	gtk_widget_grab_focus(subctrl_current);

	subctrl_next = gtk_button_new_with_label(" > ");
	gtk_widget_set_name(subctrl_next, "subctrl_next");
	gtk_box_pack_start(GTK_BOX(hbox15), subctrl_next, FALSE, FALSE, 0);

	gtk_signal_connect(GTK_OBJECT(subctrl_prev), "clicked", GTK_SIGNAL_FUNC(xs_subctrl_prevsong), NULL);

	gtk_signal_connect(GTK_OBJECT(subctrl_next), "clicked", GTK_SIGNAL_FUNC(xs_subctrl_nextsong), NULL);

	gtk_signal_connect(GTK_OBJECT(xs_subctrl), "key_press_event", GTK_SIGNAL_FUNC(xs_subctrl_keypress), NULL);

	gtk_widget_show_all(xs_subctrl);

	g_static_mutex_unlock(&xs_subctrl_mutex);
}


/*
 * Set the time-seek position
 * The playing thread will do the "seeking", which means sub-tune
 * changing in XMMS-SID's case. iTime argument is time in seconds,
 * in contrast to milliseconds used in other occasions.
 *
 * This function is called whenever position slider is clicked or
 * other method of seeking is used (keyboard, etc.)
 */
void xs_seek(gint iTime)
{
	/* Check status */
	g_static_mutex_lock(&xs_status_mutex);
	if (!xs_status.tuneInfo || !xs_status.isPlaying) {
		g_static_mutex_unlock(&xs_status_mutex);
		return;
	}

	/* Act according to settings */
	switch (xs_cfg.subsongControl) {
	case XS_SSC_SEEK:
		if (iTime < xs_status.lastTime) {
			if (xs_status.currSong > 1)
				xs_status.currSong--;
		} else if (iTime > xs_status.lastTime) {
			if (xs_status.currSong < xs_status.tuneInfo->nsubTunes)
				xs_status.currSong++;
		}
		break;

	case XS_SSC_POPUP:
		xs_subctrl_open();
		break;

		/* If we have song-position patch, check settings */
#ifdef HAVE_SONG_POSITION
	case XS_SSC_PATCH:
		if ((iTime > 0) && (iTime <= xs_status.tuneInfo->nsubTunes))
			xs_status.currSong = iTime;
		break;
#endif
	}

	g_static_mutex_unlock(&xs_status_mutex);
}


/*
 * Return the playing "position/time"
 * Determine current position/time in song. Used by XMMS to update
 * the song clock and position slider and MOST importantly to determine
 * END OF SONG! Return value of -2 means error, XMMS opens an audio
 * error dialog. -1 means end of song (if one was playing currently).
 */
gint xs_get_time(void)
{
	/* If errorflag is set, return -2 to signal it to XMMS's idle callback */
	g_static_mutex_lock(&xs_status_mutex);
	if (xs_status.isError) {
		g_static_mutex_unlock(&xs_status_mutex);
		return -2;
	}

	/* If there is no tune, return -1 */
	if (!xs_status.tuneInfo) {
		g_static_mutex_unlock(&xs_status_mutex);
		return -1;
	}

	/* If tune has ended, return -1 */
	if (!xs_status.isPlaying) {
		g_static_mutex_unlock(&xs_status_mutex);
		return -1;
	}

	/* Let's see what we do */
	switch (xs_cfg.subsongControl) {
	case XS_SSC_SEEK:
		xs_status.lastTime = (xs_plugin_ip.output->output_time() / 1000);
		break;

#ifdef HAVE_SONG_POSITION
	case XS_SSC_PATCH:
		set_song_position(xs_status.currSong, 1, xs_status.tuneInfo->nsubTunes);
		break;
#endif
	}

	g_static_mutex_unlock(&xs_status_mutex);

	/* Return output time reported by audio output plugin */
	return xs_plugin_ip.output->output_time();
}


/*
 * Return song information
 * This function is called by XMMS when initially loading the playlist.
 * Subsequent changes to information are made by the player thread,
 * which uses xs_plugin_ip.set_info();
 */
void xs_get_song_info(gchar * songFilename, gchar ** songTitle, gint * songLength)
{
	t_xs_tuneinfo *pInfo;
	gint tmpInt;

	/* Get tune information from emulation engine */
	pInfo = xs_status.sidPlayer->plrGetSIDInfo(songFilename);
	if (!pInfo)
		return;

	/* Get sub-tune information, if available */
	if ((pInfo->startTune > 0) && (pInfo->startTune <= pInfo->nsubTunes)) {
		(*songTitle) = g_strdup(pInfo->subTunes[pInfo->startTune - 1].tuneTitle);

		tmpInt = pInfo->subTunes[pInfo->startTune - 1].tuneLength;
		if (tmpInt < 0)
			(*songLength) = -1;
		else
			(*songLength) = (tmpInt * 1000);
	}

	/* Free tune information */
	xs_tuneinfo_free(pInfo);
}


/* Allocate a new tune information structure
 */
t_xs_tuneinfo *xs_tuneinfo_new(gchar * pcFilename, gint nsubTunes, gint startTune,
			       gchar * sidName, gchar * sidComposer, gchar * sidCopyright,
			       gint loadAddr, gint initAddr, gint playAddr, gint dataFileLen)
{
	t_xs_tuneinfo *pResult;

	/* Allocate structure */
	pResult = (t_xs_tuneinfo *) g_malloc0(sizeof(t_xs_tuneinfo));
	if (!pResult) {
		XSERR("Could not allocate memory for t_xs_tuneinfo ('%s')\n", pcFilename);
		return NULL;
	}

	pResult->sidFilename = g_strdup(pcFilename);
	if (!pResult->sidFilename) {
		XSERR("Could not allocate sidFilename ('%s')\n", pcFilename);
		g_free(pResult);
		return NULL;
	}

	/* Allocate space for subtune information */
	if (nsubTunes > 0) {
		pResult->subTunes = g_malloc0(sizeof(t_xs_subtuneinfo) * nsubTunes);
		if (!pResult->subTunes) {
			XSERR("Could not allocate memory for t_xs_subtuneinfo ('%s', %i)\n", pcFilename, nsubTunes);

			g_free(pResult->sidFilename);
			g_free(pResult);
			return NULL;
		}
	}

	/* The following allocations don't matter if they fail */
	pResult->sidName = g_strdup(sidName);
	pResult->sidComposer = g_strdup(sidComposer);
	pResult->sidCopyright = g_strdup(sidCopyright);

	pResult->nsubTunes = nsubTunes;
	pResult->startTune = startTune;

	pResult->loadAddr = loadAddr;
	pResult->initAddr = initAddr;
	pResult->playAddr = playAddr;
	pResult->dataFileLen = dataFileLen;

	return pResult;
}


/* Free given tune information structure
 */
void xs_tuneinfo_free(t_xs_tuneinfo * pTune)
{
	gint i;
	if (!pTune)
		return;

	for (i = 0; i < pTune->nsubTunes; i++) {
		g_free(pTune->subTunes[i].tuneTitle);
		pTune->subTunes[i].tuneTitle = NULL;
	}

	g_free(pTune->subTunes);
	pTune->nsubTunes = 0;
	g_free(pTune->sidFilename);
	pTune->sidFilename = NULL;
	g_free(pTune->sidName);
	pTune->sidName = NULL;
	g_free(pTune->sidComposer);
	pTune->sidComposer = NULL;
	g_free(pTune->sidCopyright);
	pTune->sidCopyright = NULL;
	g_free(pTune);
}
