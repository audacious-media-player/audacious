/*
 *  Copyright (c) 2006-2007 William Pitcock <nenolod -at- nenolod.net>
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

#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_X11
# include <gdk/gdkx.h>
#endif

#ifdef GDK_WINDOWING_WIN32
# include <gdk/gdkwin32.h>
#endif

#include <gdk/gdkkeysyms.h>

#ifdef GDK_WINDOWING_QUARTZ
# include <Carbon/Carbon.h>
#endif
