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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "widgetcore.h"

#include <glib.h>

#include "audacious/main.h"
#include "skin.h"

void
init_spline(gfloat * x, gfloat * y, gint n, gfloat * y2)
{
    gint i, k;
    gfloat p, qn, sig, un, *u;

    u = (gfloat *) g_malloc(n * sizeof(gfloat));

    y2[0] = u[0] = 0.0;

    for (i = 1; i < n - 1; i++) {
        sig = ((gfloat) x[i] - x[i - 1]) / ((gfloat) x[i + 1] - x[i - 1]);
        p = sig * y2[i - 1] + 2.0;
        y2[i] = (sig - 1.0) / p;
        u[i] =
            (((gfloat) y[i + 1] - y[i]) / (x[i + 1] - x[i])) -
            (((gfloat) y[i] - y[i - 1]) / (x[i] - x[i - 1]));
        u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
    }
    qn = un = 0.0;

    y2[n - 1] = (un - qn * u[n - 2]) / (qn * y2[n - 2] + 1.0);
    for (k = n - 2; k >= 0; k--)
        y2[k] = y2[k] * y2[k + 1] + u[k];
    g_free(u);
}

gfloat
eval_spline(gfloat xa[], gfloat ya[], gfloat y2a[], gint n, gfloat x)
{
    gint klo, khi, k;
    gfloat h, b, a;

    klo = 0;
    khi = n - 1;
    while (khi - klo > 1) {
        k = (khi + klo) >> 1;
        if (xa[k] > x)
            khi = k;
        else
            klo = k;
    }
    h = xa[khi] - xa[klo];
    a = (xa[khi] - x) / h;
    b = (x - xa[klo]) / h;
    return (a * ya[klo] + b * ya[khi] +
            ((a * a * a - a) * y2a[klo] +
             (b * b * b - b) * y2a[khi]) * (h * h) / 6.0);
}

void
eqgraph_draw(Widget * w)
{
    EqGraph *eg = (EqGraph *) w;
    GdkPixmap *obj;
    GdkColor col;
    guint32 cols[19];
    gint i, y, ymin, ymax, py = 0;
    gfloat x[] = { 0, 11, 23, 35, 47, 59, 71, 83, 97, 109 }, yf[10];

    /*
     * This avoids the init_spline() function to be inlined.
     * Inlining the function caused troubles when compiling with
     * `-O' (at least on FreeBSD).
     */
    void (*__init_spline) (gfloat *, gfloat *, gint, gfloat *) = init_spline;

    obj = eg->eg_widget.parent;
    skin_draw_pixmap(bmp_active_skin, obj, eg->eg_widget.gc, SKIN_EQMAIN,
                     0, 294, eg->eg_widget.x, eg->eg_widget.y,
                     eg->eg_widget.width, eg->eg_widget.height);
    skin_draw_pixmap(bmp_active_skin, obj, eg->eg_widget.gc, SKIN_EQMAIN,
                     0, 314, eg->eg_widget.x,
                     eg->eg_widget.y + 9 +
                     ((cfg.equalizer_preamp * 9) / 20),
                     eg->eg_widget.width, 1);

    skin_get_eq_spline_colors(bmp_active_skin, cols);

    __init_spline(x, cfg.equalizer_bands, 10, yf);
    for (i = 0; i < 109; i++) {
        y = 9 -
            (gint) ((eval_spline(x, cfg.equalizer_bands, yf, 10, i) *
                     9.0) / 20.0);
        if (y < 0)
            y = 0;
        if (y > 18)
            y = 18;
        if (!i)
            py = y;
        if (y < py) {
            ymin = y;
            ymax = py;
        }
        else {
            ymin = py;
            ymax = y;
        }
        py = y;
        for (y = ymin; y <= ymax; y++) {
            col.pixel = cols[y];
            gdk_gc_set_foreground(eg->eg_widget.gc, &col);
            gdk_draw_point(obj, eg->eg_widget.gc, eg->eg_widget.x + i + 2,
                           eg->eg_widget.y + y);
        }
    }
}

EqGraph *
create_eqgraph(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y)
{
    EqGraph *eg;

    eg = g_new0(EqGraph, 1);
    widget_init(&eg->eg_widget, parent, gc, x, y, 113, 19, TRUE);
    eg->eg_widget.draw = eqgraph_draw;

    widget_list_add(wlist, WIDGET(eg));

    return eg;
}
