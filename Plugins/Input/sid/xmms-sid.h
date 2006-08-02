/*  
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   Main header file

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
*/
#ifndef _XMMS_SID_H
#define _XMMS_SID_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(x) /* stub */
#endif

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "audacious/plugin.h"

/*
 * Some constants and defines
 */
/* #define to enable spurious debugging messages for development
 * purposes. Output goes to stderr. See also DEBUG_NP below.
 */
#undef DEBUG

/* Define to ISO C99 macro for debugging instead of varargs function.
 * This provides more useful information, but is incompatible with
 * older standards. If #undef'd, a varargs function is used instead.
 */
#undef DEBUG_NP

/* HardSID-support is not working and is untested, thus we disable it here.
 */
#undef HAVE_HARDSID_BUILDER

/* Size for some small buffers (always static variables) */
#define XS_BUF_SIZE		(1024)

/* If defined, some dynamically allocated temp. buffers are used.
 * Static (#undef) might give slight performance gain,
 * but fails on systems with limited stack space. */
#define XS_BUF_DYNAMIC

/* Size of audio buffer. If you are experiencing lots of audio
 * "underruns" or clicks/gaps in output, try increasing this value.
 * Do notice, however, that it also affects the update frequency of
 * XMMS's visualization plugins... 
 */
#define XS_AUDIOBUF_SIZE	(2*1024)

/* Size of data buffer used for SID-tune MD5 hash calculation.
 * If this is too small, the computed hash will be incorrect.
 * Largest SID files I've seen are ~70kB. */
#define XS_SIDBUF_SIZE		(80*1024)


/* libSIDPlay1 default filter values (copied from libsidplay1's headers) */
#define XS_SIDPLAY1_FS		(400.0f)
#define XS_SIDPLAY1_FM		(60.0f)
#define XS_SIDPLAY1_FT		(0.05f)


#define XS_BIN_BAILOUT		(32)	/* Binary search bailout treshold */

#define XS_STIL_MAXENTRY	(128)	/* Max number of sub-songs in STIL/SLDB node */


#define XS_CONFIG_IDENT		"sid"		/* Configuration file identifier */

#define XS_MIN_OVERSAMPLE	(2)		/* Minimum oversampling factor */
#define XS_MAX_OVERSAMPLE	(8)		/* Maximum oversampling factor */


/* Shorthands for linked lists
 */
#define LPREV	(pNode->pPrev)
#define LTHIS	(pNode)
#define LNEXT	(pNode->pNext)


/* Plugin-wide typedefs
 */
typedef struct {
	gint		tuneSpeed;
	gint		tuneLength;
	gchar		*tuneTitle;
} t_xs_subtuneinfo;


typedef struct {
	gchar		*sidFilename,
			*sidName,
			*sidComposer,
			*sidCopyright;
	gint		loadAddr,
			initAddr,
			playAddr,
			dataFileLen;
	gint		nsubTunes, startTune;
	t_xs_subtuneinfo	*subTunes;
} t_xs_tuneinfo;


struct t_xs_status;

typedef struct {
	gint		plrIdent;
	gboolean	(*plrIsOurFile)(gchar *);
	gboolean	(*plrInit)(struct t_xs_status *);
	void		(*plrClose)(struct t_xs_status *);
	gboolean	(*plrInitSong)(struct t_xs_status *);
	guint		(*plrFillBuffer)(struct t_xs_status *, gchar *, guint);
	gboolean	(*plrLoadSID)(struct t_xs_status *, gchar *);
	void		(*plrDeleteSID)(struct t_xs_status *);
	t_xs_tuneinfo*	(*plrGetSIDInfo)(gchar *);
} t_xs_player;


typedef struct t_xs_status {
	gint		audioFrequency,		/* Audio settings */
			audioChannels,
			audioBitsPerSample,
			oversampleFactor;	/* Factor of oversampling */
	AFormat		audioFormat;
	gboolean	oversampleEnable;	/* TRUE after sidEngine initialization,
						if xs_cfg.oversampleEnable == TRUE and
						emulation backend supports oversampling.
						*/
	void		*sidEngine;		/* SID-emulation internal engine data */
	t_xs_player	*sidPlayer;		/* Selected player engine */
	gboolean	isError, isPlaying;
	gint		currSong,		/* Current sub-tune */
			lastTime;
	t_xs_tuneinfo	*tuneInfo;
} t_xs_status;


/* Global variables
 */
extern InputPlugin	xs_plugin_ip;
extern t_xs_status	xs_status;

/* Plugin function prototypes
 */
void	xs_init(void);
void	xs_reinit(void);
void	xs_close(void);
gint	xs_is_our_file(gchar *);
void	xs_play_file(gchar *);
void	xs_stop(void);
void	xs_pause(short);
void	xs_seek(gint);
gint	xs_get_time(void);
void	xs_get_song_info(gchar *, gchar **, gint *);
void	xs_about(void);

t_xs_tuneinfo *xs_tuneinfo_new(gchar *, gint, gint, gchar *, gchar *, gchar *, gint, gint, gint, gint);
void	xs_tuneinfo_free(t_xs_tuneinfo *);

void	XSERR(const char *, ...);

#ifndef DEBUG_NP
void	XSDEBUG(const char *, ...);
#else
#ifdef DEBUG
#define XSDEBUG(...) { fprintf(stderr, "XS[%s:%s:%d]: ", __FILE__, __FUNCTION__, (int) __LINE__); fprintf(stderr, __VA_ARGS__); }
#else
#define XSDEBUG(...) /* stub */
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif /* _XMMS_SID_H */
