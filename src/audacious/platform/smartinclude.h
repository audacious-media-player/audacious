/*
 *  Copyright (c) 2006-2007 William Pitcock <nenolod -at- nenolod.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_X11

# include <gdk/gdkx.h>
# include <X11/Xlib.h>
# include <X11/Xatom.h>

#endif

#ifdef GDK_WINDOWING_WIN32

# include <gdk/gdkwin32.h>

#endif

#include <gdk/gdkkeysyms.h>
