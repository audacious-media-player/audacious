/*  xmms_ladspa - use LADSPA plugins from XMMS
    Copyright (C) 2002,2003  Nick Lamb <njl195@zepler.org.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* BMP-ladspa port by Giacomo Lozito <city_hunter@users.sf.net> */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/types.h>
#include <gtk/gtk.h>

#include <audacious/plugin.h>
#include <libaudacious/configdb.h>

#include "../../../config.h"
#include "ladspa.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define PLUGIN_NAME "LADSPA host " PACKAGE_VERSION

#define MAX_SAMPLES 8192
#define MAX_KNOBS 64

typedef struct {
  char *name;
  char *filename;
  long int id;
  long int unique_id;
  gboolean stereo;
} ladspa_plugin;

typedef struct {
  void *library;
  char *filename;
  gboolean stereo;
  gboolean restored;
  const LADSPA_Descriptor *descriptor;
  LADSPA_Handle *handle; /* left or mono */
  LADSPA_Handle *handle2; /* right stereo */
  GtkWidget *window;
  guint timeout;
  GtkAdjustment *adjustments[MAX_KNOBS];
  LADSPA_Data knobs[MAX_KNOBS];
} plugin_instance;

static void start (void);
static void stop (void);
static int apply_effect (gpointer *d, gint length, AFormat afmt,
                        gint srate, gint nch);
static void configure(void);

static void restore (void);
static plugin_instance * add_plugin (ladspa_plugin *plugin);
static void find_all_plugins(void);
static void find_plugins(char *path_entry);
static ladspa_plugin *get_plugin_by_id(long id);
static plugin_instance * load (char *filename, long int num);
static void reboot_plugins (void);
static void boot_plugin (plugin_instance *instance);
static void port_assign(plugin_instance *instance);
static void shutdown (plugin_instance *instance);
static void unload (plugin_instance *instance);

static GtkWidget * make_plugin_clist(void);
static void make_run_clist(void);
static void sort_column(GtkCList *clist, gint column, gpointer user_data);
static void select_plugin(GtkCList *clist, gint row, gint column,
                          GdkEventButton *event, gpointer user_data);
static void unselect_plugin(GtkCList *clist, gint row, gint column,
                            GdkEventButton *event, gpointer user_data);
static void add_plugin_clicked (GtkButton *button, gpointer user_data);
static void remove_plugin_clicked (GtkButton *button, gpointer user_data);
static void configure_plugin_clicked (GtkButton *button, gpointer user_data);

static void draw_plugin(plugin_instance *instance);

static LADSPA_Data left[MAX_SAMPLES], right[MAX_SAMPLES], trash[MAX_SAMPLES];

G_LOCK_DEFINE_STATIC(running_plugins);

static GSList *plugin_list, *running_plugins;

static ladspa_plugin * selected_plugin;
static plugin_instance * selected_instance;

static struct {
  AFormat afmt;
  gint srate;
  gint nch;
  gboolean ignore;
  gboolean running;
  gboolean initialised;
} state = { 0, 0, 0, FALSE, FALSE, FALSE};

static GtkWidget *config_window = NULL, *run_clist = NULL;

static EffectPlugin xmms_plugin = {
  NULL, NULL,
  PLUGIN_NAME,
  start,
  stop,
  NULL,
  configure,
  apply_effect,
  NULL
};

EffectPlugin *get_eplugin_info (void)
{
  return &xmms_plugin;
}

static void start (void)
{
  if (state.initialised == FALSE) {
    restore();
  } else if (state.srate > 0) {
    reboot_plugins();
  }
  state.running = TRUE;
}

static void restore (void)
{
  ConfigDb *db;
  gint k, plugins= 0;

  db = bmp_cfg_db_open();

  bmp_cfg_db_get_int(db, "ladspa", "plugins", &plugins);
  for (k= 0; k < plugins; ++k) {
    gint id;
    int port, ports= 0;
    plugin_instance *instance;
    gchar *section = g_strdup_printf("ladspa_plugin%d", k);

    bmp_cfg_db_get_int(db, section, "id", &id);
    instance = add_plugin(get_plugin_by_id(id));
    if (!instance) continue; /* couldn't load this plugin */
    bmp_cfg_db_get_int(db, section, "ports", &ports);
    for (port= 0; port < ports && port < MAX_KNOBS; ++port) {
      gchar *key = g_strdup_printf("port%d", port);
      bmp_cfg_db_get_float(db, section, key, &(instance->knobs[port]));
    }
    instance->restored = TRUE;
    g_free(section);
  }

  state.initialised = TRUE;
  bmp_cfg_db_close(db);
}

static ladspa_plugin *get_plugin_by_id(long id)
{
  GSList *list;
  ladspa_plugin *plugin;

  if (plugin_list == NULL) {
    find_all_plugins();
  }

  for (list= plugin_list; list != NULL; list = g_slist_next(list)) {
    plugin = (ladspa_plugin *) list->data;
    if (plugin->unique_id == id) {
      return plugin;
    }
  }

  return NULL;
}

static void find_all_plugins (void)
{
  char *ladspa_path, *directory;

  plugin_list = NULL; /* empty list */
  ladspa_path= getenv("LADSPA_PATH");
  if (ladspa_path == NULL) {
    /* Fallback, look in obvious places */
    find_plugins("/usr/lib/ladspa");
    find_plugins("/usr/local/lib/ladspa");
  } else {
    ladspa_path = g_strdup(ladspa_path);

    directory = strtok(ladspa_path, ":");
    while (directory != NULL) {
      find_plugins(directory);
      directory = strtok(NULL, ":");
    }
    g_free(ladspa_path);
  }
}

static plugin_instance * load (char *filename, long int num)
{
  LADSPA_Descriptor_Function descriptor_fn;
  plugin_instance *instance;

  instance = g_new0(plugin_instance, 1);
  
  instance->filename = filename;
  instance->library = dlopen(filename, RTLD_NOW);
  if (instance->library == NULL) {
    g_free(instance);
    return NULL;
  }
  descriptor_fn = dlsym(instance->library, "ladspa_descriptor");
  if (descriptor_fn == NULL) {
    g_free(instance);
    return NULL;
  }
  instance->descriptor = descriptor_fn(num);

  return instance;
}

static void unload (plugin_instance * instance)
{
  if (instance->window) {
    gtk_widget_destroy(instance->window);
    instance->window = NULL;
  }

  if (instance->timeout) {
    gtk_timeout_remove(instance->timeout);
  }

  shutdown(instance);

  if (instance->library) {
    dlclose(instance->library);
  }
}

static void stop (void)
{
  GSList *list;
  ConfigDb *db;
  gint plugins = 0;

  if (state.running == FALSE) {
    return;
  }
  state.running = FALSE;
  db = bmp_cfg_db_open();
  G_LOCK (running_plugins);
  for (list= running_plugins; list != NULL; list = g_slist_next(list)) {
    plugin_instance *instance = (plugin_instance *) list->data;
    gchar *section = g_strdup_printf("ladspa_plugin%d", plugins++);
    int port, ports= 0;

    bmp_cfg_db_set_int(db, section, "id", instance->descriptor->UniqueID);
    bmp_cfg_db_set_string(db, section, "file", instance->filename);
    bmp_cfg_db_set_string(db, section, "label", (gchar *)
                                              instance->descriptor->Label);

    ports = instance->descriptor->PortCount;
    if (ports > MAX_KNOBS) ports = MAX_KNOBS;
    for (port= 0; port < ports; ++port) {
      gchar *key = g_strdup_printf("port%d", port);
      bmp_cfg_db_set_float(db, section, key, instance->knobs[port]);
      g_free(key);
    }
    bmp_cfg_db_set_int(db, section, "ports", ports);
    g_free(section);
    shutdown (instance);
  }
  G_UNLOCK (running_plugins);

  bmp_cfg_db_set_int(db, "ladspa", "plugins", plugins);
  bmp_cfg_db_close(db);
}

static void shutdown (plugin_instance *instance)
{
  const LADSPA_Descriptor * descriptor= instance->descriptor;

  if (instance->handle) {
    if (descriptor->deactivate) {
      descriptor->deactivate(instance->handle);
    }
    descriptor->cleanup(instance->handle);
    instance->handle = NULL;
  }
  if (instance->handle2) {
    if (descriptor->deactivate) {
      descriptor->deactivate(instance->handle2);
    }
    descriptor->cleanup(instance->handle2);
    instance->handle2 = NULL;
  }
}

static void boot_plugin (plugin_instance *instance)
{
  const LADSPA_Descriptor * descriptor = instance->descriptor;

  shutdown(instance);
  instance->handle = descriptor->instantiate(descriptor, state.srate);
  if (state.nch > 1 && !instance->stereo) {
    /* Create an additional instance */
    instance->handle2 = descriptor->instantiate(descriptor, state.srate);
  }

  port_assign(instance);

  if (descriptor->activate) {
    descriptor->activate(instance->handle);
    if (instance->handle2) {
      descriptor->activate(instance->handle2);
    }
  }
}

static void reboot_plugins (void)
{
  GSList *list;

  G_LOCK (running_plugins);
  for (list= running_plugins; list != NULL; list = g_slist_next(list)) {
    boot_plugin ((plugin_instance *) list->data);
  }
  G_UNLOCK (running_plugins);
}

static int apply_effect (gpointer *d, gint length, AFormat afmt,
                        gint srate, gint nch)
{
  gint16 *raw16 = *d;
  GSList *list;
  plugin_instance *instance;
  int k;

  if (running_plugins == NULL || state.running == FALSE) {
    return length;
  }

  if (state.afmt != afmt || state.srate != srate || state.nch != nch) {
    state.afmt = afmt;
    state.srate = srate;
    state.nch = nch;

    if (nch < 1 || nch > 2)
      state.ignore = 1;
    else if (afmt == FMT_S16_NE)
      state.ignore = 0;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    else if (afmt == FMT_S16_LE)
      state.ignore = 0;
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    else if (afmt == FMT_S16_BE)
      state.ignore = 0;
#endif
    else
      state.ignore = 1;

    reboot_plugins();
  }

  if (state.ignore || length > MAX_SAMPLES * 2) {
    return length;
  }

  if (state.nch == 1) {
    for (k= 0; k < length / 2; ++k) {
      left[k] = ((LADSPA_Data) raw16[k]) * (1.0f / 32768.0f);
    }
    G_LOCK (running_plugins);
    for (list= running_plugins; list != NULL; list = g_slist_next(list)) {
      instance = (plugin_instance *) list->data;
      if (instance->handle) {
        instance->descriptor->run(instance->handle, length / 2);
      }
    }
    G_UNLOCK (running_plugins);
    for (k= 0; k < length / 2; ++k) {
      raw16[k] = CLAMP((int) (left[k] * 32768.0f), -32768, 32767);
    }
  } else {
    for (k= 0; k < length / 2; k += 2) {
      left[k/2] = ((LADSPA_Data) raw16[k]) * (1.0f / 32768.0f);
    }
    for (k= 1; k < length / 2; k += 2) {
      right[k/2] = ((LADSPA_Data) raw16[k]) * (1.0f / 32768.0f);
    }
    G_LOCK (running_plugins);
    for (list= running_plugins; list != NULL; list = g_slist_next(list)) {
      instance = (plugin_instance *) list->data;
      if (instance->handle) {
        instance->descriptor->run(instance->handle, length / 4);
      }
      if (instance->handle2) {
        instance->descriptor->run(instance->handle2, length / 4);
      }
    }
    G_UNLOCK (running_plugins);
    for (k= 0; k < length / 2; k += 2) {
      raw16[k] = CLAMP((int) (left[k/2] * 32768.0f), -32768, 32767);
    }
    for (k= 1; k < length / 2; k += 2) {
      raw16[k] = CLAMP((int) (right[k/2] * 32768.0f), -32768, 32767);
    }
  }

  return length;
}

static void port_assign(plugin_instance * instance) {
  unsigned long port;
  unsigned long inputs= 0, outputs= 0;
  const LADSPA_Descriptor * plugin = instance->descriptor;

  for (port = 0; port < plugin->PortCount; ++port) {

    if (LADSPA_IS_PORT_CONTROL(plugin->PortDescriptors[port])) {
      if (port < MAX_KNOBS) {
        plugin->connect_port(instance->handle, port, &(instance->knobs[port]));
        if (instance->handle2)
          plugin->connect_port(instance->handle2, port, &(instance->knobs[port]));
      } else {
        plugin->connect_port(instance->handle, port, trash);
        if (instance->handle2)
          plugin->connect_port(instance->handle2, port, trash);
      }
      
    } else if (LADSPA_IS_PORT_AUDIO(plugin->PortDescriptors[port])) {

      if (LADSPA_IS_PORT_INPUT(plugin->PortDescriptors[port])) {
        if (inputs == 0) {
          plugin->connect_port(instance->handle, port, left);
          if (instance->handle2)
            plugin->connect_port(instance->handle2, port, right);
        } else if (inputs == 1 && instance->stereo) {
          plugin->connect_port(instance->handle, port, right);
        } else {
          plugin->connect_port(instance->handle, port, trash);
          if (instance->handle2)
            plugin->connect_port(instance->handle2, port, trash);
        }
        inputs++;

      } else if (LADSPA_IS_PORT_OUTPUT(plugin->PortDescriptors[port])) {
        if (outputs == 0) {
          plugin->connect_port(instance->handle, port, left);
          if (instance->handle2)
            plugin->connect_port(instance->handle2, port, right);
        } else if (outputs == 1 && instance->stereo) {
          plugin->connect_port(instance->handle, port, right);
        } else {
          plugin->connect_port(instance->handle, port, trash);
          if (instance->handle2)
            plugin->connect_port(instance->handle2, port, trash);
        }
        outputs++;

      }
    }
  }

}

static void find_plugins(char *path_entry)
{
  ladspa_plugin *plugin;
  void *library = NULL;
  char lib_name[PATH_MAX];
  LADSPA_Descriptor_Function descriptor_fn;
  const LADSPA_Descriptor *descriptor;
  DIR *dir;
  struct dirent *dirent;
  long int k;
  unsigned long int port, input, output;
  
  dir= opendir(path_entry);
  if (dir == NULL) return;

  while ((dirent= readdir(dir))) {
    snprintf(lib_name, PATH_MAX, "%s/%s", path_entry, dirent->d_name);
    library = dlopen(lib_name, RTLD_LAZY);
    if (library == NULL) {
      continue;
    }
    descriptor_fn = dlsym(library, "ladspa_descriptor");
    if (descriptor_fn == NULL) {
      dlclose(library);
      continue;
    }

    for (k= 0;; ++k) {
      descriptor= descriptor_fn(k);
      if (descriptor == NULL) {
        break;
      }
      plugin = g_new(ladspa_plugin, 1);
      plugin->name= g_strdup(descriptor->Name);
      plugin->filename= g_strdup(lib_name);
      plugin->id= k;
      plugin->unique_id= descriptor->UniqueID;
      for (input = output = port = 0; port < descriptor->PortCount; ++port) {
        if (LADSPA_IS_PORT_AUDIO(descriptor->PortDescriptors[port])) {
          if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[port]))
            input++;
          if (LADSPA_IS_PORT_OUTPUT(descriptor->PortDescriptors[port]))
            output++;
        } else if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[port])) {
        }
      }
      if (input >= 2 && output >= 2) {
        plugin->stereo= TRUE;
      } else {
        plugin->stereo= FALSE;
      }
      plugin_list = g_slist_prepend(plugin_list, plugin);
    }
    dlclose(library);
  }

  closedir(dir);
  return;
}

static void value_changed(GtkAdjustment *adjustment, gpointer *user_data)
{
  LADSPA_Data *data = (LADSPA_Data *) user_data;

  G_LOCK (running_plugins);
  *data = (LADSPA_Data) adjustment->value;
  G_UNLOCK (running_plugins);
}

static void toggled(GtkToggleButton *togglebutton, gpointer *user_data)
{
  LADSPA_Data *data = (LADSPA_Data *) user_data;

  if (gtk_toggle_button_get_active(togglebutton)) {
    G_LOCK (running_plugins);
    *data = (LADSPA_Data) 1.0f;
    G_UNLOCK (running_plugins);
  } else {
    G_LOCK (running_plugins);
    *data = (LADSPA_Data) -1.0f;
    G_UNLOCK (running_plugins);
  }
}

static int update_instance (gpointer data)
{
  plugin_instance *instance = (plugin_instance *) data;
  unsigned long k;

  G_LOCK (running_plugins);
  for (k = 0; k < MAX_KNOBS && k < instance->descriptor->PortCount; ++k) {
    if (LADSPA_IS_PORT_OUTPUT(instance->descriptor->PortDescriptors[k])
     && LADSPA_IS_PORT_CONTROL(instance->descriptor->PortDescriptors[k])) {
      instance->adjustments[k]->value = instance->knobs[k];
      gtk_adjustment_value_changed(instance->adjustments[k]);
    }
  }
  G_UNLOCK (running_plugins);
  return TRUE;
}

static void draw_plugin(plugin_instance *instance)
{
  const LADSPA_Descriptor *plugin = instance->descriptor;
  const LADSPA_PortRangeHint *hints = plugin->PortRangeHints;
  LADSPA_Data fact, min, max, step, start;
  int dp;
  unsigned long k;
  gboolean no_ui = TRUE;
  GtkWidget *widget, *vbox, *hbox;
  GtkObject *adjustment;

  if (instance->window != NULL) {
    /* Just show window */
    gtk_widget_show(instance->window);
    return;
  }

  instance->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(instance->window), plugin->Name);

  vbox= gtk_vbox_new(FALSE, 3);

  for (k = 0; k < MAX_KNOBS && k < plugin->PortCount; ++k) {
    if (! LADSPA_IS_PORT_CONTROL(plugin->PortDescriptors[k]))
      continue;
    no_ui = FALSE;
    hbox = gtk_hbox_new(FALSE, 3);
    widget = gtk_label_new(plugin->PortNames[k]);
    gtk_container_add(GTK_CONTAINER(hbox), widget);

    if (LADSPA_IS_HINT_TOGGLED(hints[k].HintDescriptor)) {
      widget = gtk_toggle_button_new_with_label("Press");
      g_signal_connect(G_OBJECT(widget), "toggled",
                       G_CALLBACK(toggled), &(instance->knobs[k]));
      gtk_container_add(GTK_CONTAINER(hbox), widget);
      gtk_container_add(GTK_CONTAINER(vbox), hbox);
      continue;
    }

    if (LADSPA_IS_HINT_SAMPLE_RATE(hints[k].HintDescriptor)) {
      fact = state.srate ? state.srate : 44100.0f;
    } else {
      fact = 1.0f;
    }

    if (LADSPA_IS_HINT_BOUNDED_BELOW(hints[k].HintDescriptor)) {
      min= hints[k].LowerBound * fact;
    } else {
      min= -10000.0f;
    }

    if (LADSPA_IS_HINT_BOUNDED_ABOVE(hints[k].HintDescriptor)) {
      max= hints[k].UpperBound * fact;
    } else {
      max= 10000.0f;
    }

    /* infinity */
    if (10000.0f <= max - min) {
      dp = 1;
      step = 5.0f;

    /* 100.0 ... lots */
    } else if (100.0f < max - min) {
      dp = 0;
      step = 5.0f;

    /* 10.0 ... 100.0 */
    } else if (10.0f < max - min) {
      dp = 1;
      step = 0.5f;

    /* 1.0 ... 10.0 */
    } else if (1.0f < max - min) {
      dp = 2;
      step = 0.05f;

    /* 0.0 ... 1.0 */
    } else {
      dp = 3;
      step = 0.005f;
    }

    if (LADSPA_IS_HINT_INTEGER(hints[k].HintDescriptor)) {
      dp = 0;
      if (step < 1.0f) step = 1.0f;
    }

    if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hints[k].HintDescriptor)) {
      start = min;
    } else if (LADSPA_IS_HINT_DEFAULT_LOW(hints[k].HintDescriptor)) {
      start = min * 0.75f + max * 0.25f;
    } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hints[k].HintDescriptor)) {
      start = min * 0.5f + max * 0.5f;
    } else if (LADSPA_IS_HINT_DEFAULT_HIGH(hints[k].HintDescriptor)) {
      start = min * 0.25f + max * 0.75f;
    } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hints[k].HintDescriptor)) {
      start = max;
    } else if (LADSPA_IS_HINT_DEFAULT_0(hints[k].HintDescriptor)) {
      start = 0.0f;
    } else if (LADSPA_IS_HINT_DEFAULT_1(hints[k].HintDescriptor)) {
      start = 1.0f;
    } else if (LADSPA_IS_HINT_DEFAULT_100(hints[k].HintDescriptor)) {
      start = 100.0f;
    } else if (LADSPA_IS_HINT_DEFAULT_440(hints[k].HintDescriptor)) {
      start = 440.0f;
    } else if (LADSPA_IS_HINT_INTEGER(hints[k].HintDescriptor)) {
      start = min;
    } else if (max >= 0.0f && min <= 0.0f) {
      start = 0.0f;
    } else {
      start = min * 0.5f + max * 0.5f;
    }

    if (instance->restored) {
      start = instance->knobs[k];
    } else {
      instance->knobs[k] = start;
    }
    adjustment = gtk_adjustment_new(start, min, max, step, step * 10.0, 0.0);
    instance->adjustments[k] = GTK_ADJUSTMENT(adjustment);
    widget = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), step, dp);
    if (LADSPA_IS_PORT_OUTPUT(plugin->PortDescriptors[k])) {
      gtk_widget_set_sensitive(widget, FALSE);
    } else {
      g_signal_connect(adjustment, "value-changed",
                   G_CALLBACK(value_changed), &(instance->knobs[k]));
    }
    gtk_container_add(GTK_CONTAINER(hbox), widget);
    widget = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
    gtk_scale_set_digits(GTK_SCALE(widget), dp);
    if (LADSPA_IS_PORT_OUTPUT(plugin->PortDescriptors[k])) {
      gtk_widget_set_sensitive(widget, FALSE);
    }
    gtk_container_add(GTK_CONTAINER(hbox), widget);

    gtk_container_add(GTK_CONTAINER(vbox), hbox);
  }

  if (no_ui) {
    widget = gtk_label_new("This LADSPA plugin has no user controls");
    gtk_container_add(GTK_CONTAINER(vbox), widget);
  }

  instance->timeout = gtk_timeout_add(100, update_instance, instance);

  gtk_container_add(GTK_CONTAINER(instance->window), vbox);

  g_signal_connect (G_OBJECT (instance->window), "delete_event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
  gtk_widget_show_all(instance->window);
}

static void sort_column(GtkCList *clist, gint column, gpointer user_data)
{
  gtk_clist_set_sort_column(clist, column);
  gtk_clist_sort(clist);
}

static void unselect_instance(GtkCList *clist, gint row, gint column,
                              GdkEventButton *event, gpointer user_data)
{
  selected_instance= NULL;
}

static void select_instance(GtkCList *clist, gint row, gint column,
                            GdkEventButton *event, gpointer user_data)
{
  selected_instance= (plugin_instance *) gtk_clist_get_row_data(clist, row);
}

static void reorder_instance(GtkCList *clist, gint from, gint to,
                             gpointer user_data)
{
  void *data;

  G_LOCK (running_plugins);
  data = g_slist_nth_data(running_plugins, from);
  running_plugins= g_slist_remove(running_plugins, data);
  running_plugins= g_slist_insert(running_plugins, data, to);
  G_UNLOCK (running_plugins);
}

static void make_run_clist(void)
{
  char * titles[1] = { "Name" };
  GSList *list;

  run_clist = gtk_clist_new_with_titles(1, titles);
  gtk_clist_column_titles_passive(GTK_CLIST (run_clist));
  gtk_clist_set_reorderable(GTK_CLIST (run_clist), TRUE);
  g_signal_connect(G_OBJECT(run_clist), "select-row",
                     G_CALLBACK(select_instance), NULL);
  g_signal_connect(G_OBJECT(run_clist), "unselect-row",
                     G_CALLBACK(unselect_instance), NULL);
  g_signal_connect(G_OBJECT(run_clist), "row-move",
                     G_CALLBACK(reorder_instance), NULL);

  G_LOCK (running_plugins);
  for (list= running_plugins; list != NULL; list = g_slist_next(list)) {
    gint row;
    gchar *line[1];
    plugin_instance *instance = (plugin_instance *) list->data;

    line[0] = (char *) instance->descriptor->Name;
    row = gtk_clist_append(GTK_CLIST (run_clist), line);
    gtk_clist_set_row_data(GTK_CLIST (run_clist), row, (gpointer) instance);
    gtk_clist_select_row(GTK_CLIST(run_clist), row, 0);
  }
  G_UNLOCK (running_plugins);
}

static plugin_instance * add_plugin (ladspa_plugin *plugin)
{
  plugin_instance *instance;
  char * line[1];
  gint row;

  if (plugin == NULL) {
    return NULL;
  }

  instance = load(plugin->filename, plugin->id);
  if (instance == NULL) {
    return NULL;
  }

  instance->stereo = plugin->stereo;
  if (state.srate && state.running) {
    /* Jump right in */
    boot_plugin(instance);
  }

  if (run_clist) {
    line[0] = (char *) instance->descriptor->Name;
    row = gtk_clist_append(GTK_CLIST (run_clist), line);
    gtk_clist_set_row_data(GTK_CLIST (run_clist), row, (gpointer) instance);
    gtk_clist_select_row(GTK_CLIST(run_clist), row, 0);
    draw_plugin(instance);
  }
  G_LOCK (running_plugins);
  running_plugins = g_slist_append(running_plugins, instance);
  G_UNLOCK (running_plugins);

  return instance;
}


static void unselect_plugin(GtkCList *clist, gint row, gint column,
                            GdkEventButton *event, gpointer user_data)
{
  selected_plugin= NULL;
}

static void select_plugin(GtkCList *clist, gint row, gint column,
                          GdkEventButton *event, gpointer user_data)
{
  selected_plugin = (ladspa_plugin *) gtk_clist_get_row_data(clist, row);
  gtk_clist_unselect_all(GTK_CLIST(run_clist));
  if (event->type == GDK_2BUTTON_PRESS) {
    /* Double click */
    add_plugin(selected_plugin);
  }
}

static GtkWidget * make_plugin_clist(void)
{
  ladspa_plugin *plugin;
  GSList *list;
  GtkWidget *clist;
  char number[14];
  char * titles[2] = { "UID", "Name" };
  char * line[2];
  gint row;
  
  find_all_plugins();

  clist = gtk_clist_new_with_titles(2, titles);
  gtk_clist_column_titles_active(GTK_CLIST (clist));
  gtk_clist_set_column_auto_resize (GTK_CLIST (clist), 0, TRUE);
  gtk_clist_set_sort_column(GTK_CLIST (clist), 1);

  for (list= plugin_list; list != NULL; list = g_slist_next(list)) {
    plugin = (ladspa_plugin *) list->data;
    snprintf(number, sizeof(number), "%ld", plugin->unique_id);
    line[0] = number;
    line[1] = plugin->name;
    row = gtk_clist_append(GTK_CLIST (clist), line);
    gtk_clist_set_row_data(GTK_CLIST (clist), row, (gpointer) plugin);
  }
  gtk_clist_sort(GTK_CLIST (clist));

  g_signal_connect(G_OBJECT(clist), "click-column",
                     G_CALLBACK(sort_column), NULL);
  g_signal_connect(G_OBJECT(clist), "select-row",
                     G_CALLBACK(select_plugin), NULL);
  g_signal_connect(G_OBJECT(clist), "unselect-row",
                     G_CALLBACK(unselect_plugin), NULL);

  return clist;
}

static void add_plugin_clicked (GtkButton *button, gpointer user_data)
{
  add_plugin(selected_plugin);
}

static void remove_plugin_clicked (GtkButton *button, gpointer user_data)
{
  plugin_instance *instance = selected_instance;
  gint row;

  if (instance == NULL) {
    return;
  }
  row = gtk_clist_find_row_from_data(GTK_CLIST(run_clist), (gpointer) instance);
  gtk_clist_remove(GTK_CLIST(run_clist), row);

  G_LOCK (running_plugins);
  running_plugins = g_slist_remove(running_plugins, instance);
  unload(instance);
  G_UNLOCK (running_plugins);
  selected_instance= NULL;
}

static void configure_plugin_clicked (GtkButton *button, gpointer user_data)
{
  if (selected_instance) {
    draw_plugin(selected_instance);
  }
}

static void configure(void)
{
  GtkWidget *widget, *vbox, *hbox, *bbox, *frame;

  if (config_window) {
    /* just show the window */
    gtk_widget_show(config_window);
    return;
  }

  config_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  vbox= gtk_vbox_new(FALSE, 0);
  hbox= gtk_hbox_new(TRUE, 0);

  frame= gtk_frame_new("Installed plugins");
  widget = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(widget), make_plugin_clist());
  gtk_container_add(GTK_CONTAINER(frame), widget);
  gtk_container_add(GTK_CONTAINER(hbox), frame);


  frame= gtk_frame_new("Running plugins");
  widget = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
                                 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  if (run_clist == NULL) {
    make_run_clist();
  }
  gtk_container_add(GTK_CONTAINER(widget), run_clist);
  gtk_container_add(GTK_CONTAINER(frame), widget);
  gtk_container_add(GTK_CONTAINER(hbox), frame);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);

  /* Buttons */
  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_SPREAD);
  widget = gtk_button_new_with_label("Add");
  g_signal_connect(G_OBJECT(widget), "clicked",
                     G_CALLBACK(add_plugin_clicked), NULL);
  gtk_box_pack_end_defaults(GTK_BOX(bbox), widget);
  widget = gtk_button_new_with_label("Remove");
  g_signal_connect(G_OBJECT(widget), "clicked",
                     G_CALLBACK(remove_plugin_clicked), NULL);
  gtk_box_pack_end_defaults(GTK_BOX(bbox), widget);
  widget = gtk_button_new_with_label("Configure");
  g_signal_connect(G_OBJECT(widget), "clicked",
                     G_CALLBACK(configure_plugin_clicked), NULL);
  gtk_box_pack_end_defaults(GTK_BOX(bbox), widget);

  gtk_box_pack_end(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(config_window), vbox);

  gtk_window_set_title(GTK_WINDOW(config_window), "LADSPA Plugin Catalog");
  gtk_widget_set_usize(config_window, 380, 400);
  g_signal_connect (G_OBJECT (config_window), "delete_event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

  gtk_widget_show_all(config_window);
}
