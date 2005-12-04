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
#include <gtk/gtk.h>

#include "monitor.h"
#include "crossfade.h"

#include "interface.h"
#include "support.h"


static GtkWidget   *monitor_win;
static GtkWidget   *monitor_display_drawingarea;
static GtkProgress *monitor_output_progress;
static GtkLabel    *monitor_position_label;
static GtkLabel    *monitor_total_label;
static GtkLabel    *monitor_left_label;
static GtkLabel    *monitor_output_time_label;
static GtkLabel    *monitor_output_time_sep;
static GtkLabel    *monitor_written_time_label;

static gchar       *default_position_str = NULL;
static gchar       *default_total_str    = NULL;
static gchar       *default_left_str     = NULL;
static gchar       *default_output_time_str  = NULL;
static gchar       *default_written_time_str = NULL;

static gboolean monitor_active  = FALSE;
static guint    monitor_tag;
static gint     monitor_output_max;
static gint     monitor_closing;

#define RUNNING 0
#define CLOSING 1
#define CLOSED  2

#define SMOD(x,n) (((x)<0)?((n)-(x))%(n):((x)%(n)))


static void draw_wrapped(GtkWidget *widget, gint pos, gint width, GdkGC *gc)
{
  GdkDrawable *win = widget->window;

  gint ww = widget->allocation.width;
  gint wh = widget->allocation.height;

  if(width <= 0) return;
  if(width < ww) {
    gint x = SMOD(pos, ww);
    if((x+width) >= ww) {
      gdk_draw_rectangle(win, gc, TRUE, x, 0,        ww-x,  wh);
      gdk_draw_rectangle(win, gc, TRUE, 0, 0, width-(ww-x), wh);
    }
    else gdk_draw_rectangle(win, gc, TRUE, x, 0, width, wh);
  }
  else gdk_draw_rectangle(win, gc, TRUE, 0, 0, ww, wh);
}
 
gboolean on_monitor_display_drawingarea_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  if(buffer && buffer->size && output_opened) {
    gint ww = widget->allocation.width;

    gint x1 = 0;
    gint x2 = buffer->used;
    gint x3 = buffer->used + buffer->mix;
    gint x4 = buffer->size;

    x1 = (gint64)(x1 + buffer->rd_index) * ww / buffer->size;
    x2 = (gint64)(x2 + buffer->rd_index) * ww / buffer->size;
    x3 = (gint64)(x3 + buffer->rd_index) * ww / buffer->size;
    x4 = (gint64)(x4 + buffer->rd_index) * ww / buffer->size;

    draw_wrapped(widget, x1, x2-x1, widget->style->fg_gc[GTK_STATE_NORMAL]);
    draw_wrapped(widget, x2, x3-x2, widget->style->white_gc);
    draw_wrapped(widget, x3, x4-x3, widget->style->bg_gc[GTK_STATE_NORMAL]);
  }
  else
    gdk_window_clear_area(widget->window,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height);
  return TRUE;
}

gboolean on_monitor_win_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  /* V0.1.1 20000618: removed, didn't make much sense since it wasn't saved */
  /* if(config) config->enable_monitor = FALSE; */
  if(default_position_str)     { g_free(default_position_str);     default_position_str     = NULL; }
  if(default_total_str)        { g_free(default_total_str);        default_total_str        = NULL; }
  if(default_left_str)         { g_free(default_left_str);         default_left_str         = NULL; }
  if(default_output_time_str)  { g_free(default_output_time_str);  default_output_time_str  = NULL; }
  if(default_written_time_str) { g_free(default_written_time_str); default_written_time_str = NULL; }
  return(FALSE);  /* FALSE: destroy window */
}

extern GtkWindow *mainwin;

void xfade_check_monitor_win()
{
  gchar *str;

  if(config->enable_monitor) {
    if(!monitor_win && !(monitor_win = create_monitor_win())) {
      DEBUG(("[crossfade] check_monitor_win: error creating window!\n"));
      return;
    }

    /* automatically set config_win to NULL when window is destroyed */
    gtk_signal_connect(GTK_OBJECT(monitor_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &monitor_win);

    /* show window */
    gtk_widget_show(monitor_win);
   
    /* get pointers to widgets (used by crossfade.c to update the monitor) */
    monitor_display_drawingarea =              lookup_widget(monitor_win, "monitor_display_drawingarea");
    monitor_output_progress     = GTK_PROGRESS(lookup_widget(monitor_win, "monitor_output_progress"));
    monitor_position_label      = GTK_LABEL   (lookup_widget(monitor_win, "monpos_position_label"));
    monitor_total_label         = GTK_LABEL   (lookup_widget(monitor_win, "monpos_total_label"));
    monitor_left_label          = GTK_LABEL   (lookup_widget(monitor_win, "monpos_left_label"));
    monitor_output_time_label   = GTK_LABEL   (lookup_widget(monitor_win, "monpos_output_time_label"));
    monitor_output_time_sep     = GTK_LABEL   (lookup_widget(monitor_win, "monpos_output_time_separator_label"));
    monitor_written_time_label  = GTK_LABEL   (lookup_widget(monitor_win, "monpos_written_time_label"));

    /* get default strings (displayed when monitor is stopped) */
    if(!default_position_str)     { gtk_label_get(monitor_position_label,     &str); default_position_str     = g_strdup(str); }
    if(!default_total_str)        { gtk_label_get(monitor_total_label,        &str); default_total_str        = g_strdup(str); }
    if(!default_left_str)         { gtk_label_get(monitor_left_label,         &str); default_left_str         = g_strdup(str); }
    if(!default_output_time_str)  { gtk_label_get(monitor_output_time_label,  &str); default_output_time_str  = g_strdup(str); }
    if(!default_written_time_str) { gtk_label_get(monitor_written_time_label, &str); default_written_time_str = g_strdup(str); }

    /* force gtk_progress_configure */
    monitor_output_max = 0;
  }
  else
    if(monitor_win) gtk_widget_destroy(monitor_win);
}

void label_set_text(GtkLabel *label, gchar *text)
{
  gchar *old_text;
  gtk_label_get(label, &old_text);
  if(strcmp(old_text, text) == 0) return;
  gtk_label_set_text(label, text);
}

gint xfade_update_monitor(gpointer userdata)
{
  /* HACK: (see xfade_stop_monitor() below) */
  if(monitor_closing == CLOSED)
    return TRUE;

  if(monitor_closing == CLOSING)
    monitor_closing = CLOSED;

  if(!monitor_win)
    return TRUE;

  /* lock buffer (only if we need to) */
  if(monitor_closing != CLOSED)
    g_static_mutex_lock(&buffer_mutex);

  if(1 || monitor_win) {
    gint output_time  = the_op->output_time();
    gint written_time = the_op->written_time();
    gint output_used  = written_time - output_time;

    /*** Mixing Buffer ***/
    if(1 || monitor_display_drawingarea) {
      GdkRectangle update_rect;

      update_rect.x = 0;
      update_rect.y = 0;
      update_rect.width = monitor_display_drawingarea->allocation.width;
      update_rect.height = monitor_display_drawingarea->allocation.height;

      if(monitor_closing == CLOSED)
	gdk_window_clear_area(monitor_display_drawingarea->window,
			      update_rect.x,     update_rect.y,
			      update_rect.width, update_rect.height);
      else
	gtk_widget_draw(monitor_display_drawingarea, &update_rect);
    }

    /*** Output Buffer ***/ 
    if(1 || monitor_output_progress) {
      if(monitor_closing == CLOSED) {
	gtk_progress_configure(monitor_output_progress, 0, 0, 0);
	monitor_output_max = 0;
      }
      else {
	if(output_opened && the_op->buffer_playing()) {
	  if(output_used < 0) output_used = 0;
	  if(output_used > monitor_output_max) {
	    monitor_output_max = output_used;
	    gtk_progress_configure(monitor_output_progress,
				   output_used, 0, monitor_output_max);
	  }
	  else
	    gtk_progress_set_value(monitor_output_progress, output_used);
	}
	else {
	  gtk_progress_configure(monitor_output_progress, 0, 0, 0);
	  monitor_output_max = 0;
	}
      }	
    }

    /*** Position ***/
    if(1 || (monitor_position_label && monitor_total_label && monitor_left_label)) {
#ifndef HAVE_BEEP
      if(!get_input_playing() || (monitor_closing == CLOSED)) {  /* XMMS */
#else
      if(!bmp_playback_get_playing() || (monitor_closing == CLOSED)) {  /* XMMS */
#endif
	gtk_label_set_text(monitor_position_label, default_position_str);
	gtk_label_set_text(monitor_total_label,    default_total_str);
	gtk_label_set_text(monitor_left_label,     default_left_str);
      }
      else {
	gchar buffer[32];
	gint  position = output_time - output_offset;
	gint  total    = playlist_get_current_length();  /* XMMS */
	gint  left     = total - position;

	/* position */
	g_snprintf(buffer, sizeof(buffer),
		   position < 0 ? "-%d:%02d.%01d" : "%d:%02d.%01d",
		   ABS((position/60000)),
		   ABS((position/1000) % 60),
		   ABS((position/100)  % 10));
	gtk_label_set_text(monitor_position_label, buffer);
	
	/* total */
	if(total > 0) {
	  g_snprintf(buffer, sizeof(buffer), "%d:%02d", total/60000, (total/1000)%60);
	  gtk_label_set_text(monitor_total_label, buffer);
	}
	else
	  label_set_text(monitor_total_label, default_total_str);

	  
	/* left */
	if(total > 0) {
	  g_snprintf(buffer, sizeof(buffer), "%d:%02d", left/60000, (left/1000)%60);
	  gtk_label_set_text(monitor_left_label, buffer);
	}
	else
	  label_set_text(monitor_left_label, default_left_str);
      }
    }

    /*** Output Plugin position */
    if(1 || (monitor_written_time_label && monitor_output_time_label)) {
      if(monitor_closing == CLOSED) {
	gtk_widget_hide(GTK_WIDGET(monitor_output_time_label));
	gtk_widget_hide(GTK_WIDGET(monitor_output_time_sep));
	gtk_label_set_text(monitor_written_time_label, default_written_time_str);
      }
      else {
	gchar buffer[32];

	/* check for output plugin bug -- diff should always be 0 */
	gint diff = written_time - (gint)(output_streampos * 1000 / OUTPUT_BPS);
	if(diff) {
	  gtk_widget_show(GTK_WIDGET(monitor_output_time_label));
	  gtk_widget_show(GTK_WIDGET(monitor_output_time_sep));

	  g_snprintf(buffer, sizeof(buffer),
		     output_time < 0 ? "-%d:%02d.%03d" : "%d:%02d.%03d",
		     ABS((diff/60000)),
		     ABS((diff/1000) % 60),
		     ABS((diff)    % 1000));
	  gtk_label_set_text(monitor_output_time_label, buffer);
	}
	else {
	  gtk_widget_hide(GTK_WIDGET(monitor_output_time_label));
	  gtk_widget_hide(GTK_WIDGET(monitor_output_time_sep));
	}

	/* written_time */
	g_snprintf(buffer, sizeof(buffer),
		   written_time < 0 ? "-%d:%02d:%02d.%01d" : "%d:%02d:%02d.%01d",
		   ABS((written_time/3600000)),
		   ABS((written_time/60000) % 60),
		   ABS((written_time/1000)  % 60),
		   ABS((written_time/100)   % 10));
	gtk_label_set_text(monitor_written_time_label, buffer);
      }
    }
  }

  /* unlock buffer */
  if(monitor_closing != CLOSED)
    g_static_mutex_unlock(&buffer_mutex);

  return TRUE;  /* continue calling this function */
}

void xfade_start_monitor()
{
  if(monitor_active) return;
  monitor_output_max = 0;
  monitor_closing    = RUNNING;
  monitor_active     = TRUE;
  monitor_tag        = gtk_timeout_add(UPDATE_INTERVAL, xfade_update_monitor, NULL);
}

void xfade_stop_monitor()
{
  gint max_wait = UPDATE_INTERVAL/10+1+1;  /* round up / add safety */

  if(!monitor_active) return;

  /* HACK, ugly HACK: force a final call of xfade_update_monitor */
  monitor_closing = CLOSING;
  while((monitor_closing == CLOSING) && (max_wait-- > 0))
    xmms_usleep(10000);
  
  if(max_wait <= 0)
    DEBUG(("[crossfade] stop_monitor: timeout!\n"));

  /* stop calling xfade_update_monitor() */
  gtk_timeout_remove(monitor_tag);
  monitor_active = FALSE;
}
