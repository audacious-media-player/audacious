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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include "effect.h"

void
effect_init(effect_context_t * ec, EffectPlugin * ep)
{
	memset(ec, 0, sizeof(*ec));
	effect_set_plugin(ec, ep);
}

static char *
plugin_name(EffectPlugin * ep)
{
	return ep->description ? ep->description : "<unnamed>";
}


void
effect_set_plugin(effect_context_t * ec, EffectPlugin * ep)
{
	if (ec->use_xmms_plugin && (ep == EFFECT_USE_XMMS_PLUGIN))
		return;
	if (ec->ep == ep)
		return;

	if (ec->last_ep)
	{
		if (ec->last_ep->cleanup)
		{
			DEBUG(("[crossfade] effect: \"%s\" deselected, cleanup\n", plugin_name(ec->last_ep)));
			ec->last_ep->cleanup();
		}
		else
			DEBUG(("[crossfade] effect: \"%s\" deselected\n", plugin_name(ec->last_ep)));
	}

	ec->use_xmms_plugin = (ep == EFFECT_USE_XMMS_PLUGIN);
	ec->ep = ec->use_xmms_plugin ? NULL : ep;
	ec->last_ep = ec->ep;
	ec->is_active = FALSE;

	if (ec->ep)
	{
		if (ec->ep->init)
		{
			DEBUG(("[crossfade] effect: \"%s\" selected, init\n", plugin_name(ec->ep)));
			ec->ep->init();
		}
		else
			DEBUG(("[crossfade] effect: \"%s\" selected\n", plugin_name(ec->ep)));
	}
}

gint
effect_flow(effect_context_t * ec, gpointer * buffer, gint length, format_t * format, gboolean allow_format_change)
{
	EffectPlugin *ep;

	if (ec->use_xmms_plugin ? effects_enabled() : (ec->ep != NULL))
	{
		ep = ec->use_xmms_plugin ? get_current_effect_plugin() : ec->ep;

		if (ep != ec->last_ep)
		{
			DEBUG(("[crossfade] effect: plugin: \"%s\"%s\n",
			       ep ? plugin_name(ep) : "<none>", ec->use_xmms_plugin ? " (XMMS)" : ""));
			ec->last_ep = ep;
			ec->is_active = FALSE;
		}
		if (ep == NULL)
			return length;

		if (ep->query_format)
		{
			AFormat fmt = format->fmt;
			gint rate = format->rate;
			gint nch = format->nch;

			ep->query_format(&fmt, &rate, &nch);
			if (!ec->is_active || (ec->fmt != fmt) || (ec->rate != rate) || (ec->nch != nch))
			{
				if (!allow_format_change && (!format_match(fmt, format->fmt) ||
							     (rate != format->rate) || (nch != format->nch)))
				{
					DEBUG(("[crossfade] effect: format mismatch: "
					       "in=(%s/%d/%d) out=(%s/%d/%d)\n",
					       format_name(format->fmt), format->rate, format->nch,
					       format_name(fmt), rate, nch));
					ec->is_valid = FALSE;
				}
				else
				{
					if (setup_format(fmt, rate, nch, &ec->format) < 0)
					{
						DEBUG(("[crossfade] effect: format not supported "
						       "(fmt=%s rate=%d nch=%d)!\n", format_name(fmt), rate, nch));
						ec->is_valid = FALSE;
					}
					else
					{
						ec->is_valid = TRUE;
						DEBUG(("[crossfade] effect: plugin enabled "
						       "(fmt=%s rate=%d nch=%d)\n", format_name(fmt), rate, nch));
					}
				}

				ec->is_active = TRUE;
				ec->fmt = fmt;
				ec->rate = rate;
				ec->nch = nch;
			}
			if (ec->is_valid && ep->mod_samples)
			{
				length = ep->mod_samples(buffer, length, format->fmt, format->rate, format->nch);
				if (allow_format_change)
					format_copy(format, &ec->format);
			}
		}
		else
		{
			ec->is_active = TRUE;
			if (ep->mod_samples)
				length = ep->mod_samples(buffer, length, format->fmt, format->rate, format->nch);
		}
	}
	else if (ec->is_active)
	{
		ec->is_active = FALSE;
		DEBUG(("[crossfade] effect: plugin disabled\n"));
	}

	return length;
}

void
effect_free(effect_context_t * ec)
{
	/* nothing to do */
}
