/*
 * libmpgdec: An advanced MPEG 1/2/3 decoder engine.
 * interface.c: library interfaces
 *
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net>
 * Portions copyright (C) 1999 Michael Hipp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"

mpgdec_t *global_ins;

mpgdec_t *mpgdec_new(void)
{
	mpgdec_t *ins = g_malloc0(sizeof(mpgdec_t));

	/* yeah i know this is cheating */
	global_ins = ins;
}
