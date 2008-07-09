/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team.
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

#include "equalizer.h"

#include "audconfig.h"
#include "legacy/ui_equalizer.h"

gfloat
equalizer_get_preamp(void)
{
    return equalizerwin_get_preamp();
}

void
equalizer_set_preamp(gfloat preamp)
{
    equalizerwin_set_preamp(preamp);
}

gfloat
equalizer_get_band(gint band)
{
    return equalizerwin_get_band(band);
}

void
equalizer_set_band(gint band, gfloat value)
{
    equalizerwin_set_band(band, value);
}

gboolean equalizer_get_active(gboolean active)
{
    return cfg.equalizer_active;
}

void
equalizer_set_active(gboolean active)
{
    equalizerwin_activate(active);
}
