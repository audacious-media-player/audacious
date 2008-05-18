/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef AUDACIOUS_MAIN_H
#define AUDACIOUS_MAIN_H

#ifdef _AUDACIOUS_CORE
# include "ui_main.h"
# ifdef USE_DBUS
#  include "dbus-service.h"
# endif
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "audconfig.h"

G_BEGIN_DECLS

#define PLAYER_HEIGHT \
  ((cfg.player_shaded ? MAINWIN_SHADED_HEIGHT : MAINWIN_HEIGHT) * (cfg.scaled ? cfg.scale_factor : 1))
#define PLAYER_WIDTH \
  (MAINWIN_WIDTH * (cfg.scaled ? cfg.scale_factor : 1))

/* macro for debug print */
#ifdef AUD_DEBUG
#  define AUDDBG(...) do { g_print("%s:%d %s(): ", __FILE__, (int)__LINE__, __FUNCTION__); g_print(__VA_ARGS__); } while (0)
#else
#  define AUDDBG(...) do { } while (0)
#endif


enum {
    VOLSET_STARTUP,
    VOLSET_UPDATE,
    VOLUME_ADJUSTED,
    VOLUME_SET
};

enum {
    BMP_PATH_LOG_FILE,
    BMP_PATH_USER_DIR,
    BMP_PATH_USER_PLUGIN_DIR,
    BMP_PATH_USER_SKIN_DIR,
    BMP_PATH_SKIN_THUMB_DIR,
    BMP_PATH_PLAYLISTS_DIR,
    BMP_PATH_ACCEL_FILE,
    BMP_PATH_CONFIG_FILE,
    BMP_PATH_PLAYLIST_FILE,
    BMP_PATH_GTKRC_FILE,
    BMP_PATH_COUNT
};

extern gchar *aud_paths[];

extern GCond *cond_scan;
extern GMutex *mutex_scan;
#if defined(USE_DBUS) && defined(_AUDACIOUS_CORE)
extern MprisPlayer *mpris;
#endif

void aud_quit(void);

G_END_DECLS

#endif /* AUDACIOUS_MAIN_H */
