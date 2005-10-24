/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "hints.h"

#include <glib.h>
#include <gtk/gtk.h>

#include "equalizer.h"
#include "mainwin.h"
#include "playlistwin.h"

#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>

/* flags for the window layer */
typedef enum {
    WIN_LAYER_DESKTOP = 0,
    WIN_LAYER_BELOW = 2,
    WIN_LAYER_NORMAL = 4,
    WIN_LAYER_ONTOP = 6,
    WIN_LAYER_DOCK = 8,
    WIN_LAYER_ABOVE_DOCK = 10
} WinLayer;

#define WIN_STATE_STICKY                (1 << 0)

#define WIN_HINTS_SKIP_WINLIST          (1 << 1)    /* not in win list */
#define WIN_HINTS_SKIP_TASKBAR          (1 << 2)    /* not on taskbar */

#define _NET_WM_STATE_REMOVE   0
#define _NET_WM_STATE_ADD      1
#define _NET_WM_STATE_TOGGLE   2

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8


static void (*set_always_func) (GtkWidget *, gboolean) = NULL;
static void (*set_sticky_func) (GtkWidget *, gboolean) = NULL;
static void (*set_skip_winlist_func) (GtkWidget *) = NULL;
static void (*move_resize_func) (GtkWidget *, gint, gint, gboolean) = NULL;

void
hint_set_skip_winlist(GtkWidget * window)
{
    if (set_skip_winlist_func)
        set_skip_winlist_func(window);
}

void
hint_set_always(gboolean always)
{
    if (set_always_func) {
        set_always_func(mainwin, always);
        set_always_func(equalizerwin, always);
        set_always_func(playlistwin, always);
    }
}

gboolean
hint_always_on_top_available(void)
{
    return !!set_always_func;
}

void
hint_set_sticky(gboolean sticky)
{
    if (sticky) {
        gtk_window_stick(GTK_WINDOW(mainwin));
        gtk_window_stick(GTK_WINDOW(equalizerwin));
        gtk_window_stick(GTK_WINDOW(playlistwin));
    }
    else {
        gtk_window_unstick(GTK_WINDOW(mainwin));
        gtk_window_unstick(GTK_WINDOW(equalizerwin));
        gtk_window_unstick(GTK_WINDOW(playlistwin));
    }
}

gboolean
hint_move_resize_available(void)
{
    return !!move_resize_func;
}

void
hint_move_resize(GtkWidget * window, gint x, gint y, gboolean move)
{
    move_resize_func(window, x, y, move);
}

static gboolean
net_wm_found(void)
{
    Atom r_type, support_check;
    gint r_format, p;
    gulong count, bytes_remain;
    guchar *prop = NULL, *prop2 = NULL;
    gboolean ret = FALSE;

    gdk_error_trap_push();
    support_check =
        XInternAtom(GDK_DISPLAY(), "_NET_SUPPORTING_WM_CHECK", FALSE);

    p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), support_check,
                           0, 1, False, XA_WINDOW, &r_type, &r_format,
                           &count, &bytes_remain, &prop);

    if (p == Success && prop && r_type == XA_WINDOW &&
        r_format == 32 && count == 1) {
        Window n = *(Window *) prop;

        p = XGetWindowProperty(GDK_DISPLAY(), n, support_check, 0, 1,
                               False, XA_WINDOW, &r_type, &r_format,
                               &count, &bytes_remain, &prop2);

        if (p == Success && prop2 && *prop2 == *prop &&
            r_type == XA_WINDOW && r_format == 32 && count == 1)
            ret = TRUE;
    }

    if (prop)
        XFree(prop);
    if (prop2)
        XFree(prop2);
    if (gdk_error_trap_pop())
        return FALSE;
    return ret;
}

static void
net_wm_set_property(GtkWidget * window, gchar * atom, gboolean state)
{
    XEvent xev;
    gint set = _NET_WM_STATE_ADD;
    Atom type, property;

    if (state == FALSE)
        set = _NET_WM_STATE_REMOVE;

    type = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE", FALSE);
    property = XInternAtom(GDK_DISPLAY(), atom, FALSE);


    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
    xev.xclient.message_type = type;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = set;
    xev.xclient.data.l[1] = property;
    xev.xclient.data.l[2] = 0;

    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
               SubstructureNotifyMask, &xev);
}

static void
net_wm_set_desktop(GtkWidget * window, gboolean all)
{
    XEvent xev;
    guint32 current_desktop = 0;

    if (!all) {
        gint r_format, p;
        gulong count, bytes_remain;
        guchar *prop;
        Atom r_type;
        Atom current =
            XInternAtom(GDK_DISPLAY(), "_NET_CURRENT_DESKTOP", FALSE);

        p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), current,
                               0, 1, False, XA_CARDINAL, &r_type,
                               &r_format, &count, &bytes_remain, &prop);

        if (p == Success && prop && r_type == XA_CARDINAL &&
            r_format == 32 && count == 1) {
            current_desktop = *(long *) prop;
            XFree(prop);
        }
    }
    else
        current_desktop = 0xffffffff;

    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
    xev.xclient.message_type =
        XInternAtom(GDK_DISPLAY(), "_NET_WM_DESKTOP", FALSE);
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = current_desktop;

    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
               SubstructureNotifyMask, &xev);
}



static void
net_wm_set_window_always(GtkWidget * window, gboolean always)
{
    net_wm_set_property(window, "_NET_WM_STATE_STAYS_ON_TOP", always);
}

static void
net_wm_set_window_above(GtkWidget * window, gboolean always)
{
    net_wm_set_property(window, "_NET_WM_STATE_ABOVE", always);
}

static void
net_wm_move_resize(GtkWidget * window, gint x, gint y, gboolean move)
{
    XEvent xev;
    gint dir;
    Atom type;

    if (move)
        dir = _NET_WM_MOVERESIZE_MOVE;
    else
        dir = _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT;

    gdk_pointer_ungrab(GDK_CURRENT_TIME);

    type = XInternAtom(GDK_DISPLAY(), "_NET_WM_MOVERESIZE", FALSE);

    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = GDK_WINDOW_XWINDOW(window->window);
    xev.xclient.message_type = type;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = x;
    xev.xclient.data.l[1] = y;
    xev.xclient.data.l[2] = dir;
    xev.xclient.data.l[3] = 1;  /* button */


    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
               SubstructureNotifyMask, &xev);
}

static gboolean
find_atom(Atom * atoms, gint n, const gchar * name)
{
    Atom a = XInternAtom(GDK_DISPLAY(), name, FALSE);
    gint i;

    for (i = 0; i < n; i++)
        if (a == atoms[i])
            return TRUE;
    return FALSE;
}

static gboolean
get_supported_atoms(Atom ** atoms, gulong * natoms, const gchar * name)
{
    Atom supported = XInternAtom(GDK_DISPLAY(), name, FALSE), r_type;
    gulong bremain;
    gint r_format, p;

    *atoms = NULL;
    gdk_error_trap_push();
    p = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), supported,
                           0, 1000, False, XA_ATOM, &r_type, &r_format,
                           natoms, &bremain, (guchar **) atoms);
    if (gdk_error_trap_pop() || p != Success || r_type != XA_ATOM ||
        *natoms == 0 || *atoms == NULL)
        return FALSE;

    return TRUE;
}

static void
net_wm_check_features(void)
{
    Atom *atoms;
    gulong n_atoms;

    if (!get_supported_atoms(&atoms, &n_atoms, "_NET_SUPPORTED"))
        return;

    if (find_atom(atoms, n_atoms, "_NET_WM_STATE")) {
        if (!set_always_func &&
            find_atom(atoms, n_atoms, "_NET_WM_STATE_ABOVE"))
            set_always_func = net_wm_set_window_above;
        if (!set_always_func &&
            find_atom(atoms, n_atoms, "_NET_WM_STATE_STAYS_ON_TOP"))
            set_always_func = net_wm_set_window_always;
        if (!set_sticky_func && find_atom(atoms, n_atoms, "_NET_WM_DESKTOP"))
            set_sticky_func = net_wm_set_desktop;
    }

    if (find_atom(atoms, n_atoms, "_NET_WM_MOVERESIZE"))
        move_resize_func = net_wm_move_resize;

    XFree(atoms);
}

void
check_wm_hints(void)
{
    if (net_wm_found()) {
        g_message("found NET_WM");
        net_wm_check_features();
    }

}
