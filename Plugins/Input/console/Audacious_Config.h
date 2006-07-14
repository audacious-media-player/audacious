/*
 * Audacious: Cross platform multimedia player
 * Copyright (c) 2005  Audacious Team
 *
 * Driver for Game_Music_Emu library. See details at:
 * http://www.slack.net/~ant/libs/
 */


#ifndef CONSOLE_AUDACIOUS_CONFIG_H
#define CONSOLE_AUDACIOUS_CONFIG_H 1

#include <glib.h>


typedef struct {
	gint loop_length;   // length of tracks that lack timing information
	gboolean resample;  // whether or not to resample
	gint resample_rate; // rate to resample at
	gboolean nsfe_playlist; // if true, use optional NSFE playlist
	gint treble; // -100 to +100
	gint bass;   // -100 to +100
	gboolean ignore_spc_length; // if true, ignore length from SPC tags
}
AudaciousConsoleConfig;

extern AudaciousConsoleConfig audcfg;

extern "C" {
  void console_cfg_load( void );
  void console_cfg_save( void );
  void console_cfg_ui( void );
}

#endif /* !CONSOLE_AUDACIOUS_CONFIG_H */
