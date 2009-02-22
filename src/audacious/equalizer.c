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

gfloat
equalizer_get_preamp(void)
{
    return cfg.equalizer_preamp;
}

void
equalizer_set_preamp(gfloat preamp)
{
    cfg.equalizer_preamp = preamp;
    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);
}

gfloat
equalizer_get_band(gint band)
{
    return cfg.equalizer_bands[band];
}

void
equalizer_set_band(gint band, gfloat value)
{
    cfg.equalizer_bands[band] = value;
    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);
}

gboolean equalizer_get_active(gboolean active)
{
    return cfg.equalizer_active;
}

void
equalizer_set_active(gboolean active)
{
    cfg.equalizer_active = active;
    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);
}
