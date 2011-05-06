/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2008  Audacious development team.
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

#ifndef AUDACIOUS_UI_JUMPTOTRACK_CACHE_H
#define AUDACIOUS_UI_JUMPTOTRACK_CACHE_H

#include <glib.h>

typedef struct _JumpToTrackCache JumpToTrackCache;

struct _JumpToTrackCache
{
    GHashTable* keywords;
};

extern JumpToTrackCache* ui_jump_to_track_cache_new(void);
extern const GArray * ui_jump_to_track_cache_search (JumpToTrackCache * cache,
 const gchar * keyword);
extern void ui_jump_to_track_cache_free(JumpToTrackCache* cache);

#endif /* AUDACIOUS_UI_JUMPTOTRACK_CACHE_H */
