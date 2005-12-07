
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

#undef PRESET_SUPPORT

#include "crossfade.h"
#include "configure.h"
#include "interface.h"
#include "monitor.h"
#include "support.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef HAVE_LIBSAMPLERATE
#  include <samplerate.h>
#endif


#define HIDE(name)					\
{ if((set_wgt = lookup_widget(config_win, name)))	\
    gtk_widget_hide(set_wgt); }

#define SHOW(name)					\
{ if((set_wgt = lookup_widget(config_win, name)))	\
    gtk_widget_show(set_wgt); }


#define SETW_SENSITIVE(wgt, sensitive)		\
  gtk_widget_set_sensitive(wgt, sensitive)

#define SETW_TOGGLE(wgt, active)				\
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wgt), active)

#define SETW_SPIN(wgt, value)					\
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(wgt), value)


#define SET_SENSITIVE(name, sensitive)			\
{ if((set_wgt = lookup_widget(config_win, name)))	\
    gtk_widget_set_sensitive(set_wgt, sensitive); }

#define SET_TOGGLE(name, active)					\
{ if((set_wgt = lookup_widget(config_win, name)))			\
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(set_wgt), active); }

#define SET_SPIN(name, value)						\
{ if((set_wgt = lookup_widget(config_win, name)))			\
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(set_wgt), value); }

#define SET_PAGE(name, index)					\
{ if((set_wgt = lookup_widget(config_win, name)))		\
    gtk_notebook_set_page(GTK_NOTEBOOK(set_wgt), index); }

#define SET_HISTORY(name, index)					\
{ if((set_wgt = lookup_widget(config_win, name)))			\
    gtk_option_menu_set_history(GTK_OPTION_MENU(set_wgt), index); }


#define GET_SENSITIVE(name)			\
((get_wgt = lookup_widget(config_win, name))	\
  && GTK_WIDGET_SENSITIVE(get_wgt))		\

#define GET_TOGGLE(name)					\
((get_wgt = lookup_widget(config_win, name))			\
  && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(get_wgt)))

#define GET_SPIN(name)							\
((get_wgt = lookup_widget(config_win, name))				\
  ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(get_wgt)) : 0)


static GtkWidget *config_win = NULL;
static GtkWidget *about_win = NULL;
static GtkWidget *set_wgt;
static GtkWidget *get_wgt;

/* init with DEFAULT_CFG to make sure all string pointers are set to NULL */
static config_t _cfg = CONFIG_DEFAULT;
static config_t *cfg = &_cfg; 

/* some helpers to keep track of the GUI's state */
static gboolean checking = FALSE;
static gint            op_index;
static plugin_config_t op_config;
static gint            ep_index;


static void update_plugin_config(gchar **config_string, gchar *name,
				 plugin_config_t *pc, gboolean save);

/*****************************************************************************/

void g_free_f(gpointer data, gpointer user_data)
{
  g_free(data);
}

/*****************************************************************************/

#ifdef PRESET_SUPPORT
static void
scan_presets(gchar *filename)
{
  struct stat stats;
  FILE       *fh;
  gchar      *data, **lines, *tmp, *name;
  int         i;

  if(lstat(filename, &stats)) {
    DEBUG(("[crossfade] scan_presets: \"%s\":\n", filename));
    PERROR("[crossfade] scan_presets: lstat");
    return;
  }
  if(stats.st_size <= 0) return;

  if(!(data = g_malloc(stats.st_size + 1))) {
    DEBUG(("[crossfade] scan_presets: g_malloc(%ld) failed!\n", stats.st_size));
    return;
  }

  if(!(fh = fopen(filename, "r"))) {
    PERROR("[crossfade] scan_presets: fopen");
    g_free(data);
    return;
  }

  if(fread(data, stats.st_size, 1, fh) != 1) {
    DEBUG(("[crossfade] scan_presets: fread() failed!\n")); 
    g_free(data);
    fclose(fh);
    return;
  }
  fclose(fh);
  data[stats.st_size] = 0;

  lines = g_strsplit(data, "\n", 0);
  g_free(data);

  if(!lines) {
    DEBUG(("[crossfade] scan_presets: g_strsplit() failed!\n"));
    return;
  }

  g_list_foreach(config->presets, g_free_f, NULL);
  g_list_free(config->presets);
  config->presets = NULL;

  for(i=0; lines[i]; i++) {
    if(lines[i][0] == '[') {
      if((tmp = strchr(lines[i], ']'))) {
	*tmp = 0;
	if((name = g_strdup(lines[i]+1)))
	  config->presets = g_list_append(config->presets, name);
      }
    }
  }

  g_strfreev(lines);
}
#endif

static void
read_fade_config(ConfigFile *cfgfile, gchar *section, gchar *key, fade_config_t *fc)
{
  gchar *s = NULL;
  gint n;
  
  if(!cfgfile || !section || !key || !fc) return;

  xmms_cfg_read_string(cfgfile, section, key, &s);
  if(!s) return;

  n = sscanf(s,
	     "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
	     &fc->type,
	     &fc->pause_len_ms,
	     &fc->simple_len_ms,
	     &fc->out_enable,
	     &fc->out_len_ms,
	     &fc->out_volume,
	     &fc->ofs_type,
	     &fc->ofs_type_wanted,
	     &fc->ofs_custom_ms,
	     &fc->in_locked,
	     &fc->in_enable,
	     &fc->in_len_ms,
	     &fc->in_volume,
	     &fc->flush_pause_enable,
	     &fc->flush_pause_len_ms,
	     &fc->flush_in_enable,
	     &fc->flush_in_len_ms,
	     &fc->flush_in_volume);

  g_free(s);
}

static void
write_fade_config(ConfigFile *cfgfile, gchar *section, gchar *key, fade_config_t *fc)
{
  gchar *s;
  
  if(!cfgfile || !section || !key || !fc) return;

  s = g_strdup_printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		      fc->type,
		      fc->pause_len_ms,
		      fc->simple_len_ms,
		      fc->out_enable,
		      fc->out_len_ms,
		      fc->out_volume,
		      fc->ofs_type,
		      fc->ofs_type_wanted,
		      fc->ofs_custom_ms,
		      fc->in_locked,
		      fc->in_enable,
		      fc->in_len_ms,
		      fc->in_volume,
		      fc->flush_pause_enable,
		      fc->flush_pause_len_ms,
		      fc->flush_in_enable,
		      fc->flush_in_len_ms,
		      fc->flush_in_volume);

  if(!s) return;

  xmms_cfg_write_string(cfgfile, section, key, s);
  g_free(s);
}

void
xfade_load_config()
{
#ifdef PRESET_SUPPORT
  gchar      *filename;
#endif
  gchar      *section = "Crossfade";
  ConfigFile *cfgfile;

  if((cfgfile = xmms_cfg_open_default_file())) {
    /* config items used in v0.1 */
    xmms_cfg_read_string (cfgfile, section, "output_plugin",        &config->op_name);
    xmms_cfg_read_string (cfgfile, section, "op_config_string",     &config->op_config_string);
    xmms_cfg_read_int    (cfgfile, section, "buffer_size",          &config->mix_size_ms);
    xmms_cfg_read_int    (cfgfile, section, "sync_size",            &config->sync_size_ms);
    xmms_cfg_read_int    (cfgfile, section, "preload_size",         &config->preload_size_ms);
    xmms_cfg_read_int    (cfgfile, section, "songchange_timeout",   &config->songchange_timeout);
    xmms_cfg_read_boolean(cfgfile, section, "enable_mixer",         &config->enable_mixer);
    xmms_cfg_read_boolean(cfgfile, section, "mixer_reverse",        &config->mixer_reverse);
    xmms_cfg_read_boolean(cfgfile, section, "enable_debug",         &config->enable_debug);
    xmms_cfg_read_boolean(cfgfile, section, "enable_monitor",       &config->enable_monitor);

    /* config items introduced by v0.2 */
    xmms_cfg_read_boolean(cfgfile, section, "gap_lead_enable",      &config->gap_lead_enable);
    xmms_cfg_read_int    (cfgfile, section, "gap_lead_len_ms",      &config->gap_lead_len_ms);
    xmms_cfg_read_int    (cfgfile, section, "gap_lead_level",       &config->gap_lead_level);
    xmms_cfg_read_boolean(cfgfile, section, "gap_trail_enable",     &config->gap_trail_enable);
    xmms_cfg_read_int    (cfgfile, section, "gap_trail_len_ms",     &config->gap_trail_len_ms);
    xmms_cfg_read_int    (cfgfile, section, "gap_trail_level",      &config->gap_trail_level);
    xmms_cfg_read_int    (cfgfile, section, "gap_trail_locked",     &config->gap_trail_locked);
    
    /* config items introduced by v0.2.1 */
    xmms_cfg_read_boolean(cfgfile, section, "buffer_size_auto",     &config->mix_size_auto);
    
    /* config items introduced by v0.2.3 */
    xmms_cfg_read_boolean(cfgfile, section, "album_detection",      &config->album_detection);
      
    /* config items introduced by v0.2.4 */
    xmms_cfg_read_boolean(cfgfile, section, "http_workaround",      &config->enable_http_workaround);
    xmms_cfg_read_boolean(cfgfile, section, "enable_op_max_used",   &config->enable_op_max_used);
    xmms_cfg_read_int    (cfgfile, section, "op_max_used_ms",       &config->op_max_used_ms);
    
    /* config items introduced by v0.2.6 */
    xmms_cfg_read_string (cfgfile, section, "effect_plugin",        &config->ep_name);
    xmms_cfg_read_boolean(cfgfile, section, "effect_enable",        &config->ep_enable);
    xmms_cfg_read_int    (cfgfile, section, "output_rate",          &config->output_rate);
    
    /* config items introduced by v0.3.0 */
    xmms_cfg_read_boolean(cfgfile, section, "volnorm_enable",       &config->volnorm_enable);
    xmms_cfg_read_boolean(cfgfile, section, "volnorm_use_qa",       &config->volnorm_use_qa);
    xmms_cfg_read_int    (cfgfile, section, "volnorm_target",       &config->volnorm_target);
    xmms_cfg_read_boolean(cfgfile, section, "output_keep_opened",   &config->output_keep_opened);
    xmms_cfg_read_boolean(cfgfile, section, "mixer_software",       &config->mixer_software);
    xmms_cfg_read_int    (cfgfile, section, "mixer_vol_left",       &config->mixer_vol_left);
    xmms_cfg_read_int    (cfgfile, section, "mixer_vol_right",      &config->mixer_vol_right);
    
    /* config items introduced by v0.3.2 */
    xmms_cfg_read_boolean(cfgfile, section, "no_xfade_if_same_file",&config->no_xfade_if_same_file);

    /* config items introduced by v0.3.3 */
    xmms_cfg_read_boolean(cfgfile, section, "gap_crossing",         &config->gap_crossing);

    /* config items introduced by v0.3.6 */
    xmms_cfg_read_int    (cfgfile, section, "output_quality",       &config->output_quality);

    /* fade configs */
    read_fade_config(cfgfile, section, "fc_xfade",   &config->fc[FADE_CONFIG_XFADE]);
    read_fade_config(cfgfile, section, "fc_manual",  &config->fc[FADE_CONFIG_MANUAL]);
    read_fade_config(cfgfile, section, "fc_album",   &config->fc[FADE_CONFIG_ALBUM]);
    read_fade_config(cfgfile, section, "fc_start",   &config->fc[FADE_CONFIG_START]);
    read_fade_config(cfgfile, section, "fc_stop",    &config->fc[FADE_CONFIG_STOP]);
    read_fade_config(cfgfile, section, "fc_eop",     &config->fc[FADE_CONFIG_EOP]);
    read_fade_config(cfgfile, section, "fc_seek",    &config->fc[FADE_CONFIG_SEEK]);
    read_fade_config(cfgfile, section, "fc_pause",   &config->fc[FADE_CONFIG_PAUSE]);
    
    xmms_cfg_free(cfgfile);
    DEBUG(("[crossfade] load_config: configuration loaded\n"));
  }
  else
    DEBUG(("[crossfade] load_config: error loading config, using defaults\n"));

#ifdef PRESET_SUPPORT
  filename = g_strconcat(g_get_home_dir(), "/.xmms/xmms-crossfade-presets", NULL);
  scan_presets(filename);
  g_free(filename);
#endif
}

void
xfade_save_config()
{
  gchar      *section = "Crossfade";
  ConfigFile *cfgfile;

  if((cfgfile = xmms_cfg_open_default_file())) {
    /* obsolete config items */
    xmms_cfg_remove_key(cfgfile, section, "underrun_pct");
    xmms_cfg_remove_key(cfgfile, section, "enable_crossfade");
    xmms_cfg_remove_key(cfgfile, section, "enable_gapkiller");
    xmms_cfg_remove_key(cfgfile, section, "mixer_use_master");
    xmms_cfg_remove_key(cfgfile, section, "late_effect");
    xmms_cfg_remove_key(cfgfile, section, "gap_lead_length");
      
    /* config items used in v0.1 */
    xmms_cfg_write_string (cfgfile, section, "output_plugin",        config->op_name ? config->op_name : DEFAULT_OP_NAME);
    xmms_cfg_write_string (cfgfile, section, "op_config_string",     config->op_config_string ? config->op_config_string : DEFAULT_OP_CONFIG_STRING);
    xmms_cfg_write_int    (cfgfile, section, "buffer_size",          config->mix_size_ms);
    xmms_cfg_write_int    (cfgfile, section, "sync_size",            config->sync_size_ms);
    xmms_cfg_write_int    (cfgfile, section, "preload_size",         config->preload_size_ms);
    xmms_cfg_write_int    (cfgfile, section, "songchange_timeout",   config->songchange_timeout);
    xmms_cfg_write_boolean(cfgfile, section, "enable_mixer",         config->enable_mixer);
    xmms_cfg_write_boolean(cfgfile, section, "mixer_reverse",        config->mixer_reverse);
    xmms_cfg_write_boolean(cfgfile, section, "enable_debug",         config->enable_debug);
    xmms_cfg_write_boolean(cfgfile, section, "enable_monitor",       config->enable_monitor);
    
    /* config items introduced by v0.2 */
    xmms_cfg_write_boolean(cfgfile, section, "gap_lead_enable",      config->gap_lead_enable);
    xmms_cfg_write_int    (cfgfile, section, "gap_lead_len_ms",      config->gap_lead_len_ms);
    xmms_cfg_write_int    (cfgfile, section, "gap_lead_level",       config->gap_lead_level);
    xmms_cfg_write_boolean(cfgfile, section, "gap_trail_enable",     config->gap_trail_enable);
    xmms_cfg_write_int    (cfgfile, section, "gap_trail_len_ms",     config->gap_trail_len_ms);
    xmms_cfg_write_int    (cfgfile, section, "gap_trail_level",      config->gap_trail_level);
    xmms_cfg_write_int    (cfgfile, section, "gap_trail_locked",     config->gap_trail_locked);
    
    /* config items introduced by v0.2.1 */
    xmms_cfg_write_boolean(cfgfile, section, "buffer_size_auto",     config->mix_size_auto);
    
    /* config items introduced by v0.2.3 */
    xmms_cfg_write_boolean(cfgfile, section, "album_detection",      config->album_detection);
    
    /* config items introduced by v0.2.4 */
    xmms_cfg_write_boolean(cfgfile, section, "http_workaround",      config->enable_http_workaround);
    xmms_cfg_write_boolean(cfgfile, section, "enable_op_max_used",   config->enable_op_max_used);
    xmms_cfg_write_int    (cfgfile, section, "op_max_used_ms",       config->op_max_used_ms);
    
    /* config items introduced by v0.2.6 */
    xmms_cfg_write_string (cfgfile, section, "effect_plugin",        config->ep_name ? config->ep_name : DEFAULT_EP_NAME);
    xmms_cfg_write_boolean(cfgfile, section, "effect_enable",        config->ep_enable);
    xmms_cfg_write_int    (cfgfile, section, "output_rate",          config->output_rate);
   
    /* config items introduced by v0.3.0 */
#ifdef VOLUME_NORMALIZER
    xmms_cfg_write_boolean(cfgfile, section, "volnorm_enable",       config->volnorm_enable);
    xmms_cfg_write_boolean(cfgfile, section, "volnorm_use_qa",       config->volnorm_use_qa);
    xmms_cfg_write_int    (cfgfile, section, "volnorm_target",       config->volnorm_target);
#endif
    xmms_cfg_write_boolean(cfgfile, section, "output_keep_opened",   config->output_keep_opened);
    xmms_cfg_write_boolean(cfgfile, section, "mixer_software",       config->mixer_software);
    xmms_cfg_write_int    (cfgfile, section, "mixer_vol_left",       config->mixer_vol_left);
    xmms_cfg_write_int    (cfgfile, section, "mixer_vol_right",      config->mixer_vol_right);

    /* config items introduced by v0.3.2 */
    xmms_cfg_write_boolean(cfgfile, section, "no_xfade_if_same_file",config->no_xfade_if_same_file);

    /* config items introduced by v0.3.2 */
    xmms_cfg_write_boolean(cfgfile, section, "gap_crossing",         config->gap_crossing);

    /* config items introduced by v0.3.6 */
    xmms_cfg_write_int    (cfgfile, section, "output_quality",       config->output_quality);

    /* fade configs */
    write_fade_config(cfgfile, section, "fc_xfade",   &config->fc[FADE_CONFIG_XFADE]);
    write_fade_config(cfgfile, section, "fc_manual",  &config->fc[FADE_CONFIG_MANUAL]);
    write_fade_config(cfgfile, section, "fc_album",   &config->fc[FADE_CONFIG_ALBUM]);
    write_fade_config(cfgfile, section, "fc_start",   &config->fc[FADE_CONFIG_START]);
    write_fade_config(cfgfile, section, "fc_stop",    &config->fc[FADE_CONFIG_STOP]);
    write_fade_config(cfgfile, section, "fc_eop",     &config->fc[FADE_CONFIG_EOP]);
    write_fade_config(cfgfile, section, "fc_seek",    &config->fc[FADE_CONFIG_SEEK]);
    write_fade_config(cfgfile, section, "fc_pause",   &config->fc[FADE_CONFIG_PAUSE]);
    
    xmms_cfg_write_default_file(cfgfile);
    xmms_cfg_free              (cfgfile);
    DEBUG(("[crossfade] save_config: configuration saved\n"));
  }
  else
    DEBUG(("[crossfade] save_config: error saving configuration!\n"));
}

#define SAFE_FREE(x) if(x) { g_free(x); x = NULL; }
void
xfade_free_config()
{
  SAFE_FREE(cfg->op_config_string);
  SAFE_FREE(cfg->op_name);

  g_list_foreach(config->presets, g_free_f, NULL);
  g_list_free(config->presets);
  config->presets = NULL;
}

void
xfade_load_plugin_config(gchar *config_string,
			 gchar *plugin_name,
			 plugin_config_t *plugin_config)
{
  update_plugin_config(&config_string, plugin_name, plugin_config, FALSE);
}

void
xfade_save_plugin_config(gchar **config_string,
			 gchar *plugin_name,
			 plugin_config_t *plugin_config)
{
  update_plugin_config(config_string, plugin_name, plugin_config, TRUE);
}

/*** helpers *****************************************************************/

gint
xfade_cfg_fadeout_len(fade_config_t *fc)
{
  if(!fc) return 0;
  switch(fc->type) {
  case FADE_TYPE_SIMPLE_XF:
    return fc->simple_len_ms;
  case FADE_TYPE_ADVANCED_XF:
    return fc->out_enable ? fc->out_len_ms : 0;
  case FADE_TYPE_FADEOUT:
  case FADE_TYPE_PAUSE_ADV:
    return fc->out_len_ms;
  }
  return 0;
}

gint
xfade_cfg_fadeout_volume(fade_config_t *fc)
{
  gint volume;
  if(!fc) return 0;
  switch(fc->type) {
  case FADE_TYPE_ADVANCED_XF:
  case FADE_TYPE_FADEOUT:
    volume = fc->out_volume;
    if(volume <   0) volume =   0;
    if(volume > 100) volume = 100;
    return volume;
  }
  return 0;
}

gint
xfade_cfg_offset(fade_config_t *fc)
{
  if(!fc) return 0;
  switch(fc->type) {
  case FADE_TYPE_FLUSH:
    return fc->flush_pause_enable ? fc->flush_pause_len_ms : 0;
  case FADE_TYPE_PAUSE:
    return fc->pause_len_ms;
  case FADE_TYPE_SIMPLE_XF:
    return -fc->simple_len_ms;
  case FADE_TYPE_ADVANCED_XF:
    switch(fc->ofs_type) {
    case FC_OFFSET_LOCK_OUT:
      return -fc->out_len_ms;
    case FC_OFFSET_LOCK_IN:
      return -fc->in_len_ms;
    case FC_OFFSET_CUSTOM:
      return fc->ofs_custom_ms;
    }
    return 0;
  case FADE_TYPE_FADEOUT:
  case FADE_TYPE_PAUSE_ADV:
    return fc->ofs_custom_ms;
  }
  return 0;
}

gint
xfade_cfg_fadein_len(fade_config_t *fc)
{
  if(!fc) return 0;
  switch(fc->type) {
  case FADE_TYPE_FLUSH:
    return fc->flush_in_enable ? fc->flush_in_len_ms : 0;
  case FADE_TYPE_SIMPLE_XF:
    return fc->simple_len_ms;
  case FADE_TYPE_ADVANCED_XF:
    return
      fc->in_locked
      ? (fc->out_enable ? fc->out_len_ms : 0)
      : (fc->in_enable  ? fc->in_len_ms  : 0);
  case FADE_TYPE_FADEIN:
  case FADE_TYPE_PAUSE_ADV:
    return fc->in_len_ms;
  }
  return 0;
}

gint
xfade_cfg_fadein_volume(fade_config_t *fc)
{
  gint volume;
  if(!fc) return 0;
  switch(fc->type) {
  case FADE_TYPE_FLUSH:
    volume = fc->flush_in_volume;
    break;
  case FADE_TYPE_ADVANCED_XF:
    volume = fc->in_locked ? fc->out_volume : fc->in_volume;
    break;
  case FADE_TYPE_FADEIN:
    volume = fc->in_volume;
    break;
  default:
    volume = 0;
  }
  if(volume <   0) volume =   0;
  if(volume > 100) volume = 100;
  return volume;
}

gboolean
xfade_cfg_gap_trail_enable(config_t *cfg)
{
  return cfg->gap_trail_locked
    ? cfg->gap_lead_enable
    : cfg->gap_trail_enable;
}

gint
xfade_cfg_gap_trail_len(config_t *cfg)
{
  if(!xfade_cfg_gap_trail_enable(cfg)) return 0;
  return cfg->gap_trail_locked
    ? cfg->gap_lead_len_ms
    : cfg->gap_trail_len_ms;
}

gint
xfade_cfg_gap_trail_level(config_t *cfg)
{
  return cfg->gap_trail_locked
    ? cfg->gap_lead_level
    : cfg->gap_trail_level;
}

gint
xfade_mix_size_ms(config_t *cfg)
{
  if(cfg->mix_size_auto) {
    gint i, min_size = 0;

    for(i=0; i<MAX_FADE_CONFIGS; i++) {
      gint size   = xfade_cfg_fadeout_len(&cfg->fc[i]);
      gint offset = xfade_cfg_offset(&cfg->fc[i]);

      if(cfg->fc[i].type == FADE_TYPE_PAUSE_ADV)
	size += xfade_cfg_fadein_len(&cfg->fc[i]);

      if(size < -offset)
	size = -offset;

      if(size > min_size)
	min_size = size;
    }
    return min_size += xfade_cfg_gap_trail_len(cfg)
      +                cfg->songchange_timeout;
  }
  else
    return cfg->mix_size_ms;
}

/*** internal helpers ********************************************************/

static void
add_menu_item(GtkWidget *menu, gchar *title, GtkSignalFunc func, gint index, gint **imap)
{
  GtkWidget *item;
  if(!menu || !title || !func) return;
  item = gtk_menu_item_new_with_label(title);
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(func), (gpointer)index);
  gtk_widget_show(item);
  gtk_menu_append(GTK_MENU(menu), item);

  if(imap) *((*imap)++) = index;
}

/*** output method ***********************************************************/

/*-- callbacks --------------------------------------------------------------*/

static void resampling_rate_cb(GtkWidget *widget, gint index)
{
  cfg->output_rate = index;
}

#ifdef HAVE_LIBSAMPLERATE
static void resampling_quality_cb(GtkWidget *widget, gint index)
{
  cfg->output_quality = index;
}
#endif

/*** plugin output ***********************************************************/

static gchar *
strip(gchar *s)
{
  gchar *p;
  if(!s) return NULL;
  for(; *s == ' '; s++);
  if(!*s) return s;
  for(p = s+strlen(s)-1; *p == ' '; p--);
  *++p = 0;
  return s;
}

static void
update_plugin_config(gchar **config_string, gchar *name,
		     plugin_config_t *pc, gboolean save)
{
  plugin_config_t default_pc = DEFAULT_OP_CONFIG;

  gchar *buffer = NULL;
  gchar  out[1024];

  gboolean plugin_found = FALSE;
  gchar   *plugin, *next_plugin;
  gchar   *args;

  if(pc && !save) *pc = default_pc;
  if(!config_string || !*config_string || !name || !pc) {
    DEBUG(("[crossfade] update_plugin_config: missing arg!\n"));
    return;
  }

  buffer = g_strdup(*config_string);
  out[0] = 0;

  for(plugin = buffer; plugin; plugin = next_plugin) {
    if((next_plugin = strchr(plugin, ';'))) *next_plugin++ = 0;
    if((args = strchr(plugin, '='))) *args++ = 0;
    plugin = strip(plugin);
    if(!*plugin || !args || !*args) continue;

    if(save) {
      if(0 == strcmp(plugin, name)) continue;
      if(*out) strcat(out, "; ");
      strcat(out, plugin);
      strcat(out, "=");
      strcat(out, args);
      continue;
    }
    else if(strcmp(plugin, name)) continue;

    args = strip(args);
    sscanf(args, "%d,%d,%d,%d",
	   &pc->throttle_enable,
	   &pc->max_write_enable,
	   &pc->max_write_len,
	   &pc->force_reopen);
    pc->max_write_len &= -4;
    plugin_found = TRUE;
  }

  if(save) {
    /* only save if settings differ from defaults */
    if((  pc->throttle_enable  != default_pc.throttle_enable)
       ||(pc->max_write_enable != default_pc.max_write_enable)
       ||(pc->max_write_len    != default_pc.max_write_len)
       ||(pc->force_reopen     != default_pc.force_reopen)) {
      if(*out) strcat(out, "; ");
      sprintf(out + strlen(out), "%s=%d,%d,%d,%d", name,
	      pc->throttle_enable ? 1 : 0,
	      pc->max_write_enable ? 1 : 0,
	      pc->max_write_len,
	      pc->force_reopen);
    }
    if(*config_string) g_free(*config_string);
    *config_string = g_strdup(out);
  }

  g_free(buffer);
}

static void
config_plugin_cb(GtkWidget *widget, gint index);

static gint
scan_plugins(GtkWidget *option_menu, gchar *selected)
{
  GtkWidget *menu = gtk_menu_new();
  GList     *list = g_list_first(get_output_list());  /* XMMS */
  gint      index = 0;
  gint  sel_index = -1;
  gint  def_index = -1;

  /* sanity check */
  if(selected == NULL) selected = "";

  /* parse module list */
  while(list) {
    OutputPlugin *op = (OutputPlugin *)list->data;
    GtkWidget  *item = gtk_menu_item_new_with_label(op->description);
    
    if(op == get_crossfade_oplugin_info())  /* disable selecting ourselves */
      gtk_widget_set_sensitive(item, FALSE);
    else {
      if(def_index == -1) def_index = index;
      if(selected && !strcmp(g_basename(op->filename), selected))
	sel_index = index;
    }

    /* create menu item */
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       GTK_SIGNAL_FUNC(config_plugin_cb), (gpointer)index++);
    gtk_widget_show(item);
    gtk_menu_append(GTK_MENU(menu), item);

    /* advance to next module */
    list = g_list_next(list);
  }
    
  /* attach menu */
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  
  if(sel_index == -1) {
    DEBUG(("[crossfade] scan_plugins: plugin not found (\"%s\")\n", selected));
    return def_index;  /* use default (first entry) */
  }
  return sel_index;
}

/*-- plugin output callbacks ------------------------------------------------*/

static void
config_plugin_cb(GtkWidget *widget, gint index)
{
  OutputPlugin *op = g_list_nth_data(get_output_list(), index);  /* XMMS */

  /* get plugin options from gui */
  op_config.throttle_enable  = GET_TOGGLE("op_throttle_check");
  op_config.max_write_enable = GET_TOGGLE("op_maxblock_check");
  op_config.max_write_len    = GET_SPIN  ("op_maxblock_spin");
  op_config.force_reopen     = GET_TOGGLE("op_forcereopen_check");

  /* config -> string */
  xfade_save_plugin_config(&cfg->op_config_string, cfg->op_name, &op_config);

  /* select new plugin */
  op_index = index;

  /* get new plugin's name */
  if(cfg->op_name) g_free(cfg->op_name);
  cfg->op_name = (op && op->filename) 
    ? g_strdup(g_basename(op->filename)) : NULL;

  /* string -> config */
  xfade_load_plugin_config(cfg->op_config_string, cfg->op_name, &op_config);

  /* update gui */
  SET_SENSITIVE("op_configure_button",  op && (op->configure != NULL)); 
  SET_SENSITIVE("op_about_button",      op && (op->about != NULL));
  SET_TOGGLE   ("op_throttle_check",    op_config.throttle_enable);
  SET_TOGGLE   ("op_maxblock_check",    op_config.max_write_enable);
  SET_SPIN     ("op_maxblock_spin",     op_config.max_write_len); 
  SET_SENSITIVE("op_maxblock_spin",     op_config.max_write_enable);
  SET_TOGGLE   ("op_forcereopen_check", op_config.force_reopen);
}

void on_output_plugin_configure_button_clicked (GtkButton *button, gpointer user_data)
{
  OutputPlugin *op = g_list_nth_data(get_output_list(), op_index);  /* XMMS */
  if((op == NULL) || (op->configure == NULL)) return;
  op->configure();
}

void on_output_plugin_about_button_clicked(GtkButton *button, gpointer user_data)
{
  OutputPlugin *op = g_list_nth_data(get_output_list(), op_index);  /* XMMS */
  if((op == NULL) || (op->about == NULL)) return;
  op->about();
}

void on_op_throttle_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  op_config.throttle_enable = gtk_toggle_button_get_active(togglebutton);
}

void on_op_maxblock_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  op_config.max_write_enable = gtk_toggle_button_get_active(togglebutton);
  SET_SENSITIVE("op_maxblock_spin", op_config.max_write_enable);
}

void on_op_maxblock_spin_changed(GtkEditable *editable, gpointer user_data)
{
  op_config.max_write_len = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
}

void on_op_forcereopen_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  op_config.max_write_enable = gtk_toggle_button_get_active(togglebutton);
}

/*** effects *****************************************************************/

static void
config_effect_plugin_cb(GtkWidget *widget, gint index);

static gint
scan_effect_plugins(GtkWidget *option_menu, gchar *selected)
{
  GtkWidget *menu = gtk_menu_new();
  GList     *list = g_list_first(get_effect_list());  /* XMMS */
  gint      index = 0;
  gint  sel_index = -1;
  gint  def_index = -1;

  /* sanity check */
  if(selected == NULL) selected = "";

  /* parse module list */
  while(list) {
    EffectPlugin *ep = (EffectPlugin *)list->data;
    GtkWidget  *item = gtk_menu_item_new_with_label(ep->description);
    
    if(def_index == -1) def_index = index;
    if(selected && !strcmp(g_basename(ep->filename), selected))
      sel_index = index;

    /* create menu item */
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       GTK_SIGNAL_FUNC(config_effect_plugin_cb), (gpointer)index++);
    gtk_widget_show(item);
    gtk_menu_append(GTK_MENU(menu), item);

    /* advance to next module */
    list = g_list_next(list);
  }
    
  /* attach menu */
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  
  if(sel_index == -1) {
    DEBUG(("[crossfade] scan_effect_plugins: plugin not found (\"%s\")\n", selected));
    return def_index;  /* use default (first entry) */
  }
  return sel_index;
}

/*-- plugin output callbacks ------------------------------------------------*/

static void
config_effect_plugin_cb(GtkWidget *widget, gint index)
{
  EffectPlugin *ep = g_list_nth_data(get_effect_list(), index);  /* XMMS */

  /* select new plugin */
  ep_index = index;

  /* get new plugin's name */
  if(cfg->ep_name) g_free(cfg->ep_name);
  cfg->ep_name = (ep && ep->filename) 
    ? g_strdup(g_basename(ep->filename)) : NULL;

  /* update gui */
  SET_SENSITIVE("ep_configure_button", ep && (ep->configure != NULL)); 
  SET_SENSITIVE("ep_about_button",     ep && (ep->about != NULL));

  /* 0.3.5: apply effect config immediatelly */
  if(config->ep_name) g_free(config->ep_name);
  config->ep_name = g_strdup(cfg->ep_name);
  xfade_realize_ep_config();
}

void on_ep_configure_button_clicked(GtkButton *button, gpointer user_data)
{
  EffectPlugin *ep = g_list_nth_data(get_effect_list(), ep_index);  /* XMMS */
  if((ep == NULL) || (ep->configure == NULL)) return;
  ep->configure();
}

void on_ep_about_button_clicked(GtkButton *button, gpointer user_data)
{
  EffectPlugin *ep = g_list_nth_data(get_effect_list(), ep_index);  /* XMMS */
  if((ep == NULL) || (ep->about == NULL)) return;
  ep->about();
}

void on_ep_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  /* 0.3.5: apply effect config immediatelly */
  config->ep_enable = cfg->ep_enable = GET_TOGGLE("ep_enable_check");
  xfade_realize_ep_config();
}

/*-- volume normalizer ------------------------------------------------------*/

void check_effects_dependencies()
{
  if(checking) return;
  checking = TRUE;

  SET_SENSITIVE("volnorm_target_spin",      cfg->volnorm_enable);
  SET_SENSITIVE("volnorm_target_label",     cfg->volnorm_enable);
  SET_SENSITIVE("volnorm_quantaudio_check", cfg->volnorm_enable);
  SET_SENSITIVE("volnorm_target_spin",      cfg->volnorm_enable);

  checking = FALSE;
}

void on_volnorm_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->volnorm_enable = gtk_toggle_button_get_active(togglebutton);
  check_effects_dependencies();
}

/*** crossfader **************************************************************/

static void xf_config_cb(GtkWidget *widget, gint index);
static void xf_type_cb  (GtkWidget *widget, gint index);

/* crude hack to keep track of menu items */
static gint xf_config_index_map[MAX_FADE_CONFIGS];
static gint xf_type_index_map  [MAX_FADE_TYPES];

static void
create_crossfader_config_menu()
{
  GtkWidget *optionmenu, *menu;
  gint i, *imap;

  if((optionmenu = lookup_widget(config_win, "xf_config_optionmenu"))) {
    for(i=0; i<MAX_FADE_CONFIGS; i++) xf_config_index_map[i] = -1;
    imap = xf_config_index_map;
    menu = gtk_menu_new();
    add_menu_item(menu, "Start of playback",    GTK_SIGNAL_FUNC(xf_config_cb), FADE_CONFIG_START, &imap);
    add_menu_item(menu, "Automatic songchange", GTK_SIGNAL_FUNC(xf_config_cb), FADE_CONFIG_XFADE, &imap);
#if 0
    /* this should be FADE_TYPE_NONE all the time, anyway,
       so no need to make it configureable by the user */
    add_menu_item(menu, "Automatic (gapless)",  GTK_SIGNAL_FUNC(xf_config_cb), FADE_CONFIG_ALBUM, &imap);
#endif
    add_menu_item(menu, "Manual songchange",    GTK_SIGNAL_FUNC(xf_config_cb), FADE_CONFIG_MANUAL, &imap);
    add_menu_item(menu, "Manual stop",          GTK_SIGNAL_FUNC(xf_config_cb), FADE_CONFIG_STOP, &imap);
    add_menu_item(menu, "End of playlist",      GTK_SIGNAL_FUNC(xf_config_cb), FADE_CONFIG_EOP, &imap);
    add_menu_item(menu, "Seeking",              GTK_SIGNAL_FUNC(xf_config_cb), FADE_CONFIG_SEEK, &imap);
    add_menu_item(menu, "Pause",                GTK_SIGNAL_FUNC(xf_config_cb), FADE_CONFIG_PAUSE, &imap);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  }

}

static void
create_crossfader_type_menu()
{
  GtkWidget *optionmenu, *menu;
  gint i, *imap;
  guint32 mask;

  if((optionmenu = lookup_widget(config_win, "xf_type_optionmenu"))) {
    for(i=0; i<MAX_FADE_TYPES; i++) xf_type_index_map[i] = -1;
    imap = xf_type_index_map;
    menu = gtk_menu_new();
    mask = cfg->fc[cfg->xf_index].type_mask;
    if(mask & (1 << FADE_TYPE_REOPEN))      add_menu_item(menu, "Reopen output device", GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_REOPEN, &imap);
    if(mask & (1 << FADE_TYPE_FLUSH))       add_menu_item(menu, "Flush output device",  GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_FLUSH, &imap);
    if(mask & (1 << FADE_TYPE_NONE))        add_menu_item(menu, "None (gapless/off)",   GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_NONE, &imap);
    if(mask & (1 << FADE_TYPE_PAUSE))       add_menu_item(menu, "Pause",                GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_PAUSE, &imap);
    if(mask & (1 << FADE_TYPE_SIMPLE_XF))   add_menu_item(menu, "Simple crossfade",     GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_SIMPLE_XF, &imap);
    if(mask & (1 << FADE_TYPE_ADVANCED_XF)) add_menu_item(menu, "Advanced crossfade",   GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_ADVANCED_XF, &imap);
    if(mask & (1 << FADE_TYPE_FADEIN))      add_menu_item(menu, "Fadein",               GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_FADEIN, &imap);
    if(mask & (1 << FADE_TYPE_FADEOUT))     add_menu_item(menu, "Fadeout",              GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_FADEOUT, &imap);
    if(mask & (1 << FADE_TYPE_PAUSE_NONE))  add_menu_item(menu, "None",                 GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_PAUSE_NONE, &imap);
    if(mask & (1 << FADE_TYPE_PAUSE_ADV))   add_menu_item(menu, "Fadeout/Fadein",       GTK_SIGNAL_FUNC(xf_type_cb), FADE_TYPE_PAUSE_ADV, &imap);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  }
}

#define NONE             0x00000000L
#define XF_CONFIG        0x00000001L
#define XF_TYPE          0x00000002L
#define XF_MIX_SIZE      0x00000004L
#define XF_FADEOUT       0x00000008L
#define XF_OFFSET        0x00000010L
#define XF_FADEIN        0x00000020L
#define XF_PAGE          0x00000040L
#define XF_FLUSH         0x00000080L 
#define ANY              0xffffffffL

static void
check_crossfader_dependencies(guint32 mask)
{
  fade_config_t *fc = &cfg->fc[cfg->xf_index];
  gint i;

  /* HACK: avoid endless recursion */
  if(checking) return;
  checking = TRUE;

  if(mask & XF_FLUSH) {
    SET_TOGGLE   ("xftfp_enable_check",  fc->flush_pause_enable);
    SET_SENSITIVE("xftfp_length_label",  fc->flush_pause_enable);
    SET_SENSITIVE("xftfp_length_spin",   fc->flush_pause_enable);
    SET_TOGGLE   ("xftffi_enable_check", fc->flush_in_enable);
    SET_SENSITIVE("xftffi_length_label", fc->flush_in_enable);
    SET_SENSITIVE("xftffi_length_spin",  fc->flush_in_enable);
    SET_SENSITIVE("xftffi_volume_label", fc->flush_in_enable);
    SET_SENSITIVE("xftffi_volume_spin",  fc->flush_in_enable);
  }

  if(mask & XF_MIX_SIZE) {
    SET_TOGGLE   ("xf_autobuf_check", cfg->mix_size_auto);
    SET_SENSITIVE("xf_buffer_spin",  !cfg->mix_size_auto);
    SET_SPIN     ("xf_buffer_spin",  xfade_mix_size_ms(cfg));
  }

  if(mask & XF_CONFIG) {
    for(i=0; i<MAX_FADE_CONFIGS && (xf_config_index_map[i] != cfg->xf_index); i++);
    if(i == MAX_FADE_CONFIGS) i=0;
    SET_HISTORY("xf_config_optionmenu", i);
  }

  if(mask & XF_TYPE) {
    create_crossfader_type_menu();
    for(i=0; i<MAX_FADE_TYPES && (xf_type_index_map[i] != fc->type); i++);
    if(i == MAX_FADE_TYPES) {
      fc->type = FADE_TYPE_NONE;
      for(i=0; i<MAX_FADE_TYPES && (xf_type_index_map[i] != fc->type); i++);
      if(i == MAX_FADE_CONFIGS) i=0;
    }
    SET_HISTORY("xf_type_optionmenu", i);
  }
  
  if(mask & XF_PAGE) {
    SET_PAGE("xf_type_notebook",   fc->type);
    SET_SPIN("pause_length_spin",  fc->pause_len_ms);
    SET_SPIN("simple_length_spin", fc->simple_len_ms);
    if(fc->config == FADE_CONFIG_SEEK) {
      HIDE("xftf_pause_frame");
      HIDE("xftf_fadein_frame");
    }
    else {
      SHOW("xftf_pause_frame");
      SHOW("xftf_fadein_frame");
    }
  }
  
  if(mask & XF_FADEOUT) {
    SET_TOGGLE   ("fadeout_enable_check", fc->out_enable);
    SET_SENSITIVE("fadeout_length_label", fc->out_enable);
    SET_SENSITIVE("fadeout_length_spin",  fc->out_enable);
    SET_SPIN     ("fadeout_length_spin",  fc->out_len_ms);
    SET_SENSITIVE("fadeout_volume_label", fc->out_enable);
    SET_SENSITIVE("fadeout_volume_spin",  fc->out_enable);
    SET_SPIN     ("fadeout_volume_spin",  fc->out_volume);
    SET_SPIN     ("xftfo_length_spin",    fc->out_len_ms);
    SET_SPIN     ("xftfo_volume_spin",    fc->out_volume);
    SET_SPIN     ("xftfoi_fadeout_spin",  fc->out_len_ms);
  }
  
  if(mask & XF_FADEIN) {
    SET_TOGGLE   ("fadein_lock_check",    fc->in_locked);
    SET_SENSITIVE("fadein_enable_check", !fc->in_locked);
    SET_TOGGLE   ("fadein_enable_check",  fc->in_locked ? fc->out_enable : fc->in_enable);
    SET_SENSITIVE("fadein_length_label", !fc->in_locked && fc->in_enable);
    SET_SENSITIVE("fadein_length_spin",  !fc->in_locked && fc->in_enable);
    SET_SPIN     ("fadein_length_spin",   fc->in_locked ? fc->out_len_ms : fc->in_len_ms);
    SET_SENSITIVE("fadein_volume_label", !fc->in_locked && fc->in_enable);
    SET_SENSITIVE("fadein_volume_spin",  !fc->in_locked && fc->in_enable);
    SET_SPIN     ("fadein_volume_spin",   fc->in_locked ? fc->out_volume : fc->in_volume);
    SET_SPIN     ("xftfi_length_spin",    fc->in_len_ms);
    SET_SPIN     ("xftfi_volume_spin",    fc->in_volume);
    SET_SPIN     ("xftfoi_fadein_spin",   fc->in_len_ms);
  }
  
  if(mask & XF_OFFSET) {
    if(fc->out_enable)
      SET_SENSITIVE("xfofs_lockout_radiobutton", TRUE);
    if(!fc->in_locked && fc->in_enable)
      SET_SENSITIVE("xfofs_lockin_radiobutton",  TRUE);

    switch(fc->ofs_type) {
    case FC_OFFSET_LOCK_OUT:
      if(!fc->out_enable) {
	SET_TOGGLE("xfofs_none_radiobutton", TRUE);
	fc->ofs_type = FC_OFFSET_NONE;
      }
      break;
    case FC_OFFSET_LOCK_IN:
      if(!(!fc->in_locked && fc->in_enable)) {
	if((fc->in_locked && fc->out_enable)) {
	  SET_TOGGLE("xfofs_lockout_radiobutton", TRUE);
	  fc->ofs_type = FC_OFFSET_LOCK_OUT;
	} else {
	  SET_TOGGLE("xfofs_none_radiobutton", TRUE);
	  fc->ofs_type = FC_OFFSET_NONE;
	}
      }
      break;
    }
    
    switch(fc->ofs_type_wanted) {
    case FC_OFFSET_NONE:
      SET_TOGGLE("xfofs_none_radiobutton", TRUE);
      fc->ofs_type = FC_OFFSET_NONE;
      break;
    case FC_OFFSET_LOCK_OUT:
      if(fc->out_enable) {
	SET_TOGGLE("xfofs_lockout_radiobutton", TRUE);
	fc->ofs_type = FC_OFFSET_LOCK_OUT;
      }
      break;
    case FC_OFFSET_LOCK_IN:
      if(!fc->in_locked && fc->in_enable) {
	SET_TOGGLE("xfofs_lockin_radiobutton", TRUE);
	fc->ofs_type = FC_OFFSET_LOCK_IN;
      }
      else if(fc->out_enable) {
	SET_TOGGLE("xfofs_lockout_radiobutton", TRUE);
	fc->ofs_type = FC_OFFSET_LOCK_OUT;
      }
      break;
    case FC_OFFSET_CUSTOM:
      SET_TOGGLE("xfofs_custom_radiobutton", TRUE);
      fc->ofs_type = FC_OFFSET_CUSTOM;
      break;
    }
    
    if(!fc->out_enable)
      SET_SENSITIVE("xfofs_lockout_radiobutton", FALSE);
    if(!(!fc->in_locked && fc->in_enable))
      SET_SENSITIVE("xfofs_lockin_radiobutton",  FALSE);
  
    SET_SENSITIVE("xfofs_custom_spin",   fc->ofs_type == FC_OFFSET_CUSTOM);
    SET_SPIN     ("xfofs_custom_spin",   xfade_cfg_offset(fc));
    SET_SPIN     ("xftfo_silence_spin",  xfade_cfg_offset(fc));
    SET_SPIN     ("xftfoi_silence_spin", xfade_cfg_offset(fc));
  }

  checking = FALSE;
}

/*-- crossfader callbacks ---------------------------------------------------*/

void on_xf_buffer_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->mix_size_ms = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(NONE);
}

void on_xf_autobuf_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  cfg->mix_size_auto = gtk_toggle_button_get_active(togglebutton);
  check_crossfader_dependencies(XF_MIX_SIZE);
}

/* - config/type  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void xf_config_cb(GtkWidget *widget, gint index)
{
  if(checking) return;
  cfg->xf_index = index;
  check_crossfader_dependencies(ANY & ~XF_CONFIG);
}

void xf_type_cb(GtkWidget *widget, gint index)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].type = index;
  check_crossfader_dependencies(ANY & ~XF_CONFIG);
}

/* - flush  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void on_xftfp_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].flush_pause_enable
    = gtk_toggle_button_get_active(togglebutton);
  check_crossfader_dependencies(XF_FLUSH|XF_MIX_SIZE);
}

void on_xftfp_length_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].flush_pause_len_ms
    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_FLUSH);
}

void on_xftffi_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].flush_in_enable
    = gtk_toggle_button_get_active(togglebutton);
  check_crossfader_dependencies(XF_FLUSH|XF_OFFSET|XF_FADEOUT|XF_FADEIN);
}

void on_xftffi_length_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].flush_in_len_ms
    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_FLUSH);
}

void on_xftffi_volume_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].flush_in_volume
    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_FLUSH);
}

/* - pause  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void on_pause_length_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].pause_len_ms
    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_MIX_SIZE);
}

/* - simple - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void on_simple_length_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].simple_len_ms
    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_MIX_SIZE);
}

/* - crossfade-fadeout  - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void on_fadeout_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].out_enable
    = gtk_toggle_button_get_active(togglebutton);
  check_crossfader_dependencies(XF_MIX_SIZE|XF_OFFSET|XF_FADEOUT|XF_FADEIN);
}

void on_fadeout_length_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].out_len_ms
    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_MIX_SIZE|XF_OFFSET|XF_FADEIN);
}

void on_fadeout_volume_spin_changed(GtkEditable *editable, gpointer user_data)
{
  cfg->fc[cfg->xf_index].out_volume
    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_OFFSET|XF_FADEIN);
}

/* - crossfade-offset - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void on_xfofs_none_radiobutton_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking || !gtk_toggle_button_get_active(togglebutton)) return;
  cfg->fc[cfg->xf_index].ofs_type = FC_OFFSET_NONE;
  cfg->fc[cfg->xf_index].ofs_type_wanted = FC_OFFSET_NONE;
  check_crossfader_dependencies(XF_MIX_SIZE|XF_OFFSET);
}

void on_xfofs_none_radiobutton_clicked(GtkButton *button, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].ofs_type_wanted = FC_OFFSET_NONE;
}

void on_xfofs_lockout_radiobutton_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking || !gtk_toggle_button_get_active(togglebutton)) return;
  cfg->fc[cfg->xf_index].ofs_type = FC_OFFSET_LOCK_OUT;
  cfg->fc[cfg->xf_index].ofs_type_wanted = FC_OFFSET_LOCK_OUT;
  check_crossfader_dependencies(XF_MIX_SIZE|XF_OFFSET);
}

void on_xfofs_lockout_radiobutton_clicked(GtkButton *button, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].ofs_type_wanted = FC_OFFSET_LOCK_OUT;
}

void on_xfofs_lockin_radiobutton_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking || !gtk_toggle_button_get_active(togglebutton)) return;
  cfg->fc[cfg->xf_index].ofs_type = FC_OFFSET_LOCK_IN;
  cfg->fc[cfg->xf_index].ofs_type_wanted = FC_OFFSET_LOCK_IN;
  check_crossfader_dependencies(XF_MIX_SIZE|XF_OFFSET);
}

void on_xfofs_lockin_radiobutton_clicked(GtkButton *button, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].ofs_type_wanted = FC_OFFSET_LOCK_IN;
}

void on_xfofs_custom_radiobutton_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking || !gtk_toggle_button_get_active(togglebutton)) return;
  cfg->fc[cfg->xf_index].ofs_type = FC_OFFSET_CUSTOM;
  cfg->fc[cfg->xf_index].ofs_type_wanted = FC_OFFSET_CUSTOM;
  check_crossfader_dependencies(XF_MIX_SIZE|XF_OFFSET);
}

void on_xfofs_custom_radiobutton_clicked(GtkButton *button, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].ofs_type_wanted = FC_OFFSET_CUSTOM;
}

void on_xfofs_custom_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].ofs_custom_ms = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_MIX_SIZE);
}

/* - crossfade-fadein - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void on_fadein_lock_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].in_locked = gtk_toggle_button_get_active(togglebutton);
  check_crossfader_dependencies(XF_OFFSET|XF_FADEIN);
}

void on_fadein_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].in_enable = gtk_toggle_button_get_active(togglebutton);
  check_crossfader_dependencies(XF_OFFSET|XF_FADEIN);
}

void on_fadein_length_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].in_len_ms = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(XF_MIX_SIZE|XF_OFFSET);
}

void on_fadein_volume_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->fc[cfg->xf_index].in_volume = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_crossfader_dependencies(NONE);
}

/*-- fadein -----------------------------------------------------------------*/

/* signal set to on_fadein_length_spin_changed */
/* signal set to on_fadein_volume_spin_changed */

/*-- fadeout ----------------------------------------------------------------*/

/* signal set to on_fadeout_length_spin_changed */
/* signal set to on_fadeout_volume_spin_changed */

/*-- fadeout/fadein ---------------------------------------------------------*/

/* signal set to on_fadeout_length_spin_changed */
/* signal set to on_xfofs_custom_spin_changed */
/* signal set to on_fadeout_volume_spin_changed */

/*** gap killer **************************************************************/

void check_gapkiller_dependencies()
{
  if(checking) return;
  checking = TRUE;

  SET_SENSITIVE("lgap_length_spin",  cfg->gap_lead_enable);
  SET_SENSITIVE("lgap_level_spin",   cfg->gap_lead_enable);
  SET_SENSITIVE("tgap_enable_check", !cfg->gap_trail_locked);
  SET_SENSITIVE("tgap_length_spin",  !cfg->gap_trail_locked && cfg->gap_trail_enable);
  SET_SENSITIVE("tgap_level_spin",   !cfg->gap_trail_locked && cfg->gap_trail_enable);

  if(cfg->gap_trail_locked) {
    SET_TOGGLE("tgap_enable_check", cfg->gap_lead_enable);
    SET_SPIN  ("tgap_length_spin",  cfg->gap_lead_len_ms);
    SET_SPIN  ("tgap_level_spin",   cfg->gap_lead_level);
  }
  else {
    SET_TOGGLE("tgap_enable_check", cfg->gap_trail_enable);
    SET_SPIN  ("tgap_length_spin",  cfg->gap_trail_len_ms);
    SET_SPIN  ("tgap_level_spin",   cfg->gap_trail_level);
  }

  if(cfg->mix_size_auto)
    SET_SPIN("xf_buffer_spin", xfade_mix_size_ms(cfg));

  checking = FALSE;
}

/*-- gapkiller callbacks ----------------------------------------------------*/

void on_lgap_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->gap_lead_enable = gtk_toggle_button_get_active(togglebutton);
  check_gapkiller_dependencies();
}

void on_lgap_length_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->gap_lead_len_ms = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_gapkiller_dependencies();
}

void on_lgap_level_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->gap_lead_level = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_gapkiller_dependencies();
}

void on_tgap_lock_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->gap_trail_locked = gtk_toggle_button_get_active(togglebutton);
  check_gapkiller_dependencies();
}

void on_tgap_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->gap_trail_enable = gtk_toggle_button_get_active(togglebutton);
  check_gapkiller_dependencies();
}

void on_tgap_length_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->gap_trail_len_ms = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
}

void on_tgap_level_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->gap_trail_level = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
}

/*** misc ********************************************************************/

void check_misc_dependencies()
{
  if(checking) return;
  checking = TRUE;

  if(cfg->mix_size_auto)
    SET_SPIN("xf_buffer_spin", xfade_mix_size_ms(cfg));

  SET_SENSITIVE("moth_opmaxused_spin", cfg->enable_op_max_used);

  checking = FALSE;
}

/*-- misc callbacks ---------------------------------------------------------*/

void on_config_mixopt_enable_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  SET_SENSITIVE("mixopt_reverse_check",  gtk_toggle_button_get_active(togglebutton));
  SET_SENSITIVE("mixopt_software_check", gtk_toggle_button_get_active(togglebutton));
}

void on_moth_songchange_spin_changed(GtkEditable *editable, gpointer user_data)
{
  if(checking) return;
  cfg->songchange_timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  check_misc_dependencies();
}

void on_moth_opmaxused_check_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  if(checking) return;
  cfg->enable_op_max_used = gtk_toggle_button_get_active(togglebutton);
  check_misc_dependencies();
}

/*** presets *****************************************************************/

void on_presets_list_click_column(GtkCList *clist, gint column, gpointer user_data)
{
  DEBUG(("*** column=%d\n", column));
}

/*** main config *************************************************************/

void on_config_apply_clicked(GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;

  /* get current notebook page */
  if((widget = lookup_widget(config_win, "config_notebook")))
    cfg->page = gtk_notebook_get_current_page(GTK_NOTEBOOK(widget));

  /* output method */

  /* sample rate */

  /* output method: plugin */
  op_config.throttle_enable  = GET_TOGGLE("op_throttle_check");
  op_config.max_write_enable = GET_TOGGLE("op_maxblock_check");
  op_config.max_write_len    = GET_SPIN  ("op_maxblock_spin");
  op_config.force_reopen     = GET_TOGGLE("op_forcereopen_check");

  xfade_save_plugin_config(&cfg->op_config_string, cfg->op_name, &op_config);

  /* output method: none: */

  /* effects: pre-mixing effect plugin */

  /* effects: volume normalizer */
  cfg->volnorm_target = GET_SPIN  ("volnorm_target_spin");
  cfg->volnorm_use_qa = GET_TOGGLE("volnorm_quantaudio_check");

  /* crossfader */
  cfg->mix_size_auto = GET_TOGGLE("xf_autobuf_check");

  /* gap killer */
  cfg->gap_lead_enable  = GET_TOGGLE("lgap_enable_check");
  cfg->gap_lead_len_ms  = GET_SPIN  ("lgap_length_spin");
  cfg->gap_lead_level   = GET_SPIN  ("lgap_level_spin");

  cfg->gap_trail_locked = GET_TOGGLE("tgap_lock_check");

  cfg->gap_crossing     = GET_TOGGLE("gadv_crossing_check");

  /* misc */
  cfg->enable_debug           = GET_TOGGLE("debug_stderr_check");
  cfg->enable_monitor         = GET_TOGGLE("debug_monitor_check");
  cfg->enable_mixer           = GET_TOGGLE("mixopt_enable_check");
  cfg->mixer_reverse          = GET_TOGGLE("mixopt_reverse_check"); 
  cfg->mixer_software         = GET_TOGGLE("mixopt_software_check"); 
  cfg->preload_size_ms        = GET_SPIN  ("moth_preload_spin");
  cfg->album_detection        = GET_TOGGLE("noxf_album_check");
  cfg->no_xfade_if_same_file  = GET_TOGGLE("noxf_samefile_check");
  cfg->enable_http_workaround = GET_TOGGLE("moth_httpworkaround_check");
  cfg->op_max_used_ms         = GET_SPIN  ("moth_opmaxused_spin");
  cfg->output_keep_opened     = GET_TOGGLE("moth_outputkeepopened_check");

  /* presets */

  /* lock buffer */
  g_static_mutex_lock(&buffer_mutex);

  /* free existing strings */
  if(config->op_config_string)     g_free(config->op_config_string);
  if(config->op_name)              g_free(config->op_name);
  if(config->ep_name)              g_free(config->ep_name);

  /* copy current settings (dupping the strings) */
  *config = *cfg;
  config->op_config_string     = g_strdup(cfg->op_config_string);
  config->op_name              = g_strdup(cfg->op_name);
  config->ep_name              = g_strdup(cfg->ep_name);

  /* tell the engine that the config has changed */
  xfade_realize_config();
  
  /* unlock buffer */
  g_static_mutex_unlock(&buffer_mutex);

  /* save configuration */
  xfade_save_config();
  
  /* show/hide monitor win depending on config->enable_monitor */
  xfade_check_monitor_win(); 
}

void on_config_ok_clicked(GtkButton *button, gpointer user_data)
{
  /* apply and save config */
  on_config_apply_clicked(button, user_data);

  /* close and destroy window */
  gtk_widget_destroy(config_win);
}

void xfade_configure()
{
  GtkWidget *widget;

  if(!config_win) {
    /* create */
    if(!(config_win = create_config_win())) {
      DEBUG(("[crossfade] plugin_configure: error creating window!\n"));
      return;
    }

    /* update config_win when window is destroyed */
    gtk_signal_connect(GTK_OBJECT(config_win), "destroy",
		       GTK_SIGNAL_FUNC(gtk_widget_destroyed), &config_win);
    
    /* free any strings that might be left in our local copy of the config */
    if(cfg->op_config_string)     g_free(cfg->op_config_string);
    if(cfg->op_name)              g_free(cfg->op_name);
    if(cfg->ep_name)              g_free(cfg->ep_name);

    /* copy current settings (dupping the strings) */
    *cfg = *config;
    cfg->op_config_string     = g_strdup(config->op_config_string);
    cfg->op_name              = g_strdup(config->op_name);
    cfg->ep_name              = g_strdup(config->ep_name);

    /* go to remembered notebook page */
    if((widget = lookup_widget(config_win, "config_notebook")))
      gtk_notebook_set_page(GTK_NOTEBOOK(widget), config->page);

    /* output: resampling rate */
    if((widget = lookup_widget(config_win, "resampling_rate_optionmenu"))) {
      GtkWidget *menu = gtk_menu_new();
      GtkWidget *item;

      item = gtk_menu_item_new_with_label("44100 Hz");
      gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(resampling_rate_cb), (gpointer)44100);
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu), item);
      
      item = gtk_menu_item_new_with_label("48000 Hz");
      gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(resampling_rate_cb), (gpointer)48000);
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu), item);

      gtk_option_menu_set_menu(GTK_OPTION_MENU(widget), menu);

      switch(cfg->output_rate) {
      default:
	DEBUG(("[crossfade] plugin_configure: WARNING: invalid output sample rate (%d)!\n", cfg->output_rate));
	DEBUG(("[crossfade] plugin_configure:          ... using default of 44100\n"));
	cfg->output_rate = 44100;
      case 44100: gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 0); break;
      case 48000: gtk_option_menu_set_history(GTK_OPTION_MENU(widget), 1); break;
      }
    }

    /* output: resampling quality (libsamplerate setting) */
#ifdef HAVE_LIBSAMPLERATE
    if((widget = lookup_widget(config_win, "resampling_quality_optionmenu"))) {
      GtkWidget *menu = gtk_menu_new();
      GtkWidget *item;

      GtkTooltips *tooltips = (GtkTooltips *)gtk_object_get_data(GTK_OBJECT(config_win), "tooltips");

      int converter_type;
      const char *name, *description;
      for(converter_type = 0; (name = src_get_name(converter_type));
	  converter_type++) {
	description = src_get_description(converter_type);

	item = gtk_menu_item_new_with_label(name);
	gtk_tooltips_set_tip(tooltips, item, description, NULL);

	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   GTK_SIGNAL_FUNC(resampling_quality_cb), (gpointer)converter_type);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu), item);
      }
      
      gtk_option_menu_set_menu   (GTK_OPTION_MENU(widget), menu);
      gtk_option_menu_set_history(GTK_OPTION_MENU(widget), cfg->output_quality);
    }
#else
    HIDE("resampling_quality_hbox");
    HIDE("resampling_quality_optionmenu");
#endif

    /* output method: plugin */
    xfade_load_plugin_config(cfg->op_config_string, cfg->op_name, &op_config);
    SET_TOGGLE   ("op_throttle_check",    op_config.throttle_enable);
    SET_TOGGLE   ("op_maxblock_check",    op_config.max_write_enable);
    SET_SPIN     ("op_maxblock_spin",     op_config.max_write_len); 
    SET_SENSITIVE("op_maxblock_spin",     op_config.max_write_enable);
    SET_TOGGLE   ("op_forcereopen_check", op_config.force_reopen);

    if((widget = lookup_widget(config_win, "op_plugin_optionmenu"))) {
      OutputPlugin *op = NULL;
      if((op_index = scan_plugins(widget, cfg->op_name)) >= 0) {
	gtk_option_menu_set_history(GTK_OPTION_MENU(widget), op_index);
	op = g_list_nth_data(get_output_list(), op_index);  /* XMMS */
      }
      SET_SENSITIVE("op_configure_button", op && (op->configure != NULL));
      SET_SENSITIVE("op_about_button",     op && (op->about     != NULL));
    }

    /* output method: none */

    /* effects: pre-mixing effect plugin */
    if((widget = lookup_widget(config_win, "ep_plugin_optionmenu"))) {
      EffectPlugin *ep = NULL;
      if((ep_index = scan_effect_plugins(widget, cfg->ep_name)) >= 0) {
	gtk_option_menu_set_history(GTK_OPTION_MENU(widget), ep_index);
	ep = g_list_nth_data(get_effect_list(), ep_index);  /* XMMS */
      }
      SET_SENSITIVE("ep_configure_button", ep && (ep->configure != NULL));
      SET_SENSITIVE("ep_about_button",     ep && (ep->about     != NULL));
      SET_TOGGLE   ("ep_enable_check",     cfg->ep_enable);
    }

    /* effects: volume normalizer */
    SET_TOGGLE("volnorm_enable_check",     cfg->volnorm_enable);
    SET_TOGGLE("volnorm_quantaudio_check", cfg->volnorm_use_qa);
    SET_SPIN  ("volnorm_target_spin",      cfg->volnorm_target);

    check_effects_dependencies();
    
    /* crossfader */
    create_crossfader_config_menu();

    if((cfg->xf_index < 0) || (cfg->xf_index >= MAX_FADE_CONFIGS)) {
      DEBUG(("[crossfade] plugin_configure: crossfade index out of range (%d)!\n", cfg->xf_index));
      cfg->xf_index = CLAMP(cfg->xf_index, 0, MAX_FADE_CONFIGS);
    }

    check_crossfader_dependencies(ANY);
    
    /* gap killer */
    SET_TOGGLE   ("lgap_enable_check",   cfg->gap_lead_enable);
    SET_SPIN     ("lgap_length_spin",    cfg->gap_lead_len_ms);
    SET_SPIN     ("lgap_level_spin",     cfg->gap_lead_level);
    SET_TOGGLE   ("tgap_lock_check",     cfg->gap_trail_locked);
    SET_TOGGLE   ("tgap_enable_check",   cfg->gap_trail_enable);
    SET_SPIN     ("tgap_length_spin",    cfg->gap_trail_len_ms);
    SET_SPIN     ("tgap_level_spin",     cfg->gap_trail_level);
    SET_TOGGLE   ("gadv_crossing_check", cfg->gap_crossing);

    check_gapkiller_dependencies();

    /* misc */
    SET_TOGGLE("debug_stderr_check",          cfg->enable_debug);
    SET_TOGGLE("debug_monitor_check",         cfg->enable_monitor);
    SET_TOGGLE("mixopt_enable_check",         cfg->enable_mixer);
    SET_TOGGLE("mixopt_reverse_check",        cfg->mixer_reverse);
    SET_TOGGLE("mixopt_software_check",       cfg->mixer_software);
    SET_SPIN  ("moth_songchange_spin",        cfg->songchange_timeout);
    SET_SPIN  ("moth_preload_spin",           cfg->preload_size_ms);
    SET_TOGGLE("noxf_album_check",            cfg->album_detection);
    SET_TOGGLE("noxf_samefile_check",         cfg->album_detection);
    SET_TOGGLE("moth_httpworkaround_check",   cfg->enable_http_workaround);
    SET_TOGGLE("moth_opmaxused_check",        cfg->enable_op_max_used);
    SET_SPIN  ("moth_opmaxused_spin",         cfg->op_max_used_ms);
    SET_TOGGLE("moth_outputkeepopened_check", cfg->output_keep_opened);

    check_misc_dependencies();

    /* presets */
    if((set_wgt = lookup_widget(config_win, "presets_list_list"))) {
      GList *item;

      for(item = config->presets; item; item = g_list_next(item)) {
	gchar *name = (gchar *)item->data;
	gchar *text[] = {name, "Default", "No"};
	gtk_clist_append(GTK_CLIST(set_wgt), text);
      }
    }

    /* show window near mouse pointer */
    gtk_window_set_position(GTK_WINDOW(config_win), GTK_WIN_POS_MOUSE);
    gtk_widget_show(config_win);
  }
  else
    /* bring window to front */
    gdk_window_raise(config_win->window);
}

void xfade_about()
{
  if(!about_win) {
    gchar *about_text = 
      "Audacious crossfading plugin\n"
      "Code adapted for Audacious usage by Tony Vroon <chainsaw@gentoo.org> from:\n"
      "XMMS Crossfade Plugin "VERSION"\n"
      "Copyright (C) 2000-2004  Peter Eisenlohr <peter@eisenlohr.org>\n"
      "\n"
      "based on the original OSS Output Plugin  Copyright (C) 1998-2000\n"
      "Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies\n"
      "\n"
      "This program is free software; you can redistribute it and/or modify\n"
      "it under the terms of the GNU General Public License as published by\n"
      "the Free Software Foundation; either version 2 of the License, or\n"
      "(at your option) any later version.\n"
      "\n"
      "This program is distributed in the hope that it will be useful,\n"
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "GNU General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU General Public License\n"
      "along with this program; if not, write to the Free Software\n"
      "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,\n"
      "USA.";

    about_win = create_about_win();

    /* update about_win when window is destroyed */
    gtk_signal_connect(GTK_OBJECT(about_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about_win);
    
    /* set about box text (this is done here and not in interface.c because
       of the VERSION #define -- there is no way to do this with GLADE */
    if((set_wgt = lookup_widget(about_win, "about_label")))
      gtk_label_set_text(GTK_LABEL(set_wgt), about_text);
    
    /* show near mouse pointer */
    gtk_window_set_position(GTK_WINDOW(about_win), GTK_WIN_POS_MOUSE);
    gtk_widget_show(about_win);
  }
  else
    gdk_window_raise(about_win->window);
}
