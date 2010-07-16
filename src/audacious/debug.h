/*
 * debug.h
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

#ifndef AUDACIOUS_DEBUG_H
#define AUDACIOUS_DEBUG_H

#include <stdio.h>

#ifdef DEBUG
#define AUDDBG(...) do {printf ("%s:%d %s(): ", __FILE__, __LINE__, __FUNCTION__); printf (__VA_ARGS__);} while (0)
#else
#define AUDDBG(...) do {} while(0)
#endif

#endif
