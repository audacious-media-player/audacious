/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2007 Giacomo Lozito
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/* audacious_drct_* provides a handy interface for player
   plugins, originally intended for migration from xmms_remote_* calls */


#include "main.h"
#include "input.h"
#include "playback.h"
#include "ui_main.h"
#include "ui_playlist.h"
#include "ui_equalizer.h"


/* player */

void
audacious_drct_quit ( void )
{
  mainwin_quit_cb();
  return;
}

void
audacious_drct_eject ( void )
{
  if (has_x11_connection)
    mainwin_eject_pushed();
  return;
}

gboolean
audacious_drct_main_win_is_visible ( void )
{
  return cfg.player_visible;
}

void
audacious_drct_main_win_show ( gboolean show )
{
  if (has_x11_connection)
    mainwin_show(show);
  return;
}

gboolean
audacious_drct_equalizer_is_visible ( void )
{
  return cfg.equalizer_visible;
}

void
audacious_drct_equalizer_show ( gboolean show )
{
  if (has_x11_connection)
    equalizerwin_show(show);
  return;
}

gboolean
audacious_drct_playlist_is_visible ( void )
{
  return cfg.playlist_visible;
}

void
audacious_drct_playlist_show ( gboolean show )
{
  if (has_x11_connection) {
    if (show)
      playlistwin_show();
    else
      playlistwin_hide();
  }
  return;
}


/* playback */

void
audacious_drct_play ( void )
{
  if (playback_get_paused())
    playback_pause();
  else if (playlist_get_length(playlist_get_active()))
    playback_initiate();
  else
    mainwin_eject_pushed();
  return;
}

void
audacious_drct_pause ( void )
{
  playback_pause();
  return;
}

void
audacious_drct_stop ( void )
{
  ip_data.stop = TRUE;
  playback_stop();
  ip_data.stop = FALSE;
  mainwin_clear_song_info();
  return;
}

gboolean
audacious_drct_get_playing ( void )
{
  return playback_get_playing();
}

gboolean
audacious_drct_get_paused ( void )
{
  return playback_get_paused();
}

gboolean
audacious_drct_get_stopped ( void )
{
  return !playback_get_playing();
}

gint
audacious_drct_get_time ( void )
{
  gint time;
  if (playback_get_playing())
    time = playback_get_time();
  else
    time = 0;
  return time;
}

void
audacious_drct_seek ( guint pos )
{
  if (playlist_get_current_length(playlist_get_active()) > 0 &&
      pos < (guint)playlist_get_current_length(playlist_get_active()))
    playback_seek(pos / 1000);
  return;
}

void
audacious_drct_get_volume( gint *vl, gint *vr )
{
  input_get_volume(vl, vr);
  return;
}

void
audacious_drct_set_volume( gint vl, gint vr )
{
  if (vl > 100)
    vl = 100;
  if (vr > 100)
    vr = 100;
  input_set_volume(vl, vr);
  return;
}


/* playlist */

void
audacious_drct_pl_next ( void )
{
  playlist_next(playlist_get_active());
  return;
}

void
audacious_drct_pl_prev ( void )
{
  playlist_prev(playlist_get_active());
  return;
}
