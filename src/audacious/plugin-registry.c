/*
 * plugin-registry.c
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "pluginenum.h"
#include "plugin-registry.h"

#define REGISTRY_FILE "plugin-registry"

typedef struct
{
    gchar * path;
    gboolean loaded;
    GList * plugin_list;
}
ModuleData;

typedef struct
{
    gboolean enabled;
    gint priority;
    GList * keys[INPUT_KEYS];
}
InputPluginData;

typedef struct
{
    gint type, number;
    void * header;

    union
    {
        InputPluginData i;
    }
    u;
}
PluginData;

const gchar * input_key_names[INPUT_KEYS] = {"scheme", "ext", "mime"};

static GList * module_list = NULL;

static void input_plugin_save (PluginData * plugin, FILE * handle)
{
    gint key;
    GList * node;

    fprintf (handle, "input %d\n", plugin->number);
    fprintf (handle, "enabled %d\n", plugin->u.i.enabled);
    fprintf (handle, "priority %d\n", plugin->u.i.priority);

    for (key = 0; key < INPUT_KEYS; key ++)
    {
        for (node = plugin->u.i.keys[key]; node != NULL; node = node->next)
            fprintf (handle, "%s %s\n", input_key_names[key], (const gchar *)
             node->data);
    }
}

static void module_save (ModuleData * module, FILE * handle)
{
    GList * node;
    PluginData * plugin;

    for (node = module->plugin_list; node != NULL; node = node->next)
    {
        plugin = node->data;

        if (plugin->type != PLUGIN_TYPE_INPUT)
            return; /* we can't handle any other type (yet) */
    }

    fprintf (handle, "module %s\n", module->path);

    for (node = module->plugin_list; node != NULL; node = node->next)
    {
        plugin = node->data;

        if (plugin->type == PLUGIN_TYPE_INPUT)
            input_plugin_save (plugin, handle);
    }
}

void plugin_registry_save (void)
{
    gchar scratch[512];
    FILE * handle;
    GList * node;

    snprintf (scratch, sizeof scratch, "%s/" REGISTRY_FILE,
     aud_paths[BMP_PATH_USER_DIR]);
    handle = fopen (scratch, "w");

    if (handle == NULL)
        return;

    for (node = module_list; node != NULL; node = node->next)
        module_save (node->data, handle);

    fclose (handle);
}

static gchar parse_key[512];
static gchar * parse_value;

static void parse_next (FILE * handle)
{
    gchar * found;

    parse_value = NULL;

    if (fgets (parse_key, sizeof parse_key, handle) == NULL)
        return;

    found = strchr (parse_key, ' ');

    if (found == NULL)
        return;

    * found = 0;
    parse_value = found + 1;

    found = strchr (parse_value, '\n');

    if (found != NULL)
        * found = 0;
}

static gboolean parse_integer (const gchar * key, gint * value)
{
    return (parse_value != NULL && ! strcmp (parse_key, key) && sscanf
     (parse_value, "%d", value) == 1);
}

static gboolean parse_string (const gchar * key)
{
    return (parse_value != NULL && ! strcmp (parse_key, key));
}

static gboolean input_plugin_parse (ModuleData * module, FILE * handle)
{
    PluginData * plugin;
    gint number, enabled, priority, key;

    if (! parse_integer ("input", & number))
        return FALSE;

    parse_next (handle);

    if (! parse_integer ("enabled", & enabled))
        return FALSE;

    parse_next (handle);

    if (! parse_integer ("priority", & priority))
        return FALSE;

    parse_next (handle);

    plugin = g_malloc (sizeof (PluginData));
    plugin->type = PLUGIN_TYPE_INPUT;
    plugin->number = number;
    plugin->header = NULL;
    plugin->u.i.enabled = enabled;
    plugin->u.i.priority = priority;

    for (key = 0; key < INPUT_KEYS; key ++)
        plugin->u.i.keys[key] = NULL;

    module->plugin_list = g_list_prepend (module->plugin_list, plugin);

    for (key = 0; key < INPUT_KEYS; key ++)
    {
        while (parse_string (input_key_names[key]))
        {
            plugin->u.i.keys[key] = g_list_prepend (plugin->u.i.keys[key],
             g_strdup (parse_value));
            parse_next (handle);
        }
    }

    return TRUE;
}

static gboolean module_parse (FILE * handle)
{
    ModuleData * module;

    if (! parse_string ("module"))
        return FALSE;

    module = g_malloc (sizeof (ModuleData));
    module->path = g_strdup (parse_value);
    module->loaded = FALSE;
    module->plugin_list = NULL;

    module_list = g_list_prepend (module_list, module);

    parse_next (handle);

    while (input_plugin_parse (module, handle));

    return TRUE;
}

void plugin_registry_load (void)
{
    gchar scratch[512];
    FILE * handle;

    snprintf (scratch, sizeof scratch, "%s/" REGISTRY_FILE,
     aud_paths[BMP_PATH_USER_DIR]);
    handle = fopen (scratch, "r");

    if (handle == NULL)
        return;

    parse_next (handle);

    while (module_parse (handle));

    fclose (handle);
}

static ModuleData * module_lookup (const gchar * path)
{
    GList * node;
    ModuleData * module;

    for (node = module_list; node != NULL; node = node->next)
    {
        module = node->data;

        if (! strcmp (module->path, path))
            return module;
    }

    return NULL;
}

static void module_check_loaded (ModuleData * module)
{
    if (module->loaded)
        return;

    module->loaded = TRUE;
    module_load (module->path);
}

void module_register (const gchar * path)
{
    ModuleData * module;

    if (module_lookup (path) != NULL)
        return;

    module = g_malloc (sizeof (ModuleData));
    module->path = g_strdup (path);
    module->loaded = TRUE;
    module->plugin_list = NULL;

    module_list = g_list_prepend (module_list, module);

    module_load (path);
}

static PluginData * plugin_lookup (ModuleData * module, gint type, gint number)
{
    GList * node;
    PluginData * plugin;

    for (node = module->plugin_list; node != NULL; node = node->next)
    {
        plugin = node->data;

        if (plugin->type == type && plugin->number == number)
            return plugin;
    }

    return NULL;
}

static void plugin_find (void * header, ModuleData * * module, PluginData * *
 plugin)
{
    GList * node, * node2;

    for (node = module_list; node != NULL; node = node->next)
    {
        * module = node->data;

        for (node2 = (* module)->plugin_list; node2 != NULL; node2 = node2->next)
        {
            * plugin = node2->data;

            if ((* plugin)->header == header)
                return;
        }
    }

    abort ();
}

void plugin_register (const gchar * path, gint type, gint number, void * header)
{
    ModuleData * module;
    PluginData * plugin;
    gint key;

    module = module_lookup (path);

    if (module == NULL)
        return;

    plugin = plugin_lookup (module, type, number);

    if (plugin != NULL)
    {
        plugin->header = header;

        if (type == PLUGIN_TYPE_INPUT)
        {
            for (key = 0; key < INPUT_KEYS; key ++)
            {
                g_list_foreach (plugin->u.i.keys[key], (GFunc) g_free, NULL);
                g_list_free (plugin->u.i.keys[key]);
                plugin->u.i.keys[key] = NULL;
            }
        }
    }
    else
    {
        plugin = g_malloc (sizeof (PluginData));
        plugin->type = type;
        plugin->number = number;
        plugin->header = header;

        if (type == PLUGIN_TYPE_INPUT)
        {
            plugin->u.i.enabled = TRUE;
            plugin->u.i.priority = 0;

            for (key = 0; key < INPUT_KEYS; key ++)
                plugin->u.i.keys[key] = NULL;
        }

        module->plugin_list = g_list_prepend (module->plugin_list, plugin);
    }
}

void plugin_for_each (gint type, gboolean (* func) (void * header, void * data),
 void * data)
{
    GList * node, * node2;
    ModuleData * module;
    PluginData * plugin;

    for (node = module_list; node != NULL; node = node->next)
    {
        module = node->data;

        for (node2 = module->plugin_list; node2 != NULL; node2 = node2->next)
        {
            plugin = node2->data;

            if (plugin->type != type)
                continue;

            module_check_loaded (module);

            if (plugin->header == NULL)
                continue;

            if (! func (plugin->header, data))
                return;
        }
    }
}

void input_plugin_set_enabled (InputPlugin * header, gboolean enabled)
{
    ModuleData * module;
    PluginData * plugin;

    plugin_find (header, & module, & plugin);
    plugin->u.i.enabled = enabled;
}

gboolean input_plugin_get_enabled (InputPlugin * header)
{
    ModuleData * module;
    PluginData * plugin;

    plugin_find (header, & module, & plugin);
    return plugin->u.i.enabled;
}

void input_plugin_set_priority (InputPlugin * header, gint priority)
{
    ModuleData * module;
    PluginData * plugin;

    plugin_find (header, & module, & plugin);
    plugin->u.i.priority = priority;
}

void input_plugin_by_priority (gboolean (* func) (InputPlugin * header, void *
 data), void * data)
{
    gint priority;
    GList * node, * node2;
    ModuleData * module;
    PluginData * plugin;

    for (priority = 0; priority <= 10; priority ++)
    {
        for (node = module_list; node != NULL; node = node->next)
        {
            module = node->data;

            for (node2 = module->plugin_list; node2 != NULL; node2 = node2->next)
            {
                plugin = node2->data;

                if (plugin->type != PLUGIN_TYPE_INPUT || ! plugin->u.i.enabled
                 || plugin->u.i.priority != priority)
                    continue;

                module_check_loaded (module);

                if (plugin->header == NULL)
                    continue;

                if (! func (plugin->header, data))
                    return;
            }
        }
    }
}

void input_plugin_add_keys (InputPlugin * header, gint key, GList * values)
{
    ModuleData * module;
    PluginData * plugin;

    plugin_find (header, & module, & plugin);
    plugin->u.i.keys[key] = g_list_concat (plugin->u.i.keys[key], values);
}

InputPlugin * input_plugin_for_key (gint key, const gchar * value)
{
    GList * node, * node2, * node3;
    ModuleData * module;
    PluginData * plugin;

    for (node = module_list; node != NULL; node = node->next)
    {
        module = node->data;

        for (node2 = module->plugin_list; node2 != NULL; node2 = node2->next)
        {
            plugin = node2->data;

            if (plugin->type != PLUGIN_TYPE_INPUT || ! plugin->u.i.enabled)
                continue;

            for (node3 = plugin->u.i.keys[key]; node3 != NULL; node3 =
             node3->next)
            {
                if (strcmp (node3->data, value))
                    continue;

                module_check_loaded (module);

                return plugin->header;
            }
        }
    }

    return NULL;
}

void input_plugin_add_scheme_compat (const gchar * scheme, InputPlugin * header)
{
    input_plugin_add_keys (header, INPUT_KEY_SCHEME, g_list_prepend (NULL,
     g_strdup (scheme)));
}

void input_plugin_add_mime_compat (const gchar * mime, InputPlugin * header)
{
    input_plugin_add_keys (header, INPUT_KEY_MIME, g_list_prepend (NULL,
     g_strdup (mime)));
}
