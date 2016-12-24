/*
 * menu.c
 * Copyright 2011-2014 John Lindgren
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

#include "menu.h"

#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

static GtkWidget * image_menu_item_new (const char * text, const char * icon)
{
    GtkWidget * widget = gtk_image_menu_item_new_with_mnemonic (text);

    if (icon)
    {
        GtkWidget * image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image ((GtkImageMenuItem *) widget, image);
    }

    return widget;
}

static void toggled_cb (GtkCheckMenuItem * check, const AudguiMenuItem * item)
{
    gboolean on = gtk_check_menu_item_get_active (check);

    if (aud_get_bool (item->csect, item->cname) == on)
        return;

    aud_set_bool (item->csect, item->cname, on);

    if (item->func)
        item->func ();
}

static void hook_cb (void * data, GtkWidget * check)
{
    const AudguiMenuItem * item = (const AudguiMenuItem *) g_object_get_data
     ((GObject *) check, "item");
    gtk_check_menu_item_set_active ((GtkCheckMenuItem *) check, aud_get_bool
     (item->csect, item->cname));
}

static void unhook_cb (GtkCheckMenuItem * check, const AudguiMenuItem * item)
{
    hook_dissociate (item->hook, (HookFunction) hook_cb, check);
}

EXPORT GtkWidget * audgui_menu_item_new_with_domain
 (const AudguiMenuItem * item, GtkAccelGroup * accel, const char * domain)
{
    const char * name = (domain && item->name) ? dgettext (domain, item->name) : item->name;
    GtkWidget * widget = nullptr;

    if (name && item->func && ! item->cname) /* normal widget */
    {
        widget = image_menu_item_new (name, item->icon);
        g_signal_connect (widget, "activate", item->func, nullptr);
    }
    else if (name && item->cname) /* toggle widget */
    {
        widget = gtk_check_menu_item_new_with_mnemonic (name);
        gtk_check_menu_item_set_active ((GtkCheckMenuItem *) widget,
         aud_get_bool (item->csect, item->cname));
        g_signal_connect (widget, "toggled", (GCallback) toggled_cb, (void *) item);

        if (item->hook)
        {
            g_object_set_data ((GObject *) widget, "item", (void *) item);
            hook_associate (item->hook, (HookFunction) hook_cb, widget);
            g_signal_connect (widget, "destroy", (GCallback) unhook_cb, (void *) item);
        }
    }
    else if (name && (item->items.len || item->get_sub)) /* submenu */
    {
        widget = image_menu_item_new (name, item->icon);

        GtkWidget * sub;

        if (item->get_sub)
            sub = item->get_sub ();
        else
        {
            sub = gtk_menu_new ();
            audgui_menu_init_with_domain (sub, item->items, accel, domain);
        }

        gtk_menu_item_set_submenu ((GtkMenuItem *) widget, sub);
    }
    else if (item->sep) /* separator */
        widget = gtk_separator_menu_item_new ();

    if (widget && accel && item->key)
        gtk_widget_add_accelerator (widget, "activate", accel, item->key,
         item->mod, GTK_ACCEL_VISIBLE);

    return widget;
}

EXPORT void audgui_menu_init_with_domain (GtkWidget * shell,
 ArrayRef<AudguiMenuItem> items, GtkAccelGroup * accel,
 const char * domain)
{
    for (const AudguiMenuItem & item : items)
    {
        GtkWidget * widget = audgui_menu_item_new_with_domain (& item, accel, domain);
        if (! widget)
            continue;

        gtk_widget_show (widget);
        gtk_menu_shell_append ((GtkMenuShell *) shell, widget);
    }
}
