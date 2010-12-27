/*
 * misc.h
 * Copyright 2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_MISC_H
#define AUDACIOUS_MISC_H

#include <glib.h>
#include <audacious/api.h>
#include <audacious/types.h>
#include <libaudcore/vfs.h>
#include <libaudcore/tuple.h>

enum {
 AUDACIOUS_MENU_MAIN,
 AUDACIOUS_MENU_PLAYLIST,
 AUDACIOUS_MENU_PLAYLIST_RCLICK,
 AUDACIOUS_MENU_PLAYLIST_ADD,
 AUDACIOUS_MENU_PLAYLIST_REMOVE,
 AUDACIOUS_MENU_PLAYLIST_SELECT,
 AUDACIOUS_MENU_PLAYLIST_MISC,
 TOTAL_PLUGIN_MENUS};

enum {
 AUD_PATH_BIN_DIR,
 AUD_PATH_DATA_DIR,
 AUD_PATH_PLUGIN_DIR,
 AUD_PATH_LOCALE_DIR,
 AUD_PATH_DESKTOP_FILE,
 AUD_PATH_ICON_FILE,
 AUD_PATH_USER_DIR,
 AUD_PATH_USER_PLUGIN_DIR,
 AUD_PATH_PLAYLISTS_DIR,
 AUD_PATH_PLAYLIST_FILE,
 AUD_PATH_GTKRC_FILE,
 AUD_PATH_COUNT
};

typedef struct {
    gchar * name;
    gfloat preamp, bands[10];
} EqualizerPreset;

typedef gint16 VisFreqData[2][256];
typedef gint16 VisPCMData[2][512];

typedef struct {
    gint time, nch;
    gint length; /* obsolete, always 512 */
    VisPCMData data;
} VisNode;

typedef void (* VisHookFunc) (const VisNode * node, void * user);

#define AUD_API_NAME MiscAPI
#define AUD_API_SYMBOL misc_api

#ifdef _AUDACIOUS_CORE

#include "api-local-begin.h"
#include "misc-api.h"
#include "api-local-end.h"

#define create_widgets(b, w, a) create_widgets_with_domain (b, w, a, PACKAGE)

#else

#include <audacious/api-define-begin.h>
#include <audacious/misc-api.h>
#include <audacious/api-define-end.h>

#include <audacious/api-alias-begin.h>
#include <audacious/misc-api.h>
#include <audacious/api-alias-end.h>

#define aud_create_widgets(b, w, a) aud_create_widgets_with_domain (b, w, a, \
 PACKAGE)

#endif

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif

#ifdef AUD_API_DECLARE

#define AUD_API_NAME MiscAPI
#define AUD_API_SYMBOL misc_api

#include "api-define-begin.h"
#include "misc-api.h"
#include "api-define-end.h"

#include "api-declare-begin.h"
#include "misc-api.h"
#include "api-declare-end.h"

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif
