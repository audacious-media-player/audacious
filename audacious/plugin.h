/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Please see the BMP Wiki for information about the plugin interface */

#ifndef BMP_PLUGIN_H
#define BMP_PLUGIN_H


#include <glib.h>
#include "libaudacious/titlestring.h"

#define INPUT_PLUGIN(x)   ((InputPlugin *)(x))
#define OUTPUT_PLUGIN(x)  ((OutputPlugin *)(x))
#define EFFECT_PLUGIN(x)  ((EffectPlugin *)(x))
#define GENERAL_PLUGIN(x) ((GeneralPlugin *)(x))
#define VIS_PLUGIN(x)     ((VisPlugin *)(x))


typedef enum {
    FMT_U8,
    FMT_S8,
    FMT_U16_LE,
    FMT_U16_BE,
    FMT_U16_NE,
    FMT_S16_LE,
    FMT_S16_BE,
    FMT_S16_NE
} AFormat;

typedef enum {
    INPUT_VIS_ANALYZER,
    INPUT_VIS_SCOPE,
    INPUT_VIS_VU,
    INPUT_VIS_OFF
} InputVisType;


typedef struct _Plugin        Plugin;
typedef struct _InputPlugin   InputPlugin;
typedef struct _OutputPlugin  OutputPlugin;
typedef struct _EffectPlugin  EffectPlugin;
typedef struct _GeneralPlugin GeneralPlugin;
typedef struct _VisPlugin     VisPlugin;

/* Sadly, this is the most we can generalize out of the disparate
   plugin structs usable with typecasts - descender */
struct _Plugin {
    gpointer handle;
    gchar *filename;
};

struct _OutputPlugin {
    gpointer handle;
    gchar *filename;

    gchar *description;

    void (*init) (void);
    void (*cleanup) (void);
    void (*about) (void);
    void (*configure) (void);
    void (*get_volume) (gint * l, gint * r);
    void (*set_volume) (gint l, gint r);

    gint (*open_audio) (AFormat fmt, gint rate, gint nch);
    void (*write_audio) (gpointer ptr, gint length);
    void (*close_audio) (void);

    void (*flush) (gint time);
    void (*pause) (gshort paused);
    gint (*buffer_free) (void);
    gint (*buffer_playing) (void);
    gint (*output_time) (void);
    gint (*written_time) (void);

    void (*tell_audio) (AFormat * fmt, gint * rate, gint * nch);
};

struct _EffectPlugin {
    gpointer handle;
    gchar *filename;

    gchar *description;

    void (*init) (void);
    void (*cleanup) (void);
    void (*about) (void);
    void (*configure) (void);

    gint (*mod_samples) (gpointer * data, gint length, AFormat fmt, gint srate, gint nch);
    void (*query_format) (AFormat * fmt, gint * rate, gint * nch);
};

struct _InputPlugin {
    gpointer handle;
    gchar *filename;

    gchar *description;

    void (*init) (void);
    void (*about) (void);
    void (*configure) (void);

    gint (*is_our_file) (gchar * filename);
    GList *(*scan_dir) (gchar * dirname);

    void (*play_file) (gchar * filename);
    void (*stop) (void);
    void (*pause) (gshort paused);
    void (*seek) (gint time);

    void (*set_eq) (gint on, gfloat preamp, gfloat * bands);

    gint (*get_time) (void);

    void (*get_volume) (gint * l, gint * r);
    void (*set_volume) (gint l, gint r);

    void (*cleanup) (void);

    InputVisType (*get_vis_type) (void);
    void (*add_vis_pcm) (gint time, AFormat fmt, gint nch, gint length, gpointer ptr);

    void (*set_info) (gchar * title, gint length, gint rate, gint freq, gint nch);
    void (*set_info_text) (gchar * text);
    void (*get_song_info) (gchar * filename, gchar ** title, gint * length);
    void (*file_info_box) (gchar * filename);

    OutputPlugin *output;

    TitleInput *(*get_song_tuple) (gchar * filename);
    void (*set_song_tuple) (TitleInput * tuple);
};

struct _GeneralPlugin {
    gpointer handle;
    gchar *filename;

    gint xmms_session;
    gchar *description;

    void (*init) (void);
    void (*about) (void);
    void (*configure) (void);
    void (*cleanup) (void);
};

struct _VisPlugin {
    gpointer handle;
    gchar *filename;

    gint xmms_session;
    gchar *description;

    gint num_pcm_chs_wanted;
    gint num_freq_chs_wanted;

    void (*init) (void);
    void (*cleanup) (void);
    void (*about) (void);
    void (*configure) (void);
    void (*disable_plugin) (struct _VisPlugin *);
    void (*playback_start) (void);
    void (*playback_stop) (void);
    void (*render_pcm) (gint16 pcm_data[2][512]);
    void (*render_freq) (gint16 freq_data[2][256]);
};


G_BEGIN_DECLS

/* So that input plugins can get the title formatting information */
G_CONST_RETURN gchar * xmms_get_gentitle_format(void);

/* So that output plugins can communicate with effect plugins */
EffectPlugin *get_current_effect_plugin(void);
gboolean effects_enabled(void);
gboolean plugin_set_errortext(const gchar * text);

G_END_DECLS

#endif
