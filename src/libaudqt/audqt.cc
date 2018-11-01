/*
 * util.cc
 * Copyright 2014 William Pitcock
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

#include <stdlib.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "libaudqt-internal.h"
#include "libaudqt.h"

namespace audqt {

static int init_count;

static PixelSizes sizes_local;
static PixelMargins margins_local;

EXPORT const PixelSizes & sizes = sizes_local;
EXPORT const PixelMargins & margins = margins_local;

EXPORT void init ()
{
    if (init_count ++)
        return;

    static char app_name[] = "audacious";
    static int dummy_argc = 1;
    static char * dummy_argv[] = {app_name, nullptr};

    auto qapp = new QApplication (dummy_argc, dummy_argv);

    qapp->setAttribute (Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    qapp->setAttribute (Qt::AA_ForceRasterWidgets);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    qapp->setAttribute (Qt::AA_UseStyleSheetPropagationInWidgetStyles);
#endif

    qapp->setApplicationName (_("Audacious"));
    if (qapp->windowIcon ().isNull ())
        qapp->setWindowIcon (audqt::get_icon (app_name));

    qapp->setQuitOnLastWindowClosed (false);

    auto desktop = qapp->desktop ();
    sizes_local.OneInch = aud::max (96, (desktop->logicalDpiX () + desktop->logicalDpiY ()) / 2);
    sizes_local.TwoPt = aud::rescale (2, 72, sizes_local.OneInch);
    sizes_local.FourPt = aud::rescale (4, 72, sizes_local.OneInch);
    sizes_local.EightPt = aud::rescale (8, 72, sizes_local.OneInch);

    margins_local.TwoPt = QMargins (sizes.TwoPt, sizes.TwoPt, sizes.TwoPt, sizes.TwoPt);
    margins_local.FourPt = QMargins (sizes.FourPt, sizes.FourPt, sizes.FourPt, sizes.FourPt);
    margins_local.EightPt = QMargins (sizes.EightPt, sizes.EightPt, sizes.EightPt, sizes.EightPt);

#ifdef Q_OS_MAC  // Mac-specific font tweaks
    QApplication::setFont (QApplication::font ("QSmallFont"), "QDialog");
    QApplication::setFont (QApplication::font ("QSmallFont"), "QTreeView");
    QApplication::setFont (QApplication::font ("QTipLabel"), "QStatusBar");
#endif

    log_init ();
}

EXPORT void run ()
{
    qApp->exec ();
}

EXPORT void quit ()
{
    qApp->quit ();
}

EXPORT void cleanup ()
{
    if (-- init_count)
        return;

    aboutwindow_hide ();
    eq_presets_hide ();
    equalizer_hide ();
    infopopup_hide_now ();
    infowin_hide ();
    log_inspector_hide ();
    prefswin_hide ();
    queue_manager_hide ();

    log_cleanup ();

    delete qApp;
}

EXPORT QIcon get_icon (const char * name)
{
    auto icon = QIcon::fromTheme (name);

    if (icon.isNull ())
        icon = QIcon (QString (":/") + name + ".svg");

    return icon;
}

EXPORT QHBoxLayout * make_hbox (QWidget * parent, int spacing)
{
    auto layout = new QHBoxLayout (parent);
    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (spacing);
    return layout;
}

EXPORT QVBoxLayout * make_vbox (QWidget * parent, int spacing)
{
    auto layout = new QVBoxLayout (parent);
    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (spacing);
    return layout;
}

EXPORT void enable_layout (QLayout * layout, bool enabled)
{
    int count = layout->count ();
    for (int i = 0; i < count; i ++)
    {
        auto item = layout->itemAt (i);
        if (QLayout * layout2 = item->layout ())
            enable_layout (layout2, enabled);
        if (QWidget * widget = item->widget ())
            widget->setEnabled (enabled);
    }
}

EXPORT void clear_layout (QLayout * layout)
{
    while (QLayoutItem * item = layout->takeAt (0))
    {
        if (QLayout * layout2 = item->layout ())
            clear_layout (layout2);
        if (QWidget * widget = item->widget ())
            delete widget;

        delete item;
    }
}

/* the goal is to force a window to come to the front on any Qt platform */
EXPORT void window_bring_to_front (QWidget * window)
{
    window->show ();

    Qt::WindowStates state = window->windowState ();

    state &= ~Qt::WindowMinimized;
    state |= Qt::WindowActive;

    window->setWindowState (state);
    window->raise ();
    window->activateWindow ();
}

EXPORT void simple_message (const char * title, const char * text)
{
    simple_message (title, text, QMessageBox::NoIcon);
}

EXPORT void simple_message (const char * title, const char * text, QMessageBox::Icon icon)
{
    auto msgbox = new QMessageBox (icon, title, text, QMessageBox::Close);
    msgbox->button (QMessageBox::Close)->setText (translate_str (N_("_Close")));
    msgbox->setAttribute (Qt::WA_DeleteOnClose);
    msgbox->show ();
}

/* translate GTK+ accelerators and also handle dgettext() */
EXPORT QString translate_str (const char * str, const char * domain)
{
    /* handle null and empty strings */
    if (! str || ! str[0])
        return QString (str);

    /* translate the GTK+ accelerator (_) into a Qt accelerator (&) */
    return QString (dgettext (domain, str)).replace ('_', '&');
}

} // namespace audqt
