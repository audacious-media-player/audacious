/*
 * Audacious - a cross-platform multimedia player
 * Copyright (c) 2007 Tomasz Moń
 *
 * Based on:
 * BMP - Cross-platform multimedia player
 * Copyright (C) 2003-2004  BMP development team.
 * XMMS:
 * Copyright (C) 1998-2003  XMMS development team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

/*
 *  A note about Pango and some funky spacey fonts: Weirdly baselined
 *  fonts, or fonts with weird ascents or descents _will_ display a
 *  little bit weird in the playlist widget, but the display engine
 *  won't make it look too bad, just a little deranged.  I honestly
 *  don't think it's worth fixing (around...), it doesn't have to be
 *  perfectly fitting, just the general look has to be ok, which it
 *  IMHO is.
 *
 *  A second note: The numbers aren't perfectly aligned, but in the
 *  end it looks better when using a single Pango layout for each
 *  number.
 */


#include "skin.h"
#include "ui_skinned_playlist.h"
#include "main.h"
#include "util.h"
#include "ui_playlist.h"

#include "input.h"
#include "strings.h"
#include "playback.h"
#include "playlist.h"
#include "ui_manager.h"
#include "ui_fileinfopopup.h"

#include "debug.h"
static PangoFontDescription *playlist_list_font = NULL;
static gint ascent, descent, width_delta_digit_one;
static gboolean has_slant;
static guint padding;

/* FIXME: the following globals should not be needed. */
static gint width_approx_letters;
static gint width_colon, width_colon_third;
static gint width_approx_digits, width_approx_digits_half;

#define UI_SKINNED_PLAYLIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), ui_skinned_playlist_get_type(), UiSkinnedPlaylistPrivate))
typedef struct _UiSkinnedPlaylistPrivate UiSkinnedPlaylistPrivate;

enum {
    REDRAW,
    LAST_SIGNAL
};

struct _UiSkinnedPlaylistPrivate {
    SkinPixmapId     skin_index;
    gint             width, height;
    gint             resize_width, resize_height;
    gint             drag_pos;
    gboolean         dragging, auto_drag_down, auto_drag_up;
    gint             auto_drag_up_tag, auto_drag_down_tag;
};

static void ui_skinned_playlist_class_init         (UiSkinnedPlaylistClass *klass);
static void ui_skinned_playlist_init               (UiSkinnedPlaylist *playlist);
static void ui_skinned_playlist_destroy            (GtkObject *object);
static void ui_skinned_playlist_realize            (GtkWidget *widget);
static void ui_skinned_playlist_size_request       (GtkWidget *widget, GtkRequisition *requisition);
static void ui_skinned_playlist_size_allocate      (GtkWidget *widget, GtkAllocation *allocation);
static gboolean ui_skinned_playlist_expose         (GtkWidget *widget, GdkEventExpose *event);
static gboolean ui_skinned_playlist_button_press   (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_playlist_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean ui_skinned_playlist_motion_notify  (GtkWidget *widget, GdkEventMotion *event);
static gboolean ui_skinned_playlist_leave_notify   (GtkWidget *widget, GdkEventCrossing *event);
static void ui_skinned_playlist_redraw             (UiSkinnedPlaylist *playlist);
static gboolean ui_skinned_playlist_popup_show     (gpointer data);
static void ui_skinned_playlist_popup_hide         (GtkWidget *widget);
static void ui_skinned_playlist_popup_timer_start  (GtkWidget *widget);
static void ui_skinned_playlist_popup_timer_stop   (GtkWidget *widget);

static GtkWidgetClass *parent_class = NULL;
static guint playlist_signals[LAST_SIGNAL] = { 0 };

GType ui_skinned_playlist_get_type() {
    static GType playlist_type = 0;
    if (!playlist_type) {
        static const GTypeInfo playlist_info = {
            sizeof (UiSkinnedPlaylistClass),
            NULL,
            NULL,
            (GClassInitFunc) ui_skinned_playlist_class_init,
            NULL,
            NULL,
            sizeof (UiSkinnedPlaylist),
            0,
            (GInstanceInitFunc) ui_skinned_playlist_init,
        };
        playlist_type = g_type_register_static (GTK_TYPE_WIDGET, "UiSkinnedPlaylist", &playlist_info, 0);
    }

    return playlist_type;
}

static void ui_skinned_playlist_class_init(UiSkinnedPlaylistClass *klass) {
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    gobject_class = G_OBJECT_CLASS(klass);
    object_class = (GtkObjectClass*) klass;
    widget_class = (GtkWidgetClass*) klass;
    parent_class = gtk_type_class (gtk_widget_get_type ());

    object_class->destroy = ui_skinned_playlist_destroy;

    widget_class->realize = ui_skinned_playlist_realize;
    widget_class->expose_event = ui_skinned_playlist_expose;
    widget_class->size_request = ui_skinned_playlist_size_request;
    widget_class->size_allocate = ui_skinned_playlist_size_allocate;
    widget_class->button_press_event = ui_skinned_playlist_button_press;
    widget_class->button_release_event = ui_skinned_playlist_button_release;
    widget_class->motion_notify_event = ui_skinned_playlist_motion_notify;
    widget_class->leave_notify_event = ui_skinned_playlist_leave_notify;

    klass->redraw = ui_skinned_playlist_redraw;

    playlist_signals[REDRAW] = 
        g_signal_new ("redraw", G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (UiSkinnedPlaylistClass, redraw), NULL, NULL,
                      gtk_marshal_VOID__VOID, G_TYPE_NONE, 0);

    g_type_class_add_private (gobject_class, sizeof (UiSkinnedPlaylistPrivate));
}

static void ui_skinned_playlist_init(UiSkinnedPlaylist *playlist) {
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(playlist);
    playlist->pressed = FALSE;
    priv->resize_width = 0;
    priv->resize_height = 0;
    playlist->prev_selected = -1;
    playlist->prev_min = -1;
    playlist->prev_max = -1;

    g_object_set_data(G_OBJECT(playlist), "timer_id", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(playlist), "timer_active", GINT_TO_POINTER(0));

    GtkWidget *popup = fileinfopopup_create();
    g_object_set_data(G_OBJECT(playlist), "popup", popup);
    g_object_set_data(G_OBJECT(playlist), "popup_active", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(playlist), "popup_position", GINT_TO_POINTER(-1));
}

GtkWidget* ui_skinned_playlist_new(GtkWidget *fixed, gint x, gint y, gint w, gint h) {

    UiSkinnedPlaylist *hs = g_object_new (ui_skinned_playlist_get_type (), NULL);
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(hs);

    hs->x = x;
    hs->y = y;
    priv->width = w;
    priv->height = h;
    priv->skin_index = SKIN_PLEDIT;

    gtk_fixed_put(GTK_FIXED(fixed), GTK_WIDGET(hs), hs->x, hs->y);

    return GTK_WIDGET(hs);
}

static void ui_skinned_playlist_destroy(GtkObject *object) {
    UiSkinnedPlaylist *playlist;

    g_return_if_fail (object != NULL);
    g_return_if_fail (UI_SKINNED_IS_PLAYLIST (object));

    playlist = UI_SKINNED_PLAYLIST (object);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void ui_skinned_playlist_realize(GtkWidget *widget) {
    UiSkinnedPlaylist *playlist;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (UI_SKINNED_IS_PLAYLIST(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    playlist = UI_SKINNED_PLAYLIST(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
                             GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window = gdk_window_new(widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach(widget->style, widget->window);
    gdk_window_set_user_data(widget->window, widget);
}

static void ui_skinned_playlist_size_request(GtkWidget *widget, GtkRequisition *requisition) {
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(widget);

    requisition->width = priv->width;
    requisition->height = priv->height;
}

static void ui_skinned_playlist_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
    UiSkinnedPlaylist *playlist = UI_SKINNED_PLAYLIST (widget);
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(playlist);

    widget->allocation = *allocation;
    if (GTK_WIDGET_REALIZED (widget))
        gdk_window_move_resize(widget->window, widget->allocation.x, widget->allocation.y, allocation->width, allocation->height);

    playlist->x = widget->allocation.x;
    playlist->y = widget->allocation.y;

    if (priv->height != widget->allocation.height || priv->width != widget->allocation.width) {
        priv->width = priv->width + priv->resize_width;
        priv->height = priv->height + priv->resize_height;
        priv->resize_width = 0;
        priv->resize_height = 0;
        gtk_widget_queue_draw(widget);
    }
}

static gboolean ui_skinned_playlist_auto_drag_down_func(gpointer data) {
    UiSkinnedPlaylist *pl = UI_SKINNED_PLAYLIST(data);
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(data);

    if (priv->auto_drag_down) {
        ui_skinned_playlist_move_down(pl);
        pl->first++;
        playlistwin_update_list(playlist_get_active());
        return TRUE;
    }
    return FALSE;
}

static gboolean ui_skinned_playlist_auto_drag_up_func(gpointer data) {
    UiSkinnedPlaylist *pl = UI_SKINNED_PLAYLIST(data);
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(data);

    if (priv->auto_drag_up) {
        ui_skinned_playlist_move_up(pl);
        pl->first--;
        playlistwin_update_list(playlist_get_active());
        return TRUE;

    }
    return FALSE;
}

void ui_skinned_playlist_move_up(UiSkinnedPlaylist * pl) {
    GList *list;
    Playlist *playlist = playlist_get_active();

    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist);
    if ((list = playlist->entries) == NULL) {
        PLAYLIST_UNLOCK(playlist);
        return;
    }
    if (PLAYLIST_ENTRY(list->data)->selected) {
        /* We are at the top */
        PLAYLIST_UNLOCK(playlist);
        return;
    }
    while (list) {
        if (PLAYLIST_ENTRY(list->data)->selected)
            glist_moveup(list);
        list = g_list_next(list);
    }
    PLAYLIST_UNLOCK(playlist);
    if (pl->prev_selected != -1)
        pl->prev_selected--;
    if (pl->prev_min != -1)
        pl->prev_min--;
    if (pl->prev_max != -1)
        pl->prev_max--;
}

void ui_skinned_playlist_move_down(UiSkinnedPlaylist * pl) {
    GList *list;
    Playlist *playlist = playlist_get_active();

    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist);

    if (!(list = g_list_last(playlist->entries))) {
        PLAYLIST_UNLOCK(playlist);
        return;
    }

    if (PLAYLIST_ENTRY(list->data)->selected) {
        /* We are at the bottom */
        PLAYLIST_UNLOCK(playlist);
        return;
    }

    while (list) {
        if (PLAYLIST_ENTRY(list->data)->selected)
            glist_movedown(list);
        list = g_list_previous(list);
    }

    PLAYLIST_UNLOCK(playlist);

    if (pl->prev_selected != -1)
        pl->prev_selected++;
    if (pl->prev_min != -1)
        pl->prev_min++;
    if (pl->prev_max != -1)
        pl->prev_max++;
}

static void
playlist_list_draw_string(GdkPixmap *obj, GdkGC *gc, UiSkinnedPlaylist *pl,
                          PangoFontDescription * font,
                          gint line,
                          gint width,
                          const gchar * text,
                          guint ppos)
{
    guint plist_length_int;
    Playlist *playlist = playlist_get_active();
    PangoLayout *layout;

    REQUIRE_LOCK(playlist->mutex);

    if (cfg.show_numbers_in_pl) {
        gchar *pos_string = g_strdup_printf(cfg.show_separator_in_pl == TRUE ? "%d" : "%d.", ppos);
        plist_length_int =
            gint_count_digits(playlist_get_length(playlist)) + !cfg.show_separator_in_pl + 1; /* cf.show_separator_in_pl will be 0 if false */

        padding = plist_length_int;
        padding = ((padding + 1) * width_approx_digits);

        layout = gtk_widget_create_pango_layout(playlistwin, pos_string);
        pango_layout_set_font_description(layout, playlist_list_font);
        pango_layout_set_width(layout, plist_length_int * 100);

        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);


        gdk_draw_layout(obj, gc, (width_approx_digits *
                         (-1 + plist_length_int - strlen(pos_string))) +
                        (width_approx_digits / 4),
                        (line - 1) * pl->fheight +
                        ascent + abs(descent), layout);
        g_free(pos_string);
        g_object_unref(layout);

        if (!cfg.show_separator_in_pl)
            padding -= (width_approx_digits * 1.5);
    } else {
        padding = 3;
    }

    width -= padding;

    layout = gtk_widget_create_pango_layout(playlistwin, text);

    pango_layout_set_font_description(layout, playlist_list_font);
    pango_layout_set_width(layout, width * PANGO_SCALE);
    pango_layout_set_single_paragraph_mode(layout, TRUE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    gdk_draw_layout(obj, gc,
                    padding + (width_approx_letters / 4),
                    (line - 1) * pl->fheight +
                    ascent + abs(descent), layout);

    g_object_unref(layout);
}

static gboolean ui_skinned_playlist_expose(GtkWidget *widget, GdkEventExpose *event) {
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (UI_SKINNED_IS_PLAYLIST (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    UiSkinnedPlaylist *pl = UI_SKINNED_PLAYLIST (widget);
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(pl);

    Playlist *playlist = playlist_get_active();
    GList *list;
    PangoLayout *layout;
    gchar *title;
    gint width, height;
    gint i, max_first;
    guint padding, padding_dwidth, padding_plength;
    guint max_time_len = 0;
    gfloat queue_tailpadding = 0;
    gint tpadding; 
    gsize tpadding_dwidth = 0;
    gint x, y;
    guint tail_width;
    guint tail_len;

    gchar tail[100];
    gchar queuepos[255];
    gchar length[40];

    gchar **frags;
    gchar *frag0;

    gint plw_w, plw_h;

    GdkPixmap *obj = NULL;
    GdkGC *gc;

    obj = gdk_pixmap_new(NULL, priv->width, priv->height, gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(obj);

    GdkRectangle *playlist_rect;

    width = priv->width;
    height = priv->height;

    plw_w = playlistwin_get_width();
    plw_h = playlistwin_get_height();

    playlist_rect = g_slice_new0(GdkRectangle);

    playlist_rect->x = 0;
    playlist_rect->y = 0;
    playlist_rect->width = plw_w - 17;
    playlist_rect->height = plw_h - 36;

    gdk_gc_set_clip_origin(gc, 31, 58);
    gdk_gc_set_clip_rectangle(gc, playlist_rect);

    gdk_gc_set_foreground(gc,
                          skin_get_color(bmp_active_skin,
                                         SKIN_PLEDIT_NORMALBG));
    gdk_draw_rectangle(obj, gc, TRUE, 0, 0,
                       width, height);

    if (!playlist_list_font) {
        g_critical("Couldn't open playlist font");
        return FALSE;
    }

    pl->fheight = (ascent + abs(descent));
    pl->num_visible = height / pl->fheight;

    max_first = playlist_get_length(playlist) - pl->num_visible;
    max_first = MAX(max_first, 0);

    pl->first = CLAMP(pl->first, 0, max_first);

    PLAYLIST_LOCK(playlist);
    list = playlist->entries;
    list = g_list_nth(list, pl->first);

    /* It sucks having to run the iteration twice but this is the only
       way you can reliably get the maximum width so we can get our
       playlist nice and aligned... -- plasmaroo */

    for (i = pl->first;
         list && i < pl->first + pl->num_visible;
         list = g_list_next(list), i++) {
        PlaylistEntry *entry = list->data;

        if (entry->length != -1)
        {
            g_snprintf(length, sizeof(length), "%d:%-2.2d",
                       entry->length / 60000, (entry->length / 1000) % 60);
            tpadding_dwidth = MAX(tpadding_dwidth, strlen(length));
        }
    }

    /* Reset */
    list = playlist->entries;
    list = g_list_nth(list, pl->first);

    for (i = pl->first;
         list && i < pl->first + pl->num_visible;
         list = g_list_next(list), i++) {
        gint pos;
        PlaylistEntry *entry = list->data;

        if (entry->selected) {
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_SELECTEDBG));
            gdk_draw_rectangle(obj, gc, TRUE, 0,
                               ((i - pl->first) * pl->fheight),
                               width, pl->fheight);
        }

        /* FIXME: entry->title should NEVER be NULL, and there should
           NEVER be a need to do a UTF-8 conversion. Playlist title
           strings should be kept properly. */

        if (!entry->title) {
            gchar *realfn = g_filename_from_uri(entry->filename, NULL, NULL);
            gchar *basename = g_path_get_basename(realfn ? realfn : entry->filename);
            title = filename_to_utf8(basename);
            g_free(basename); g_free(realfn);
        }
        else
            title = str_to_utf8(entry->title);

        title = convert_title_text(title);

        pos = playlist_get_queue_position(playlist, entry);

        tail[0] = 0;
        queuepos[0] = 0;
        length[0] = 0;

        if (pos != -1)
            g_snprintf(queuepos, sizeof(queuepos), "%d", pos + 1);

        if (entry->length != -1)
        {
            g_snprintf(length, sizeof(length), "%d:%-2.2d",
                       entry->length / 60000, (entry->length / 1000) % 60);
        }

        strncat(tail, length, sizeof(tail) - 1);
        tail_len = strlen(tail);

        max_time_len = MAX(max_time_len, tail_len);

        if (pos != -1 && tpadding_dwidth <= 0)
            tail_width = width - (width_approx_digits * (strlen(queuepos) + 2.25));
        else if (pos != -1)
            tail_width = width - (width_approx_digits * (tpadding_dwidth + strlen(queuepos) + 4));
        else if (tpadding_dwidth > 0)
            tail_width = width - (width_approx_digits * (tpadding_dwidth + 2.5));
        else
            tail_width = width;

        if (i == playlist_get_position_nolock(playlist))
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_CURRENT));
        else
            gdk_gc_set_foreground(gc,
                                  skin_get_color(bmp_active_skin,
                                                 SKIN_PLEDIT_NORMAL));
        playlist_list_draw_string(obj, gc, pl, playlist_list_font,
                                  i - pl->first, tail_width, title,
                                  i + 1);

        x = width - width_approx_digits * 2;
        y = ((i - pl->first) - 1) * pl->fheight + ascent;

        frags = NULL;
        frag0 = NULL;

        if ((strlen(tail) > 0) && (tail != NULL)) {
            frags = g_strsplit(tail, ":", 0);
            frag0 = g_strconcat(frags[0], ":", NULL);

            layout = gtk_widget_create_pango_layout(playlistwin, frags[1]);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_width(layout, tail_len * 100);
            pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
            gdk_draw_layout(obj, gc, x - (0.5 * width_approx_digits),
                            y + abs(descent), layout);
            g_object_unref(layout);

            layout = gtk_widget_create_pango_layout(playlistwin, frag0);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_width(layout, tail_len * 100);
            pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
            gdk_draw_layout(obj, gc, x - (0.75 * width_approx_digits),
                            y + abs(descent), layout);
            g_object_unref(layout);

            g_free(frag0);
            g_strfreev(frags);
        }

        if (pos != -1) {

            /* DON'T remove the commented code yet please     -- Milosz */

            if (tpadding_dwidth > 0)
                queue_tailpadding = tpadding_dwidth + 1;
            else
                queue_tailpadding = -0.75;

            gdk_draw_rectangle(obj, gc, FALSE,
                               x -
                               (((queue_tailpadding +
                                  strlen(queuepos)) *
                                 width_approx_digits) +
                                (width_approx_digits / 4)),
                               y + abs(descent),
                               (strlen(queuepos)) *
                               width_approx_digits +
                               (width_approx_digits / 2),
                               pl->fheight - 2);

            layout =
                gtk_widget_create_pango_layout(playlistwin, queuepos);
            pango_layout_set_font_description(layout, playlist_list_font);
            pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

            gdk_draw_layout(obj, gc,
                            x -
                            ((queue_tailpadding +
                              strlen(queuepos)) * width_approx_digits) +
                            (width_approx_digits / 4),
                            y + abs(descent), layout);
            g_object_unref(layout);
        }

        g_free(title);
    }


    /*
     * Drop target hovering over the playlist, so draw some hint where the
     * drop will occur.
     *
     * This is (currently? unfixably?) broken when dragging files from Qt/KDE apps,
     * probably due to DnD signaling problems (actually i have no clue).
     *
     */

    if (pl->drag_motion) {
        guint pos, plength, lpadding;
	gint x, y, plx, ply;

        if (cfg.show_numbers_in_pl) {
            lpadding = gint_count_digits(playlist_get_length(playlist)) + 1;
            lpadding = ((lpadding + 1) * width_approx_digits);
        }
        else {
            lpadding = 3;
        };

        /* We already hold the mutex and have the playlist locked, so call
           the non-locking function. */
        plength = playlist_get_length(playlist);

        x = pl->drag_motion_x;
        y = pl->drag_motion_y;

        plx = pl->x;
        ply = pl->y;

        if ((x > pl->x) && !(x > priv->width)) {

            if ((y > pl->y)
                && !(y > (priv->height + ply))) {

                pos = ((y - pl->y) / pl->fheight) +
                    pl->first;

                if (pos > (plength)) {
                    pos = plength;
                }

                gdk_gc_set_foreground(gc,
                                      skin_get_color(bmp_active_skin,
                                                     SKIN_PLEDIT_CURRENT));

                gdk_draw_line(obj, gc, 0,
			      ((pos - pl->first) * pl->fheight),
                              priv->width - 1,
                              ((pos - pl->first) * pl->fheight));
            }

        }

        /* When dropping on the borders of the playlist, outside the text area,
         * files get appended at the end of the list. Show that too.
         */

        if ((y < ply) || (y > priv->height + ply)) {
            if ((y >= 0) || (y <= (priv->height + ply))) {
                pos = plength;
                gdk_gc_set_foreground(gc, skin_get_color(bmp_active_skin, SKIN_PLEDIT_CURRENT));

                gdk_draw_line(obj, gc, 0, ((pos - pl->first) * pl->fheight),
                              priv->width - 1, ((pos - pl->first) * pl->fheight));

            }
        }
    }

    gdk_gc_set_foreground(gc, skin_get_color(bmp_active_skin, SKIN_PLEDIT_NORMAL));

    if (cfg.show_numbers_in_pl)
    {
        padding_plength = playlist_get_length(playlist);

        if (padding_plength == 0) {
            padding_dwidth = 0;
        }
        else {
            padding_dwidth = gint_count_digits(playlist_get_length(playlist));
        }

        padding =
            (padding_dwidth *
             width_approx_digits) + width_approx_digits;


        /* For italic or oblique fonts we add another half of the
         * approximate width */
        if (has_slant)
            padding += width_approx_digits_half;

        if (cfg.show_separator_in_pl) {
            gdk_draw_line(obj, gc, padding, 0, padding, priv->height - 1);
        }
    }

    if (tpadding_dwidth != 0)
    {
        tpadding = (tpadding_dwidth * width_approx_digits) + (width_approx_digits * 1.5);

        if (has_slant)
            tpadding += width_approx_digits_half;

        if (cfg.show_separator_in_pl) {
            gdk_draw_line(obj, gc, priv->width - tpadding, 0, priv->width - tpadding, priv->height - 1);
        }
    }

    gdk_gc_set_clip_origin(gc, 0, 0);
    gdk_gc_set_clip_rectangle(gc, NULL);

    PLAYLIST_UNLOCK(playlist);

    gdk_draw_drawable(widget->window, gc, obj, 0, 0, 0, 0, priv->width, priv->height);

    g_object_unref(obj);
    g_object_unref(gc);
    g_slice_free(GdkRectangle, playlist_rect);

    return FALSE;
}

gint ui_skinned_playlist_get_position(GtkWidget *widget, gint x, gint y) {
    gint iy, length;
    gint ret;
    Playlist *playlist = playlist_get_active();
    UiSkinnedPlaylist *pl = UI_SKINNED_PLAYLIST (widget);

    if (!pl->fheight)
        return -1;

    if ((length = playlist_get_length(playlist)) == 0)
        return -1;
    iy = y;

    ret = (iy / pl->fheight) + pl->first;

    if (ret > length - 1)
        ret = -1;

    return ret;
}

static gboolean ui_skinned_playlist_button_press(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedPlaylist *pl = UI_SKINNED_PLAYLIST (widget);
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(widget);

    gint nr;
    Playlist *playlist = playlist_get_active();

    nr = ui_skinned_playlist_get_position(widget, event->x, event->y);
    if (nr == -1)
        return FALSE;

    if (event->button == 3) {
        ui_manager_popup_menu_show(GTK_MENU(playlistwin_popup_menu),
                                   event->x_root, event->y_root + 5,
                                   event->button, event->time);
        GList* selection = playlist_get_selected(playlist);
        if (g_list_find(selection, GINT_TO_POINTER(nr)) == NULL) {
            playlist_select_all(playlist, FALSE);
            playlist_select_range(playlist, nr, nr, TRUE);
        }
    } else if (event->button == 1) {
        if (!(event->state & GDK_CONTROL_MASK))
            playlist_select_all(playlist, FALSE);

        if ((event->state & GDK_MOD1_MASK))
            playlist_queue_position(playlist, nr);

        if (event->state & GDK_SHIFT_MASK && pl->prev_selected != -1) {
            playlist_select_range(playlist, pl->prev_selected, nr, TRUE);
            pl->prev_min = pl->prev_selected;
            pl->prev_max = nr;
            priv->drag_pos = nr - pl->first;
        }
        else {
            if (playlist_select_invert(playlist, nr)) {
                if (event->state & GDK_CONTROL_MASK) {
                    if (pl->prev_min == -1) {
                        pl->prev_min = pl->prev_selected;
                        pl->prev_max = pl->prev_selected;
                    }
                    if (nr < pl->prev_min)
                        pl->prev_min = nr;
                    else if (nr > pl->prev_max)
                        pl->prev_max = nr;
                }
                else
                    pl->prev_min = -1;
                pl->prev_selected = nr;
                priv->drag_pos = nr - pl->first;
            }
        }
        if (event->type == GDK_2BUTTON_PRESS) {
            /*
             * Ungrab the pointer to prevent us from
             * hanging on to it during the sometimes slow
             * playback_initiate().
             */
            gdk_pointer_ungrab(GDK_CURRENT_TIME);
            playlist_set_position(playlist, nr);
            if (!playback_get_playing())
                playback_initiate();
        }

        priv->dragging = TRUE;
    }
    playlistwin_update_list(playlist);
    ui_skinned_playlist_popup_hide(widget);
    ui_skinned_playlist_popup_timer_stop(widget);

    return TRUE;
}

static gboolean ui_skinned_playlist_button_release(GtkWidget *widget, GdkEventButton *event) {
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(widget);

    if (event->button == 1) {
        priv->dragging = FALSE;
        priv->auto_drag_down = FALSE;
        priv->auto_drag_up = FALSE;
        gtk_widget_queue_draw(widget);
    }

    ui_skinned_playlist_popup_hide(widget);
    ui_skinned_playlist_popup_timer_stop(widget);
    return TRUE;
}

static gboolean ui_skinned_playlist_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
    UiSkinnedPlaylist *pl = UI_SKINNED_PLAYLIST(widget);
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(widget);

    gint nr, y, off, i;
    if (priv->dragging) {
        y = event->y;
        nr = (y / pl->fheight);
        if (nr < 0) {
            nr = 0;
            if (!priv->auto_drag_up) {
                priv->auto_drag_up = TRUE;
                priv->auto_drag_up_tag =
                    g_timeout_add(100, ui_skinned_playlist_auto_drag_up_func, pl);
            }
        }
        else if (priv->auto_drag_up)
            priv->auto_drag_up = FALSE;

        if (nr >= pl->num_visible) {
            nr = pl->num_visible - 1;
            if (!priv->auto_drag_down) {
                priv->auto_drag_down = TRUE;
                priv->auto_drag_down_tag =
                    g_timeout_add(100, ui_skinned_playlist_auto_drag_down_func, pl);
            }
        }
        else if (priv->auto_drag_down)
            priv->auto_drag_down = FALSE;

        off = nr - priv->drag_pos;
        if (off) {
            for (i = 0; i < abs(off); i++) {
                if (off < 0)
                    ui_skinned_playlist_move_up(pl);
                else
                    ui_skinned_playlist_move_down(pl);

            }
            playlistwin_update_list(playlist_get_active());
        }
        priv->drag_pos = nr;
    } else if (cfg.show_filepopup_for_tuple) {
        gint pos = ui_skinned_playlist_get_position(widget, event->x, event->y);
        gint cur_pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "popup_position"));
        if (pos != cur_pos) {
            g_object_set_data(G_OBJECT(widget), "popup_position", GINT_TO_POINTER(pos));
            ui_skinned_playlist_popup_hide(widget);
            ui_skinned_playlist_popup_timer_stop(widget);
            if (pos != -1)
                ui_skinned_playlist_popup_timer_start(widget);
        }
    }

    return TRUE;
}

static gboolean ui_skinned_playlist_leave_notify(GtkWidget *widget, GdkEventCrossing *event) {
    ui_skinned_playlist_popup_hide(widget);
    ui_skinned_playlist_popup_timer_stop(widget);

    return FALSE;
}

static void ui_skinned_playlist_redraw(UiSkinnedPlaylist *playlist) {
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(playlist);

    if (priv->resize_height || priv->resize_width)
        gtk_widget_set_size_request(GTK_WIDGET(playlist), priv->width+priv->resize_width, priv->height+priv->resize_height);

    gtk_widget_queue_draw(GTK_WIDGET(playlist));
}

void ui_skinned_playlist_set_font(const gchar * font) {
    /* Welcome to bad hack central 2k3 */
    gchar *font_lower;
    gint width_temp;
    gint width_temp_0;

    playlist_list_font = pango_font_description_from_string(font);

    text_get_extents(font,
                     "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz ",
                     &width_approx_letters, NULL, &ascent, &descent);

    width_approx_letters = (width_approx_letters / 53);

    /* Experimental: We don't weigh the 1 into total because it's width is almost always
     * very different from the rest
     */
    text_get_extents(font, "023456789", &width_approx_digits, NULL, NULL,
                     NULL);
    width_approx_digits = (width_approx_digits / 9);

    /* Precache some often used calculations */
    width_approx_digits_half = width_approx_digits / 2;

    /* FIXME: We assume that any other number is broader than the "1" */
    text_get_extents(font, "1", &width_temp, NULL, NULL, NULL);
    text_get_extents(font, "2", &width_temp_0, NULL, NULL, NULL);

    if (abs(width_temp_0 - width_temp) < 2) {
        width_delta_digit_one = 0;
    }
    else {
        width_delta_digit_one = ((width_temp_0 - width_temp) / 2) + 2;
    }

    text_get_extents(font, ":", &width_colon, NULL, NULL, NULL);
    width_colon_third = width_colon / 4;

    font_lower = g_utf8_strdown(font, strlen(font));
    /* This doesn't take any i18n into account, but i think there is none with TTF fonts
     * FIXME: This can probably be retrieved trough Pango too
     */
    has_slant = g_strstr_len(font_lower, strlen(font_lower), "oblique")
        || g_strstr_len(font_lower, strlen(font_lower), "italic");

    g_free(font_lower);
}

void ui_skinned_playlist_resize_relative(GtkWidget *widget, gint w, gint h) {
    UiSkinnedPlaylistPrivate *priv = UI_SKINNED_PLAYLIST_GET_PRIVATE(widget);
    priv->resize_width += w;
    priv->resize_height += h;
}

static gboolean ui_skinned_playlist_popup_show(gpointer data) {
    GtkWidget *widget = data;
    gint pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "popup_position"));

    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "timer_active")) == 1 && pos != -1) {
        Tuple *tuple;
        Playlist *pl_active = playlist_get_active();
        GtkWidget *popup = g_object_get_data(G_OBJECT(widget), "popup");

        tuple = playlist_get_tuple(pl_active, pos);
        if ((tuple == NULL) || (tuple_get_int(tuple, FIELD_LENGTH, NULL) < 1)) {
           gchar *title = playlist_get_songtitle(pl_active, pos);
           fileinfopopup_show_from_title(popup, title);
           g_free(title);
        } else {
           fileinfopopup_show_from_tuple(popup , tuple);
        }
        g_object_set_data(G_OBJECT(widget), "popup_active" , GINT_TO_POINTER(1));
    }

    ui_skinned_playlist_popup_timer_stop(widget);
    return FALSE;
}

static void ui_skinned_playlist_popup_hide(GtkWidget *widget) {
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "popup_active")) == 1) {
        GtkWidget *popup = g_object_get_data(G_OBJECT(widget), "popup");
        g_object_set_data(G_OBJECT(widget), "popup_active", GINT_TO_POINTER(0));
        fileinfopopup_hide(popup, NULL);
    }
}

static void ui_skinned_playlist_popup_timer_start(GtkWidget *widget) {
    gint timer_id = g_timeout_add(cfg.filepopup_delay*100, ui_skinned_playlist_popup_show, widget);
    g_object_set_data(G_OBJECT(widget), "timer_id", GINT_TO_POINTER(timer_id));
    g_object_set_data(G_OBJECT(widget), "timer_active", GINT_TO_POINTER(1));
}

static void ui_skinned_playlist_popup_timer_stop(GtkWidget *widget) {
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "timer_active")) == 1)
        g_source_remove(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "timer_id")));

    g_object_set_data(G_OBJECT(widget), "timer_id", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(widget), "timer_active", GINT_TO_POINTER(0));
}
