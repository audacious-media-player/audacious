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
#include "util.h"

#define REGISTRY_FILE "plugin-registry"

typedef struct
{
    gchar * path;
    gboolean confirmed;
    gint timestamp;
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
    gint priority;
}
OutputPluginData;

typedef struct
{
    gint type, number;
    gboolean confirmed;
    void * header;

    union
    {
        InputPluginData i;
        OutputPluginData o;
    }
    u;
}
PluginData;

const gchar * input_key_names[INPUT_KEYS] = {"scheme", "ext", "mime"};

static GList * module_list = NULL;

static ModuleData * module_new (gchar * path, gboolean confirmed,
 gint timestamp, gboolean loaded)
{
    ModuleData * module = g_malloc (sizeof (ModuleData));

    module->path = path;
    module->confirmed = confirmed;
    module->timestamp = timestamp;
    module->loaded = loaded;
    module->plugin_list = NULL;

    return module;
}

static PluginData * plugin_new (gint type, gint number, gboolean confirmed,
 void * header)
{
    PluginData * plugin = g_malloc (sizeof (PluginData));
    gint key;

    plugin->type = type;
    plugin->number = number;
    plugin->confirmed = confirmed;
    plugin->header = header;

    if (type == PLUGIN_TYPE_INPUT)
    {
        plugin->u.i.enabled = TRUE;
        plugin->u.i.priority = 0;

        for (key = 0; key < INPUT_KEYS; key ++)
            plugin->u.i.keys[key] = NULL;
    }
    else if (type == PLUGIN_TYPE_OUTPUT)
        plugin->u.o.priority = 0;

    return plugin;
}

static void plugin_free (PluginData * plugin)
{
    gint key;

    if (plugin->type == PLUGIN_TYPE_INPUT)
    {
        for (key = 0; key < INPUT_KEYS; key ++)
        {
            g_list_foreach (plugin->u.i.keys[key], (GFunc) g_free, NULL);
            g_list_free (plugin->u.i.keys[key]);
        }
    }

    g_free (plugin);
}

static void module_free (ModuleData * module)
{
    g_free (module->path);
    g_list_foreach (module->plugin_list, (GFunc) plugin_free, NULL);
    g_list_free (module->plugin_list);
    g_free (module);
}

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

static void output_plugin_save (PluginData * plugin, FILE * handle)
{
    fprintf (handle, "output %d\n", plugin->number);
    fprintf (handle, "priority %d\n", plugin->u.o.priority);
}

static void module_save (ModuleData * module, FILE * handle)
{
    GList * node;
    PluginData * plugin;

    for (node = module->plugin_list; node != NULL; node = node->next)
    {
        plugin = node->data;

        if (plugin->type != PLUGIN_TYPE_INPUT && plugin->type !=
         PLUGIN_TYPE_OUTPUT)
            return; /* we can't handle any other type (yet) */
    }

    fprintf (handle, "module %s\n", module->path);
    fprintf (handle, "stamp %d\n", module->timestamp);

    for (node = module->plugin_list; node != NULL; node = node->next)
    {
        plugin = node->data;

        if (plugin->type == PLUGIN_TYPE_INPUT)
            input_plugin_save (plugin, handle);
        else if (plugin->type == PLUGIN_TYPE_OUTPUT)
            output_plugin_save (plugin, handle);
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

    g_list_foreach (module_list, (GFunc) module_free, NULL);
    g_list_free (module_list);
    module_list = NULL;
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

    plugin = plugin_new (PLUGIN_TYPE_INPUT, number, FALSE, NULL);
    plugin->u.i.enabled = enabled;
    plugin->u.i.priority = priority;

    for (key = 0; key < INPUT_KEYS; key ++)
    {
        while (parse_string (input_key_names[key]))
        {
            plugin->u.i.keys[key] = g_list_prepend (plugin->u.i.keys[key],
             g_strdup (parse_value));
            parse_next (handle);
        }
    }

    module->plugin_list = g_list_prepend (module->plugin_list, plugin);
    return TRUE;
}

static gboolean output_plugin_parse (ModuleData * module, FILE * handle)
{
    PluginData * plugin;
    gint number, priority;

    if (! parse_integer ("output", & number))
        return FALSE;

    parse_next (handle);

    if (! parse_integer ("priority", & priority))
        return FALSE;

    parse_next (handle);

    plugin = plugin_new (PLUGIN_TYPE_OUTPUT, number, FALSE, NULL);
    plugin->u.o.priority = priority;

    module->plugin_list = g_list_prepend (module->plugin_list, plugin);
    return TRUE;
}

static gboolean module_parse (FILE * handle)
{
    ModuleData * module;
    gchar * path = NULL;
    gint timestamp;

    if (! parse_string ("module"))
        goto ERROR;

    path = g_strdup (parse_value);

    parse_next (handle);

    if (! parse_integer ("stamp", & timestamp))
        goto ERROR;

    parse_next (handle);

    module = module_new (path, FALSE, timestamp, FALSE);

    while (input_plugin_parse (module, handle) || output_plugin_parse (module,
     handle));

    module_list = g_list_prepend (module_list, module);
    return TRUE;

ERROR:
    g_free (path);
    return FALSE;
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

void plugin_registry_prune (void)
{
    GList * node, * next, * node2, * next2;
    ModuleData * module;
    PluginData * plugin;

    for (node = module_list; node != NULL; node = next)
    {
        module = node->data;
        next = node->next;

        if (! module->confirmed)
        {
            AUDDBG ("Module not found: %s\n", module->path);
            module_free (module);
            module_list = g_list_delete_link (module_list, node);
            continue;
        }

        if (! module->loaded)
            continue;

        for (node2 = module->plugin_list; node2 != NULL; node2 = next2)
        {
            plugin = node2->data;
            next2 = node2->next;

            if (plugin->confirmed)
                continue;

            AUDDBG ("Plugin not found: %s %d:%d\n", module->path, plugin->type,
             plugin->number);
            plugin_free (plugin);
            module->plugin_list = g_list_delete_link (module->plugin_list, node2);
        }
    }
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
    gint timestamp = file_get_mtime (path);

    if (timestamp < 0)
        return;

    module = module_lookup (path);

    if (module != NULL)
    {
        module->confirmed = TRUE;

        if (module->timestamp != timestamp)
        {
            AUDDBG ("Module rescan: %s\n", path);
            module->timestamp = timestamp;
            module->loaded = TRUE;
            module_load (path);
        }
    }
    else
    {
        AUDDBG ("New module: %s\n", path);
        module = module_new (g_strdup (path), TRUE, timestamp, TRUE);
        module_list = g_list_prepend (module_list, module);
        module_load (path);
    }
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
        plugin->confirmed = TRUE;
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
        AUDDBG ("New plugin: %s %d:%d\n", path, type, number);
        plugin = plugin_new (type, number, TRUE, header);
        module->plugin_list = g_list_prepend (module->plugin_list, plugin);
    }
}

void plugin_get_path (void * header, const gchar * * path, gint * type, gint *
 number)
{
    ModuleData * module;
    PluginData * plugin;

    plugin_find (header, & module, & plugin);
    * path = module->path;
    * type = plugin->type;
    * number = plugin->number;
}

void * plugin_by_path (const gchar * path, gint type, gint number)
{
    ModuleData * module;
    PluginData * plugin;

    module = module_lookup (path);

    if (module == NULL)
        return NULL;

    plugin = plugin_lookup (module, type, number);

    if (plugin == NULL)
        return NULL;

    module_check_loaded (module);
    return plugin->header;
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

void input_plugin_for_key (gint key, const gchar * value, gboolean (* func)
 (InputPlugin * plugin, void * data), void * data)
{
    gint priority;
    GList * node, * node2, * node3;
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

                for (node3 = plugin->u.i.keys[key]; node3 != NULL; node3 =
                 node3->next)
                {
                    if (strcmp (node3->data, value))
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

void output_plugin_set_priority (OutputPlugin * header, gint priority)
{
    ModuleData * module;
    PluginData * plugin;

    plugin_find (header, & module, & plugin);
    plugin->u.o.priority = priority;
}

void output_plugin_by_priority (gboolean (* func) (OutputPlugin * header, void *
 data), void * data)
{
    gint priority;
    GList * node, * node2;
    ModuleData * module;
    PluginData * plugin;

    for (priority = 10; priority >= 0; priority --)
    {
        for (node = module_list; node != NULL; node = node->next)
        {
            module = node->data;

            for (node2 = module->plugin_list; node2 != NULL; node2 = node2->next)
            {
                plugin = node2->data;

                if (plugin->type != PLUGIN_TYPE_OUTPUT || plugin->u.o.priority
                 != priority)
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
