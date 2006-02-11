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

#ifndef __EFFECT_H__
#define __EFFECT_H__

#include "crossfade.h"
#include "format.h"

#include <glib.h>


#define EFFECT_USE_XMMS_PLUGIN GINT_TO_POINTER(-1)

typedef struct
{
	EffectPlugin *ep;
	EffectPlugin *last_ep;
	gboolean      use_xmms_plugin;
	gboolean      is_active;
	gboolean      is_valid;
	format_t      format;
	AFormat       fmt;
	gint          rate;
	gint          nch;
}
effect_context_t;

void effect_init      (effect_context_t *ec,
		       EffectPlugin     *ep);
void effect_set_plugin(effect_context_t *ec,
		       EffectPlugin     *ep);
gint effect_flow      (effect_context_t *ec,
		       gpointer         *buffer,
		       gint              length,
		       format_t         *format,
		       gboolean          allow_format_change);
void effect_free      (effect_context_t *ec);

#endif  /* _EFFECT_H_ */
