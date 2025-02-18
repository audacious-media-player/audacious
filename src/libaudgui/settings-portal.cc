/*
 * settings-portal.cc
 * Copyright 2025 Thomas Lange
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <gio/gio.h>

#include <libaudcore/runtime.h>

#include "internal.h"

enum ColorScheme {
    DEFAULT,
    PREFER_DARK,
    PREFER_LIGHT
};

static GDBusProxy * settings_portal;

static int get_api_version (GDBusProxy * proxy)
{
    GVariant * variant = g_dbus_proxy_get_cached_property (proxy, "version");
    if (! variant)
        return -1;

    unsigned int version = g_variant_get_uint32 (variant);
    g_variant_unref (variant);
    return version;
}

static bool read_color_scheme (GDBusProxy * proxy, GVariant ** out)
{
    int api_version = get_api_version (proxy);
    if (api_version < 1)
        return false;

    const char * method = api_version == 1 ? "Read" : "ReadOne";
    GError * error = nullptr;

    GVariant * result = g_dbus_proxy_call_sync (proxy, method,
     g_variant_new ("(ss)", "org.freedesktop.appearance", "color-scheme"),
     G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, & error);

    if (! result)
    {
        AUDINFO ("Failed to read the color-scheme setting: %s\n", error->message);
        g_error_free (error);
        return false;
    }

    if (api_version == 1)
    {
        GVariant * child;
        g_variant_get (result, "(v)", & child);
        g_variant_get (child, "v", out);
        g_variant_unref (child);
    }
    else
        g_variant_get (result, "(v)", out);

    g_variant_unref (result);
    return true;
}

static void set_color_scheme (GVariant * variant)
{
    bool prefer_dark_theme = g_variant_get_uint32 (variant) == PREFER_DARK;

    g_object_set ((GObject *) gtk_settings_get_default (),
     "gtk-application-prefer-dark-theme", prefer_dark_theme, nullptr);
}

static void portal_changed_cb (GDBusProxy * proxy, const char * sender_name,
 const char * signal_name, GVariant * parameters, void * data)
{
    const char * name_space;
    const char * name;
    GVariant * color_scheme;

    if (g_strcmp0 (signal_name, "SettingChanged"))
        return;

    g_variant_get (parameters, "(&s&sv)", & name_space, & name, & color_scheme);

    if (g_strcmp0 (name_space, "org.freedesktop.appearance") ||
        g_strcmp0 (name, "color-scheme"))
    {
        g_variant_unref (color_scheme);
        return;
    }

    set_color_scheme (color_scheme);
    g_variant_unref (color_scheme);
}

void portal_init ()
{
    GVariant * color_scheme;
    GError * error = nullptr;

    settings_portal = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
     G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.freedesktop.portal.Desktop",
     "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Settings",
     nullptr, & error);

    if (! settings_portal)
    {
        AUDINFO ("Failed to initialize settings portal: %s\n", error->message);
        g_error_free (error);
        return;
    }

    if (! read_color_scheme (settings_portal, & color_scheme))
        return;

    set_color_scheme (color_scheme);
    g_variant_unref (color_scheme);

    g_signal_connect (settings_portal, "g-signal", (GCallback) portal_changed_cb, nullptr);
}

void portal_cleanup ()
{
    if (settings_portal)
        g_object_unref (settings_portal);
}
