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

#ifdef _AUDACIOUS_CORE
#  include "config.h"
#endif

#ifndef AUDACIOUS_MAIN_H
#define AUDACIOUS_MAIN_H

#define NOT_ALPHA_RELEASE

#ifdef _AUDACIOUS_CORE
# ifdef USE_DBUS
#  include "dbus-service.h"
# endif
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "audconfig.h"

/* Read, write, execute/search by group. */
#ifndef S_IRWXG
#define S_IRWXG     (S_IRGRP | S_IWGRP | S_IXGRP)
#endif

/* Read permission, group. */
#ifndef S_IRGRP
#define S_IRGRP     S_IRUSR
#endif

/* Write permission, group. */
#ifndef S_IWGRP
#define S_IWGRP     S_IWUSR
#endif

/* Execute/search permission, group. */
#ifndef S_IXGRP
#define S_IXGRP     S_IXUSR
#endif

/* Read, write, execute/search by others. */
#ifndef S_IRWXO
#define S_IRWXO     (S_IROTH | S_IWOTH | S_IXOTH)
#endif

/* Read permission, others. */
#ifndef S_IROTH
#define S_IROTH     S_IRUSR
#endif

/* Write permission, others. */
#ifndef S_IWOTH
#define S_IWOTH     S_IWUSR
#endif

/* Execute/search permission, others. */
#ifndef S_IXOTH
#define S_IXOTH     S_IXUSR
#endif

G_BEGIN_DECLS

/* macro for debug print */
#ifdef DEBUG
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

#if defined(USE_DBUS) && defined(_AUDACIOUS_CORE)
extern MprisPlayer *mpris;
#endif

void aud_quit(void);

G_END_DECLS

#endif /* AUDACIOUS_MAIN_H */
