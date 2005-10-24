/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Zinx Verituse
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
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
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "fullscreen.h"

#include "libaudacious/util.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#ifndef XF86VIDMODE

gboolean
xmms_fullscreen_available(Display * dpy)
{
    return FALSE;
}

gboolean
xmms_fullscreen_init(GtkWidget * win)
{
    return FALSE;
}

gboolean
xmms_fullscreen_enter(GtkWidget * win, gint * w, gint * h)
{
    return FALSE;
}

void
xmms_fullscreen_leave(GtkWidget * win)
{
    return;
}

gboolean
xmms_fullscreen_in(GtkWidget * win)
{
    return FALSE;
}

gboolean
xmms_fullscreen_mark(GtkWidget * win)
{
    return FALSE;
}

void
xmms_fullscreen_unmark(GtkWidget * win)
{
    return;
}

void
xmms_fullscreen_cleanup(GtkWidget * win)
{
    return;
}

GSList *
xmms_fullscreen_modelist(GtkWidget * win)
{
    return NULL;
}

void
xmms_fullscreen_modelist_free(GSList * modes)
{
    return;
}

#else                           /* XF86VIDMODE */

#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/xf86vmstr.h>

gboolean
xmms_fullscreen_available(Display * dpy)
{
    int event_base, error_base, num_modes;
    XF86VidModeModeInfo **dummy;

    if (!XF86VidModeQueryExtension(dpy, &event_base, &error_base))
        return FALSE;

    XF86VidModeGetAllModeLines(dpy, DefaultScreen(dpy), &num_modes, &dummy);
    XFree(dummy);

    return (num_modes > 1);
}

typedef struct {
    Display *display;
    XF86VidModeModeInfo **modes, *origmode;
    gboolean is_full, can_full;
    gint num_modes;
} fullscreen_display_t;

static fullscreen_display_t **displays = NULL;

typedef struct {
    GtkWidget *window;
    gint is_full;
    gint ox, oy, owidth, oheight;
    fullscreen_display_t *display;
} fullscreen_window_t;
static fullscreen_window_t **windows = NULL;

G_LOCK_DEFINE_STATIC(full_mutex);

#define FULL_LOCK() G_LOCK(full_mutex);
#define FULL_UNLOCK() G_UNLOCK(full_mutex);

static fullscreen_display_t *
getdisplay(Display * dpy)
{
    gint i;

    if (displays) {
        for (i = 0; displays[i]; i++) {
            if (displays[i]->display == dpy)
                return displays[i];
        }
        displays = g_realloc(displays, sizeof(*displays) * (i + 2));
    }
    else {
        displays = g_malloc(sizeof(*displays) * 2);
        i = 0;
    }
    displays[i + 1] = NULL;
    displays[i] = g_malloc(sizeof(**displays));
    displays[i]->display = dpy;
    displays[i]->modes = NULL;
    displays[i]->origmode = NULL;
    displays[i]->num_modes = 0;
    displays[i]->is_full = FALSE;
    displays[i]->can_full = FALSE;
    return displays[i];
}

static fullscreen_window_t *
getwindow(GtkWidget * win)
{
    gint i;

    if (windows) {
        for (i = 0; windows[i]; i++) {
            if (windows[i]->window == win)
                return windows[i];
        }
        windows = g_realloc(windows, sizeof(*windows) * (i + 2));
    }
    else {
        windows = g_malloc(sizeof(*windows) * 2);
        i = 0;
    }
    windows[i + 1] = NULL;
    windows[i] = g_malloc(sizeof(**windows));
    windows[i]->window = win;
    windows[i]->ox = 0;
    windows[i]->oy = 0;
    windows[i]->owidth = 0;
    windows[i]->oheight = 0;
    windows[i]->display = getdisplay(GDK_WINDOW_XDISPLAY(win->window));
    windows[i]->is_full = 0;
    return windows[i];
}

gboolean
xmms_fullscreen_init(GtkWidget * win)
{
    int event_base, error_base, dummy;
    fullscreen_window_t *fwin;
    gint i;
    XF86VidModeModeLine origmode;

    FULL_LOCK();
    fwin = getwindow(win);

    if (!XF86VidModeQueryExtension
        (fwin->display->display, &event_base, &error_base)) {
        FULL_UNLOCK();
        return FALSE;
    }

    if (!fwin->display->modes) {
        XF86VidModeGetAllModeLines(fwin->display->display,
                                   DefaultScreen(fwin->display->display),
                                   &fwin->display->num_modes,
                                   &fwin->display->modes);

        if (!fwin->display->origmode) {
            XF86VidModeGetModeLine(fwin->display->display,
                                   DefaultScreen(fwin->display->display),
                                   &dummy, &origmode);
            for (i = 0; i < fwin->display->num_modes; i++) {
                if (fwin->display->modes[i]->hdisplay ==
                    origmode.hdisplay
                    && fwin->display->modes[i]->vdisplay ==
                    origmode.vdisplay) {
                    fwin->display->origmode = fwin->display->modes[i];
                    break;
                }
            }

            if (!fwin->display->origmode) {
                fprintf(stderr,
                        "ERROR: Could not determine original mode.\n");
                FULL_UNLOCK();
                return FALSE;
            }

        }

        fwin->display->can_full = (fwin->display->num_modes > 1);
    }
    FULL_UNLOCK();
    return fwin->display->can_full;
}

gboolean
xmms_fullscreen_enter(GtkWidget * win, gint * w, gint * h)
{
    gint i, close, how_close = -1, t, dummy;
    gboolean retval = FALSE;
    fullscreen_window_t *fwin;

    FULL_LOCK();
    fwin = getwindow(win);

    if (!fwin->display->is_full && !fwin->is_full && fwin->display->can_full) {
        for (close = 0; close < fwin->display->num_modes; close++) {
            if ((fwin->display->modes[close]->hdisplay >= *w) &&
                (fwin->display->modes[close]->vdisplay >= *h)) {
                how_close = fwin->display->modes[close]->hdisplay - *w;
                break;
            }
        }

        for (i = close + 1; i < fwin->display->num_modes; i++) {
            if (fwin->display->modes[i]->vdisplay < *h)
                continue;
            t = fwin->display->modes[i]->hdisplay - *w;
            if (t >= 0 && t < how_close) {
                close = i;
                how_close = t;
            }
        }

        if (close < fwin->display->num_modes) {
            *w = fwin->display->modes[close]->hdisplay;
            *h = fwin->display->modes[close]->vdisplay;

            /* Save the old position/size */
            gdk_window_get_root_origin(fwin->window->window, &fwin->ox,
                                       &fwin->oy);
            gdk_window_get_size(fwin->window->window, &fwin->owidth,
                                &fwin->oheight);

            /* Move it. */
            gdk_window_move_resize(fwin->window->window, 0, 0,
                                   fwin->display->modes[close]->hdisplay,
                                   fwin->display->modes[close]->vdisplay);

            /* Tell the WM not to mess with this window (no more decor) */
            gdk_window_hide(fwin->window->window);
            gdk_window_set_override_redirect(fwin->window->window, TRUE);
            gdk_window_show(fwin->window->window);

            /*
             * XXX: HACK
             * Something is ungrabbing the pointer shortly
             * after the above unmap/override_redirect=TRUE/map
             * is done.  I don't know what at this time, only
             * that it's not XMMS, and that it's very very evil.
             */
            gdk_flush();
            xmms_usleep(50000);

            /* Steal the keyboard/mouse */
            /* XXX: FIXME, use timeouts.. */
            for (t = 0; t < 10; t++) {
                dummy = gdk_pointer_grab(fwin->window->window,
                                         TRUE, 0,
                                         fwin->window->window,
                                         NULL, GDK_CURRENT_TIME);

                if (dummy == GrabSuccess)
                    break;

                gtk_main_iteration_do(FALSE);
                xmms_usleep(10000);
            }
            gdk_keyboard_grab(fwin->window->window, TRUE, GDK_CURRENT_TIME);

            /* Do the video mode switch.. */
            XF86VidModeSwitchToMode(fwin->display->display,
                                    DefaultScreen(fwin->display->display),
                                    fwin->display->modes[close]);

            XF86VidModeSetViewPort(fwin->display->display,
                                   DefaultScreen(fwin->display->display),
                                   0, 0);

            retval = TRUE;

            fwin->is_full = TRUE;
            fwin->display->is_full = TRUE;
        }
    }

    FULL_UNLOCK();

    return retval;
}

void
xmms_fullscreen_leave(GtkWidget * win)
{
    fullscreen_window_t *fwin;

    FULL_LOCK();
    fwin = getwindow(win);

    if (fwin->is_full && fwin->display->is_full) {
        /* Release our grabs */
        gdk_pointer_ungrab(GDK_CURRENT_TIME);
        gdk_keyboard_ungrab(GDK_CURRENT_TIME);

        /* Let the WM manage this window again */
        gdk_window_hide(fwin->window->window);
        gdk_window_set_override_redirect(fwin->window->window, FALSE);
        gdk_window_show(fwin->window->window);

        /* Restore size/position */
        gdk_window_move_resize(fwin->window->window, fwin->ox, fwin->oy,
                               fwin->owidth, fwin->oheight);

        XF86VidModeSwitchToMode(fwin->display->display,
                                DefaultScreen(fwin->display->display),
                                fwin->display->origmode);
        fwin->display->is_full = FALSE;
    }
    fwin->is_full = FALSE;
    FULL_UNLOCK();
}

gboolean
xmms_fullscreen_in(GtkWidget * win)
{
    fullscreen_window_t *fwin;

    FULL_LOCK();
    fwin = getwindow(win);
    FULL_UNLOCK();

    if (fwin->display->is_full)
        return TRUE;
    else
        return FALSE;
}

gboolean
xmms_fullscreen_mark(GtkWidget * win)
{
    fullscreen_window_t *fwin;

    FULL_LOCK();
    fwin = getwindow(win);

    if (fwin->display->is_full) {
        FULL_UNLOCK();
        return FALSE;
    }
    else {
        fwin->is_full = TRUE;
        fwin->display->is_full = TRUE;
        FULL_UNLOCK();
        return TRUE;
    }
}

void
xmms_fullscreen_unmark(GtkWidget * win)
{
    fullscreen_window_t *fwin;

    FULL_LOCK();
    fwin = getwindow(win);

    if (fwin->is_full) {
        fwin->is_full = FALSE;
        fwin->display->is_full = FALSE;
    }
    FULL_UNLOCK();
}

void
xmms_fullscreen_cleanup(GtkWidget * win)
{
    gint i, j;
    fullscreen_display_t *display;

    FULL_LOCK();
    if (!windows)
        goto unlock_return;

    for (i = 0; windows[i]; i++) {
        if (windows[i]->window == win) {
            display = windows[i]->display;
            for (j = i + 1; windows[j]; j++);
            windows[i] = windows[j - 1];
            windows = g_realloc(windows, sizeof(*windows) * (j + 1));
            windows[j] = NULL;

            for (i = 0; windows[i]; i++) {
                if (windows[i]->display == display)
                    goto unlock_return;
            }
            /* bugger all, kill the display */
            for (i = 0; displays[i]; i++) {
                if (displays[i] == display) {
                    XFree(displays[i]->modes);
                    for (j = i + 1; displays[j]; j++);
                    displays[i] = displays[j - 1];
                    displays =
                        g_realloc(displays, sizeof(*displays) * (j + 1));
                    displays[j] = NULL;
                    break;
                }
            }
        }
    }
  unlock_return:
    FULL_UNLOCK();
}

GSList *
xmms_fullscreen_modelist(GtkWidget * win)
{
    fullscreen_window_t *fwin;
    xmms_fullscreen_mode_t *ent;
    GSList *retlist = NULL;
    int i;

    FULL_LOCK();
    fwin = getwindow(win);

    for (i = 0; i < fwin->display->num_modes; i++) {
        ent = g_malloc(sizeof(*ent));
        ent->width = fwin->display->modes[i]->hdisplay;
        ent->height = fwin->display->modes[i]->vdisplay;
        retlist = g_slist_append(retlist, ent);
    }
    FULL_UNLOCK();

    return retlist;
}

void
xmms_fullscreen_modelist_free(GSList * modes)
{
    g_slist_foreach(modes, (GFunc) g_free_func, NULL);
    g_slist_free(modes);
}

#endif                          /* XF86VIDMODE */
