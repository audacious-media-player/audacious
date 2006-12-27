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
#include <gdk/gdk.h>
#include <string.h>

#include "main.h"
#include "skin.h"
#include "widget.h"

static const gfloat vis_afalloff_speeds[] = { 0.34, 0.5, 1.0, 1.3, 1.6 };
static const gfloat vis_pfalloff_speeds[] = { 1.2, 1.3, 1.4, 1.5, 1.6 };
static const gint vis_redraw_delays[] = { 1, 2, 4, 8 };
static const guint8 vis_scope_colors[] =
    { 21, 21, 20, 20, 19, 19, 18, 19, 19, 20, 20, 21, 21 };
//static guint8 vs_data_ext[76 * 16 * 4];
static guchar voiceprint_data[76*16];
//static guchar voiceprint_data_normal[76*16];
//static guchar voiceprint_data_rgb[76*16*4];

void
vis_timeout_func(Vis * vis, guchar * data)
{
    static GTimer *timer = NULL;
    gulong micros = 9999999;
    gboolean falloff = FALSE;
    gint i, n;

    if (!timer) {
        timer = g_timer_new();
        g_timer_start(timer);
    }
    else {
      g_timer_elapsed(timer, &micros);
      if (micros > 14000)
	g_timer_reset(timer);
    }
    if (cfg.vis_type == VIS_ANALYZER) {
        if (micros > 14000)
            falloff = TRUE;
        if (data || falloff) {
            for (i = 0; i < 75; i++) {
                if (data && data[i] > vis->vs_data[i]) {
                    vis->vs_data[i] = data[i];
                    if (vis->vs_data[i] > vis->vs_peak[i]) {
                        vis->vs_peak[i] = vis->vs_data[i];
                        vis->vs_peak_speed[i] = 0.01;

                    }
                    else if (vis->vs_peak[i] > 0.0) {
                        vis->vs_peak[i] -= vis->vs_peak_speed[i];
                        vis->vs_peak_speed[i] *=
                            vis_pfalloff_speeds[cfg.peaks_falloff];
                        if (vis->vs_peak[i] < vis->vs_data[i])
                            vis->vs_peak[i] = vis->vs_data[i];
                        if (vis->vs_peak[i] < 0.0)
                            vis->vs_peak[i] = 0.0;
                    }
                }
                else if (falloff) {
                    if (vis->vs_data[i] > 0.0) {
                        vis->vs_data[i] -=
                            vis_afalloff_speeds[cfg.analyzer_falloff];
                        if (vis->vs_data[i] < 0.0)
                            vis->vs_data[i] = 0.0;
                    }
                    if (vis->vs_peak[i] > 0.0) {
                        vis->vs_peak[i] -= vis->vs_peak_speed[i];
                        vis->vs_peak_speed[i] *=
                            vis_pfalloff_speeds[cfg.peaks_falloff];
                        if (vis->vs_peak[i] < vis->vs_data[i])
                            vis->vs_peak[i] = vis->vs_data[i];
                        if (vis->vs_peak[i] < 0.0)
                            vis->vs_peak[i] = 0.0;
                    }
                }
            }
        }
    }
    else if (cfg.vis_type == VIS_VOICEPRINT && data){
      for(i = 0; i < 16; i++)
	{
	  vis->vs_data[i] = data[15 - i];
	}
    }
    else if (data) {
        for (i = 0; i < 75; i++)
            vis->vs_data[i] = data[i];
    }

    if (micros > 14000) {
        if (!vis->vs_refresh_delay) {
            vis_draw((Widget *) vis);
            vis->vs_refresh_delay = vis_redraw_delays[cfg.vis_refresh];

        }
        vis->vs_refresh_delay--;
    }
}
void
vis_draw(Widget * w)
{
    Vis *vis = (Vis *) w;
    gint x, y, n, h = 0, h2;
    guchar vis_color[24][3];
    guchar rgb_data[76 * 16 * 3 * 2 * 2], *ptr, c;
    guint32 colors[24];
    GdkRgbCmap *cmap;

    if (!vis->vs_widget.visible)
        return;

    skin_get_viscolor(bmp_active_skin, vis_color);
    for (y = 0; y < 24; y++) {
        colors[y] =
            vis_color[y][0] << 16 | vis_color[y][1] << 8 | vis_color[y][2];
    }
    cmap = gdk_rgb_cmap_new(colors, 24);

    if (!vis->vs_doublesize) {
      if(cfg.vis_type == VIS_VOICEPRINT && cfg.voiceprint_mode != VOICEPRINT_NORMAL){
	memset(rgb_data, 0, 76 * 16 * 3);
      }
      else{
	memset(rgb_data, 0, 76 * 16);
	for (y = 1; y < 16; y += 2) {
	  ptr = rgb_data + (y * 76);
	  for (x = 0; x < 76; x += 2, ptr += 2)
	    *ptr = 1;
      }
      }
    }
    else{
      if(cfg.vis_type == VIS_VOICEPRINT && cfg.voiceprint_mode != VOICEPRINT_NORMAL){
	memset(rgb_data, 0, 3 * 4 * 16 * 76);
      }
      else{
	memset(rgb_data, 0, 152 * 32);
	for (y = 1; y < 16; y += 2) {
	  ptr = rgb_data + (y * 304);
	  for (x = 0; x < 76; x += 2, ptr += 4) {
	    *ptr = 1;
	    *(ptr + 1) = 1;
	    *(ptr + 152) = 1;
	    *(ptr + 153) = 1;
	}
      }
      }
    }
    if (cfg.vis_type == VIS_ANALYZER) {
      for (x = 0; x < 75; x++) {
	if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
	  h = vis->vs_data[x >> 2];
	else if (cfg.analyzer_type == ANALYZER_LINES)
	  h = vis->vs_data[x];
	if (h && (cfg.analyzer_type == ANALYZER_LINES ||
		  (x % 4) != 3)) {
	  if (!vis->vs_doublesize) {
	    ptr = rgb_data + ((16 - h) * 76) + x;
	    switch (cfg.analyzer_mode) {
	    case ANALYZER_NORMAL:
	      for (y = 0; y < h; y++, ptr += 76)
		*ptr = 18 - h + y;
	      break;
	    case ANALYZER_FIRE:
	      for (y = 0; y < h; y++, ptr += 76)
		*ptr = y + 2;
	      break;
	    case ANALYZER_VLINES:
	      for (y = 0; y < h; y++, ptr += 76)
		*ptr = 18 - h;
	      break;
	    }
	  }
	  else{
	    ptr = rgb_data + ((16 - h) * 304) + (x << 1);
	    switch (cfg.analyzer_mode) {
	    case ANALYZER_NORMAL:
	      for (y = 0; y < h; y++, ptr += 304) {
		*ptr = 18 - h + y;
		*(ptr + 1) = 18 - h + y;
		*(ptr + 152) = 18 - h + y;
		*(ptr + 153) = 18 - h + y;
	      }
	      break;
	    case ANALYZER_FIRE:
	      for (y = 0; y < h; y++, ptr += 304) {
		*ptr = y + 2;
		*(ptr + 1) = y + 2;
		*(ptr + 152) = y + 2;
		*(ptr + 153) = y + 2;
	      }
	      break;
	    case ANALYZER_VLINES:
	      for (y = 0; y < h; y++, ptr += 304) {
		*ptr = 18 - h;
		*(ptr + 1) = 18 - h;
		*(ptr + 152) = 18 - h;
		*(ptr + 153) = 18 - h;
	      }
	      
	      break;
	    }
	  }
	}
      }
      if (cfg.analyzer_peaks) {
	for (x = 0; x < 75; x++) {
	  if (cfg.analyzer_type == ANALYZER_BARS && (x % 4) == 0)
	    h = vis->vs_peak[x >> 2];
	  else if (cfg.analyzer_type == ANALYZER_LINES)
	    h = vis->vs_peak[x];
	  if (h && (cfg.analyzer_type == ANALYZER_LINES || (x % 4) != 3)){
	    
	    if (!vis->vs_doublesize) {
	      rgb_data[(16 - h) * 76 + x] = 23;
	    }
	    else{
	      ptr = rgb_data + (16 - h) * 304 + (x << 1);
	      *ptr = 23;
	      *(ptr + 1) = 23;
	      *(ptr + 152) = 23;
	      *(ptr + 153) = 23;
	    }
	  }
	}
      }
    }
    else if (cfg.vis_type == VIS_VOICEPRINT) {
      if(!bmp_playback_get_paused() && bmp_playback_get_playing()){/*Don't scroll when it's paused or stopped*/
	for (y = 0; y < 16; y ++)
	  for (x = 75; x > 0; x--)
	    voiceprint_data[x + y * 76] = voiceprint_data[x-1+y*76];
	  for(y=0;y<16;y++)
	    voiceprint_data[y * 76] = vis->vs_data[y];
      }
      if(bmp_playback_get_playing()){ /*Only draw the data if we're playing*/
	for (y = 0; y < 16; y ++){
	  for (x = 0; x < 76; x++){
	    guint8 d = voiceprint_data[x + y*76];
	    if(cfg.voiceprint_mode == VOICEPRINT_NORMAL){
	      d = d > 64 ? 17 : d >> 3 ;
	      if(!vis->vs_doublesize){
		rgb_data[x + y * 76] =  d;
	      }
	      else{
		ptr = rgb_data + (x << 1) + y * 304;
		*ptr = d;
		*(ptr + 1) = d;
		*(ptr + 152) = d;
		*(ptr + 153) = d;
	      }
	    }
	    else{
	      guint8 c[3]; // R, G, B array
	      if(cfg.voiceprint_mode == VOICEPRINT_FIRE){
		c[0] = d < 64 ? (d * 2) : 255; //R
		c[1] = d < 64 ? 0 : (d < 128 ? (d-64) * 2 : 255); //G
		c[2] = d < 128 ? 0 : (d-128) * 2; //B
	      }
	      else if(cfg.voiceprint_mode == VOICEPRINT_ICE){	    
		//c[0] = d < 192 ? 0 : (d-192) * 4; //R
		//c[1] = d < 192 ? 0 : (d-192) * 4; //G
		c[0] = d; //R
		c[1] = d < 128 ? d * 2 : 255; //G
		c[2] = d < 64 ? d * 4 : 255; //B
	      }
	      if(!vis->vs_doublesize){
		for(n=0;n<3;n++)
		  rgb_data[x * 3 + y * 76*3+n] = c[n];
	      }
	      else{
		ptr = rgb_data + x * 3 * 2 + y * 2 * 76 * 3 * 2;
		for(n=0;n<3;n++)
		  {
		    *(ptr + n) = c[n];
		    *(ptr + n + 3) = c[n];
		    *(ptr + n + 76 * 2 * 3) = c[n];
		    *(ptr + n + 3 + 76 * 2 * 3) = c[n];
		  }
	      }
	    }
	  }
	}
      }
    }
 if (cfg.vis_type == VIS_SCOPE) {
      for (x = 0; x < 75; x++) {
	switch (cfg.scope_mode) {
	case SCOPE_DOT:
	  h = vis->vs_data[x];
	  if (!vis->vs_doublesize) {
	  ptr = rgb_data + ((14 - h) * 76) + x;
	    *ptr = vis_scope_colors[h + 1];
	  }else{
	    ptr = rgb_data + ((14 - h) * 304) + (x << 1);
	    *ptr = vis_scope_colors[h + 1];
	    *(ptr + 1) = vis_scope_colors[h + 1];
	    *(ptr + 152) = vis_scope_colors[h + 1];
	    *(ptr + 153) = vis_scope_colors[h + 1];
	  }
	  break;
	case SCOPE_LINE:
	  if (x != 74) {
	    h = 14 - vis->vs_data[x];
	    h2 = 14 - vis->vs_data[x + 1];
	    if (h > h2) {
	      y = h;
	      h = h2;
	      h2 = y;
	    }
	    if (!vis->vs_doublesize) {
	    ptr = rgb_data + (h * 76) + x;
	    for (y = h; y <= h2; y++, ptr += 76)
	      *ptr = vis_scope_colors[y - 2];
	    }
	    else{
	      ptr = rgb_data + (h * 304) + (x << 1);
	      for (y = h; y <= h2; y++, ptr += 304) {
		*ptr = vis_scope_colors[y - 2];
		*(ptr + 1) = vis_scope_colors[y - 2];
		*(ptr + 152) = vis_scope_colors[y - 2];
		*(ptr + 153) = vis_scope_colors[y - 2];
	      }
	    }
	  }
	  else {
	    h = 14 - vis->vs_data[x];
	    if (!vis->vs_doublesize) {
	      ptr = rgb_data + (h * 76) + x;
	      *ptr = vis_scope_colors[h + 1];
	    }else{
	      ptr = rgb_data + (h * 304) + (x << 1);
	      *ptr = vis_scope_colors[h + 1];
	      *(ptr + 1) = vis_scope_colors[h + 1];
	      *(ptr + 152) = vis_scope_colors[h + 1];
	      *(ptr + 153) = vis_scope_colors[h + 1];
	    }
	  }
	  break;
	case SCOPE_SOLID:
	  h = 14 - vis->vs_data[x];
	  h2 = 8;
	  c = vis_scope_colors[(gint) vis->vs_data[x]];
	  if (h > h2) {
	    y = h;
	    h = h2;
	    h2 = y;
	  }
	  if (!vis->vs_doublesize) {
	    ptr = rgb_data + (h * 76) + x;
	    for (y = h; y <= h2; y++, ptr += 76)
	      *ptr = c;
	  }else{
	    ptr = rgb_data + (h * 304) + (x << 1);
	    for (y = h; y <= h2; y++, ptr += 304) {
	      *ptr = c;
	      *(ptr + 1) = c;
	      *(ptr + 152) = c;
	      *(ptr + 153) = c;
	    }
	  }
	  break;
	}
      }
    }
    

    
    if (!vis->vs_doublesize) {
        GDK_THREADS_ENTER();
	if (cfg.vis_type == VIS_VOICEPRINT && cfg.voiceprint_mode != VOICEPRINT_NORMAL){
	  gdk_draw_rgb_image(vis->vs_window, vis->vs_widget.gc,
			     vis->vs_widget.x, vis->vs_widget.y,
			     vis->vs_widget.width, vis->vs_widget.height,
			     GDK_RGB_DITHER_NORMAL, (guchar *) rgb_data,
			     76 * 3);
	}
	else{
	  gdk_draw_indexed_image(vis->vs_window, vis->vs_widget.gc,
				 vis->vs_widget.x, vis->vs_widget.y,
				 vis->vs_widget.width, vis->vs_widget.height,
				 GDK_RGB_DITHER_NORMAL, (guchar *) rgb_data,
				 76, cmap);
	}
        GDK_THREADS_LEAVE();
    }
    else {
      GDK_THREADS_ENTER();
	if (cfg.vis_type == VIS_VOICEPRINT && cfg.voiceprint_mode != VOICEPRINT_NORMAL){
	  gdk_draw_rgb_image(vis->vs_window, vis->vs_widget.gc,
				 vis->vs_widget.x << 1,
				 vis->vs_widget.y << 1,
				 vis->vs_widget.width << 1,
				 vis->vs_widget.height << 1,
				 GDK_RGB_DITHER_NONE, (guchar *) rgb_data,
				 76 * 2 * 3);
	}
	else{
      gdk_draw_indexed_image(vis->vs_window, vis->vs_widget.gc,
			     vis->vs_widget.x << 1,
			     vis->vs_widget.y << 1,
			     vis->vs_widget.width << 1,
			     vis->vs_widget.height << 1,
			     GDK_RGB_DITHER_NONE, (guchar *) rgb_data,
			     76 * 2 , cmap);
	}
      GDK_THREADS_LEAVE();
    }
    gdk_rgb_cmap_free(cmap);
}

	    

void
vis_clear_data(Vis * vis)
{
    gint i;

    if (!vis)
        return;
    memset(voiceprint_data, 0, 16*76);
    for (i = 0; i < 75; i++) {
        vis->vs_data[i] = (cfg.vis_type == VIS_SCOPE) ? 6 : 0;
        vis->vs_peak[i] = 0;
    }
}

void
vis_set_doublesize(Vis * vis, gboolean doublesize)
{
    vis->vs_doublesize = doublesize;
}

void
vis_clear(Vis * vis)
{
    if (!vis->vs_doublesize)
        gdk_window_clear_area(vis->vs_window, vis->vs_widget.x,
                              vis->vs_widget.y, vis->vs_widget.width,
                              vis->vs_widget.height);
    else
        gdk_window_clear_area(vis->vs_window, vis->vs_widget.x << 1,
                              vis->vs_widget.y << 1,
                              vis->vs_widget.width << 1,
                              vis->vs_widget.height << 1);
}

void
vis_set_window(Vis * vis, GdkWindow * window)
{
    vis->vs_window = window;
}

void vis_draw_pixel(Vis * vis, guchar* texture, gint x, gint y, guint8 colour){
  if(vis->vs_doublesize){
    texture[y * 76 + x] = colour;
    texture[y * 76 + x + 1] = colour;
    texture[y * 76 * 4 + x] = colour;
    texture[y * 76 * 4 + x + 1] = colour;
  }
  else{
    texture[y * 76 + x] = colour;
  }
}


Vis *
create_vis(GList ** wlist,
           GdkPixmap * parent,
           GdkWindow * window,
           GdkGC * gc,
           gint x, gint y,
           gint width,
	   gboolean doublesize)
{
    Vis *vis;

    vis = g_new0(Vis, 1);
    memset(voiceprint_data, 0, 16*76);
    widget_init(&vis->vs_widget, parent, gc, x, y, width, 16, 1);

    vis->vs_doublesize = doublesize;

    widget_list_add(wlist, WIDGET(vis));

    return vis;
}
