/* 
 *  XMMS Crossfade Plugin
 *  Copyright (C) 2000-2004  Peter Eisenlohr <peter@eisenlohr.org>
 *
 *  based on the original OSS Output Plugin
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#ifndef _CROSSFADE_H_
#define _CROSSFADE_H_

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "audacious/plugin.h"
#include "libaudacious/beepctrl.h"
#include "libaudacious/configdb.h"
#include "libaudacious/util.h"

#include "debug.h"

#undef  VOLUME_NORMALIZER
#undef  SONGCHANGE_TIMEOUT

#define OUTPUT_RATE the_rate
#define OUTPUT_NCH  2
#define OUTPUT_BPS (OUTPUT_RATE * OUTPUT_NCH * 2)

#define SYNC_OUTPUT_TIMEOUT 2000

#define OUTPUT_METHOD_PLUGIN       0
#define OUTPUT_METHOD_BUILTIN_NULL 1

#define FADE_CONFIG_XFADE   0
#define FADE_CONFIG_MANUAL  1
#define FADE_CONFIG_ALBUM   2
#define FADE_CONFIG_START   3
#define FADE_CONFIG_STOP    4
#define FADE_CONFIG_EOP     5
#define FADE_CONFIG_SEEK    6
#define FADE_CONFIG_PAUSE   7
#define FADE_CONFIG_TIMING  8
#define MAX_FADE_CONFIGS    9

#define FADE_TYPE_REOPEN      0
#define FADE_TYPE_FLUSH       1
#define FADE_TYPE_NONE        2
#define FADE_TYPE_PAUSE       3
#define FADE_TYPE_SIMPLE_XF   4
#define FADE_TYPE_ADVANCED_XF 5
#define FADE_TYPE_FADEIN      6
#define FADE_TYPE_FADEOUT     7
#define FADE_TYPE_PAUSE_NONE  8
#define FADE_TYPE_PAUSE_ADV   9
#define MAX_FADE_TYPES        10

#define TYPEMASK_XFADE ((1 << FADE_TYPE_REOPEN) |	\
			(1 << FADE_TYPE_NONE) |		\
			(1 << FADE_TYPE_PAUSE) |	\
			(1 << FADE_TYPE_SIMPLE_XF) |	\
			(1 << FADE_TYPE_ADVANCED_XF))

#define TYPEMASK_MANUAL ((1 << FADE_TYPE_REOPEN) |	\
			 (1 << FADE_TYPE_FLUSH) |	\
		         (1 << FADE_TYPE_NONE) |	\
			 (1 << FADE_TYPE_PAUSE) |	\
			 (1 << FADE_TYPE_SIMPLE_XF) |	\
			 (1 << FADE_TYPE_ADVANCED_XF))

#define TYPEMASK_ALBUM ((1 << FADE_TYPE_NONE))

#define TYPEMASK_START ((1 << FADE_TYPE_NONE) |	\
			(1 << FADE_TYPE_FADEIN))

#define TYPEMASK_STOP ((1 << FADE_TYPE_NONE) | \
		       (1 << FADE_TYPE_FADEOUT))

#define TYPEMASK_EOP ((1 << FADE_TYPE_NONE) | \
		      (1 << FADE_TYPE_FADEOUT))

#define TYPEMASK_SEEK ((1 << FADE_TYPE_FLUSH) | \
		       (1 << FADE_TYPE_NONE)  | \
		       (1 << FADE_TYPE_SIMPLE_XF))

#define TYPEMASK_PAUSE ((1 << FADE_TYPE_PAUSE_NONE) | \
	                      (1 << FADE_TYPE_PAUSE_ADV))

#define TYPEMASK_TIMING (0)

#define FC_OFFSET_NONE     0
#define FC_OFFSET_LOCK_IN  1
#define FC_OFFSET_LOCK_OUT 2
#define FC_OFFSET_CUSTOM   3

#define DEFAULT_OP_CONFIG_STRING     "libALSA.so=0,1,2304,0; libdisk_writer.so=1,0,2304,1"
#define DEFAULT_OP_NAME              "libALSA.so"
#define DEFAULT_EP_NAME              "libnormvol.so"

#define DEFAULT_OP_CONFIG			\
{ FALSE, FALSE, 2304, FALSE }

#define CONFIG_DEFAULT							\
{ 44100,                        /* output_rate */			\
	2,                            /* output_quality */			\
	NULL,                         /* op_config_string */			\
	NULL,                         /* op_name */				\
	NULL,                         /* ep_name */				\
	FALSE,                        /* ep_enable */				\
	TRUE,                         /* volnorm_enable */			\
	8000,                         /* volnorm_target */			\
	FALSE,                        /* volnorm_use_qa */			\
	10000,                        /* mix_size_ms */			\
	TRUE,                         /* mix_size_auto */			\
									\
	{                             /* fc[MAX_FADE_CONFIGS] */		\
	  { FADE_CONFIG_XFADE,        /* config */				\
	    FADE_TYPE_ADVANCED_XF,    /* type */				\
	    2000,                     /* pause_len_ms */			\
	    6000,                     /* simple_len_ms */			\
	    TRUE, 4000, 0,            /* out_enable, _len_ms, _volume */	\
	    FC_OFFSET_CUSTOM,         /* ofs_type */				\
	    FC_OFFSET_CUSTOM, -6000,  /* ofs_type_wanted, ofs_custom_ms */	\
	    TRUE,                     /* in_locked */				\
	    FALSE, 4000, 33,          /* in_enable, _len_ms, _volume */	\
	    FALSE,                    /* - */					\
	    0,                        /* - */					\
	    FALSE,                    /* - */					\
	    0, 0,                     /* - */					\
	    FALSE,                    /* flush */				\
	    TYPEMASK_XFADE            /* type_mask */				\
	  },									\
	  { FADE_CONFIG_MANUAL,       /* config */				\
	    FADE_TYPE_SIMPLE_XF,      /* type */				\
	    2000,                     /* pause_len_ms */			\
	    1000,                     /* simple_len_ms */			\
	    TRUE, 500, 0,             /* out_enable, _len_ms, _volume */	\
	    FC_OFFSET_CUSTOM,         /* ofs_type */				\
	    FC_OFFSET_CUSTOM, -500,   /* ofs_type_wanted, ofs_custom_ms */	\
	    TRUE,                     /* in_locked */				\
	    FALSE, 500, 50,           /* in_enable, _len_ms, _volume */	\
	    FALSE,                    /* flush_pause_enable */		\
	    500,                      /* flush_in_len_ms */			\
	    FALSE,                    /* flush_in_enable */			\
	    500, 0,                   /* flush_in_len_ms, _volume */		\
	    TRUE,                     /* flush */				\
	    TYPEMASK_MANUAL           /* type_mask */				\
	  },									\
	  { FADE_CONFIG_ALBUM,        /* config */				\
	    FADE_TYPE_NONE,           /* type */				\
	    0,                        /* - */					\
	    0,                        /* - */					\
	    FALSE, 0, 0,              /* - */					\
	    FC_OFFSET_NONE,           /* - */					\
	    FC_OFFSET_NONE, 0,        /* - */					\
	    FALSE,                    /* - */					\
	    FALSE, 1000, 0,           /* - */					\
	    FALSE,                    /* - */					\
	    0,                        /* - */					\
	    FALSE,                    /* - */					\
	    0, 0,                     /* - */					\
	    FALSE,                    /* flush */				\
	    TYPEMASK_ALBUM            /* type_mask */				\
	  },									\
	  { FADE_CONFIG_START,        /* config */				\
	    FADE_TYPE_FADEIN,         /* type */				\
	    0,                        /* - */					\
	    0,                        /* - */					\
	    FALSE, 0, 0,              /* - */					\
	    FC_OFFSET_NONE,           /* - */					\
	    FC_OFFSET_NONE, 0,        /* - */					\
	    FALSE,                    /* - */					\
	    FALSE, 100, 0,            /* - in_len_ms, _volume */		\
	    FALSE,                    /* - */					\
	    0,                        /* - */					\
	    FALSE,                    /* - */					\
	    0, 0,                     /* - */					\
	    TRUE,                     /* flush */				\
	    TYPEMASK_START            /* type_mask */				\
	  },									\
	  { FADE_CONFIG_STOP,         /* config */				\
	    FADE_TYPE_FADEOUT,        /* type */				\
	    0,                        /* - */					\
	    0,                        /* - */					\
	    FALSE, 100, 0,            /* - out_len_ms, _volume */		\
	    FC_OFFSET_NONE,           /* - */					\
	    FC_OFFSET_NONE, 500,      /* - ofs_custom_ms */			\
	    FALSE,                    /* - */					\
	    FALSE, 0, 0,              /* - */					\
	    FALSE,                    /* - */					\
	    0,                        /* - */					\
	    FALSE,                    /* - */					\
	    0, 0,                     /* - */					\
	    TRUE,                     /* flush */				\
	    TYPEMASK_STOP             /* type_mask */				\
	  },									\
	  { FADE_CONFIG_EOP,          /* config */				\
	    FADE_TYPE_FADEOUT,        /* type */				\
	    0,                        /* - */					\
	    0,                        /* - */					\
	    FALSE, 100, 0,            /* - out_len_ms, _volume */		\
	    FC_OFFSET_NONE,           /* - */					\
	    FC_OFFSET_NONE, 500,      /* - ofs_custom_ms  */			\
	    FALSE,                    /* - */					\
	    FALSE, 0, 0,              /* - */					\
	    FALSE,                    /* - */					\
	    0,                        /* - */					\
	    FALSE,                    /* - */					\
	    0, 0,                     /* - */					\
	    FALSE,                    /* flush */				\
	    TYPEMASK_EOP              /* type_mask */				\
	  },									\
	  { FADE_CONFIG_SEEK,         /* config */				\
	    FADE_TYPE_SIMPLE_XF,      /* type */				\
	    0,                        /* - */					\
	    50,                       /* simple_len_ms */			\
	    FALSE, 0, 0,              /* - */					\
	    FC_OFFSET_NONE,           /* - */					\
	    FC_OFFSET_NONE, 0,        /* - */					\
	    FALSE,                    /* - */					\
	    FALSE, 1000, 0,           /* - */					\
	    FALSE,                    /* - */					\
	    0,                        /* - */					\
	    FALSE,                    /* - */					\
	    0, 0,                     /* - */					\
	    TRUE,                     /* flush */				\
	    TYPEMASK_SEEK             /* type_mask */				\
	  },									\
	  { FADE_CONFIG_PAUSE,        /* config */				\
	    FADE_TYPE_PAUSE_ADV,      /* type */				\
	    0,                        /* - */					\
	    0,                        /* - */					\
	    TRUE, 100, 0,             /* - out_len_ms, - */			\
	    FC_OFFSET_NONE,           /* - */					\
	    FC_OFFSET_NONE, 100,      /* - ofs_custom_ms */			\
	    FALSE,                    /* - */					\
	    TRUE, 100, 0,             /* - in_len_ms, - */			\
	    FALSE,                    /* - */					\
	    0,                        /* - */					\
	    FALSE,                    /* - */					\
	    0, 0,                     /* - */					\
	    FALSE,                    /* - */					\
	    TYPEMASK_PAUSE            /* type_mask */				\
	  },									\
	  { FADE_CONFIG_TIMING,       /* config */				\
	    FADE_TYPE_NONE,           /* type */				\
	    0,                        /* - */					\
	    0,                        /* - */					\
	    TRUE, 0, 0,               /* out_enable, _len_ms, - */		\
	    FC_OFFSET_CUSTOM,         /* - */					\
	    FC_OFFSET_CUSTOM, 0,      /* - ofs_custom_ms */			\
	    FALSE,                    /* - */					\
	    TRUE, 0, 0,               /* in_enable, _len_ms, - */		\
	    FALSE,                    /* - */					\
	    0,                        /* - */					\
	    FALSE,                    /* - */					\
	    0, 0,                     /* - */					\
	    FALSE,                    /* flush */				\
	    TYPEMASK_TIMING           /* type_mask */				\
	  }									\
	},									\
	TRUE,                         /* gap_lead_enable */			\
	500,                          /* gap_lead_len_ms */			\
	512,                          /* gap_lead_level */			\
	TRUE,                         /* gap_trail_enable */			\
	500,                          /* gap_trail_len_ms */			\
	512,                          /* gap_trail_level */			\
	TRUE,                         /* gap_trail_locked */			\
	TRUE,                         /* gap_crossing */			\
	FALSE,                        /* enable_debug */			\
	FALSE,                        /* enable_monitor */			\
	TRUE,                         /* enable_mixer */			\
	FALSE,                        /* mixer_reverse */			\
	FALSE,                        /* mixer_software */			\
	75,                           /* mixer_vol_left */			\
	75,                           /* mixer_vol_right */			\
	500,                          /* songchange_timeout */		\
	0,                            /* preload_size_ms */			\
	TRUE,                         /* album detection */			\
	FALSE,                        /* no_xfade_if_same_file */		\
	FALSE,                        /* enable_http_workaround */		\
	FALSE,                        /* enable_op_max_used */		\
	250,                          /* op_max_used_ms */			\
	FALSE,                        /* output_keep_opened */		\
	NULL,                         /* presets */				\
	250                           /* sync_size_ms */			\
}


#define DEBUG(x)  {if(config->enable_debug) debug x;}
#define PERROR(x) {if(config->enable_debug) perror(x);}

#define WRAP(x,n) (((x)<0)?((n)-(x))%(n):((x)%(n)))
#define B2MS(x)   ((gint)((gint64)(x)*1000/OUTPUT_BPS))
#define MS2B(x)   ((gint)((gint64)(x)*OUTPUT_BPS/1000))


/* get plugin info (imported by XMMS) */
OutputPlugin *get_oplugin_info();
OutputPlugin *get_crossfade_oplugin_info();

/* xmms internal prototypes */ /* XMMS */
gint     ctrlsocket_get_session_id();
gboolean bmp_playback_get_playing();
GList   *get_output_list();
GList   *get_effect_list();
gint     get_playlist_position();
gint     playlist_get_current_length();
#ifdef HAVE_PLAYLIST_GET_FILENAME
gchar *playlist_get_filename(gint pos);
#endif

/* config change callbacks */
void xfade_realize_config();
void xfade_realize_ep_config();


typedef struct
{
	gint mix_size;        /* mixing buffer length */
	gint sync_size;       /* additional buffer space for mix timing */
	gint preload_size;    /* preload buffer length */

	/* ---------------------------------------------------------------------- */

	gpointer data;        /* buffer */
	gint     size;        /* total buffer length */

	/* ---------------------------------------------------------------------- */

	gint   used;          /* length */
	gint   rd_index;      /* offset */

	gint   preload;       /* > 0: preloading */

	/* ---------------------------------------------------------------------- */

	gint   mix;           /* > 0: mixing new data into buffer */

	gint   fade;          /* > 0: fading in new data */
	gint   fade_len;      /* length of fadein */
	gfloat fade_scale;    /* 1.0f - in_level */

#define GAP_SKIPPING_POSITIVE -1
#define GAP_SKIPPING_NEGATIVE -2
#define GAP_SKIPPING_DONE     -3
	gint   gap;           /* > 0: removing (leading) gap */
	gint   gap_len;       /* max. len of gap, 0=disabled */
	gint   gap_level;     /* max. sample value+1 to be considered "silent" */
	gint   gap_killed;    /* number of bytes that were killed last time */
	gint   gap_skipped;   /* negative/positive samples skipped */

	/* ---------------------------------------------------------------------- */

	gint   silence;       /* > 0: delay until start of silence */
	gint   silence_len;   /* > 0: inserting silence */

	gint     reopen;      /* >= 0: countdown to reopen device (disk_writer hack) */
	gboolean reopen_sync; /* TRUE: sync output plugin before reopening */

	gint     pause;       /* >= 0: countdown to pause output plugin */
}
buffer_t;

typedef struct
{
	gint     config;              // one of FADE_CONFIG_*, constant
	gint     type;                // one of FADE_TYPE_*

	gint     pause_len_ms;        // PAUSE

	gint     simple_len_ms;       // SIMPLE_XF

	gboolean out_enable;          // ADVANCED_XF
	gint     out_len_ms;          // ADVANCED_XF, FADEOUT, PAUSE
	gint     out_volume;          // ADVANCED_XF, FADEOUT

	gint     ofs_type;            // ADVANCED_XF
	gint     ofs_type_wanted;     // ADVANCED_XF
	gint     ofs_custom_ms;       // ADVANCED_XF, FADEOUT (additional silence), PAUSE

	gboolean in_locked;           // ADVANCED_XF
	gboolean in_enable;           // ADVANCED_XF, FLUSH
	gint     in_len_ms;           // ADVANCED_XF, FLUSH, FADEIN, PAUSE
	gint     in_volume;           // ADVANCED_XF, FLUSH, FADEIN

	gboolean flush_pause_enable;  // FLUSH
	gint     flush_pause_len_ms;  // FLUSH
	gboolean flush_in_enable;     // FLUSH
	gint     flush_in_len_ms;     // FLUSH
	gint     flush_in_volume;     // FLUSH

	/* additional stuff which is not configureable by the user */
	gboolean flush;               // TRUE for manual, FALSE for xfade config
	guint32  type_mask;           // bitmask for FADE_TYPEs
}
fade_config_t;

typedef struct
{
	gboolean  throttle_enable;
	gboolean  max_write_enable;
	gint      max_write_len;
	gboolean  force_reopen;
}
plugin_config_t;

typedef struct
{
	/* output: method */
	gint output_rate;
	gint output_quality;

	/* output: plugin */
	gchar   *op_config_string;  /* stores configs for all plugins */
	gchar   *op_name;           /* name of the current plugin */

	/* effects */
	gchar   *ep_name;
	gboolean ep_enable;
	gboolean volnorm_enable;
	gboolean volnorm_use_qa;
	gint     volnorm_target;

	/* crossfader */
	gint          mix_size_ms;
	gboolean      mix_size_auto;
	fade_config_t fc[MAX_FADE_CONFIGS];

	/* gap killer */
	gboolean gap_lead_enable;
	gint     gap_lead_len_ms;
	gint     gap_lead_level;

	gboolean gap_trail_enable;
	gint     gap_trail_len_ms;
	gint     gap_trail_level;
	gboolean gap_trail_locked;

	gboolean gap_crossing;

	/* misc */
	gboolean enable_debug;
	gboolean enable_monitor;

	gboolean enable_mixer;
	gboolean mixer_reverse;
	gboolean mixer_software;
	gint     mixer_vol_left;
	gint     mixer_vol_right;

	gint     songchange_timeout;
	gint     preload_size_ms;
	gboolean album_detection;
	gboolean no_xfade_if_same_file;
	gboolean enable_http_workaround;
	gboolean enable_op_max_used;
	gint     op_max_used_ms;
	gboolean output_keep_opened;

	/* presets */
	GList   *presets;

	/* additional stuff which is not configureable by the user */
	gint     sync_size_ms;

	/* additional stuff which is not saved to the config file */
	gint     page;
	gint     xf_index;
}
config_t;


/* some global vars... we should really get rid of those somehow */
extern config_t *config;
extern config_t  config_default;
extern buffer_t *buffer;

extern GStaticMutex buffer_mutex;

extern gboolean opened;         /* XMMS-crossfade is opened */
extern gboolean output_opened;  /* the output plugin is opened */
extern gint     output_offset;
extern gint64   output_streampos;

extern OutputPlugin *the_op;
extern gint          the_rate;

#endif  /* _CROSSFADE_H_ */
