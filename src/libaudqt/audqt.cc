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
#include <QDebug>
#include <QStandardPaths>
#include <QUrl>
#include <QIcon>
#include <QStyle>
#include <QHash>

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

    // call the local QApplication instance "app" to avoid
    // confusion with the qApp macro.
    auto app = new QApplication (dummy_argc, dummy_argv);

    // stuff that could be done in qapp's ctor:
    app->setAttribute (Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    app->setAttribute (Qt::AA_ForceRasterWidgets);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    app->setAttribute (Qt::AA_UseStyleSheetPropagationInWidgetStyles);
#endif

    app->setApplicationName (_("Audacious"));
    if (app->windowIcon ().isNull ()) {
        app->setWindowIcon (audqt::get_icon (app_name));
    }

    // set up our font preferences for the various classes used in our widgets
    // Uses the get_font_for_class() wrapper defined below which hides platform-
    // specific choices (Mac vs. all others at this time). This is why certain
    // setFont calls seem to confirm the default and thus to be redundant.
    // Note that widget classes like InfoWidget still have to call
    // `setFont (get_font_for_class (classname));`explicitly
    // even if the inherit QObject (though possibly not when the class definition
    // includes the Q_OBJECT macro). Widget classes that do not inherit a QWidget
    // create one as a member variable will evidently have to set the selected
    // font explicitly too (e.g. QueueManagerDialog).
    QApplication::setFont (get_font_for_class ("QDialog"), "QDialog");
    QApplication::setFont (get_font_for_class ("QTreeView"), "QTreeView");
    QApplication::setFont (get_font_for_class ("QSmallFont"), "InfoWidget");
    QApplication::setFont (get_font_for_class ("QSmallFont"), "LogEntryInspector");
    QApplication::setFont (get_font_for_class ("QSmallFont"), "QueueManagerDialog");
    // the font used for (tool) tip labels is a good choice for the lyrics widget too
    QApplication::setFont (get_font_for_class ("QTipLabel"), "LyricWikiQt");
    // idem for the status bar (QTipLabel will be more consistently small but readable
    // across the supported platforms).
    QApplication::setFont (get_font_for_class ("QTipLabel"), "StatusBar");
    // are QHeaderViews used in situations where QSmallFont is not appropriate?
    QApplication::setFont (get_font_for_class ("QSmallFont"), "PlaylistHeader");

    auto desktop = app->desktop ();
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
    equalizer_hide ();
    infowin_hide ();
    log_inspector_hide ();
    prefswin_hide ();
    queue_manager_hide ();

    log_cleanup ();

    // this is also where we need to delete our QApplication instance
    // deleting qApp looks weird but should be safe.
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

EXPORT QFont get_font_for_class (const char *className)
{
    QString currentStyle = qApp->style ()->objectName();
    if (currentStyle.compare ("Macintosh", Qt::CaseInsensitive) == 0
        || currentStyle.contains ("Aqua", Qt::CaseInsensitive))
    {
        static QHash<const QString, const char *> map = {
            { "QDialog", "QSmallFont" },
            { "QListBox", "QSmallFont" },
            { "QListView", "QSmallFont" },
            { "QTreeView", "QSmallFont" },
        };
        if (map.contains (className))
        {
            QFont ret = QApplication::font (map[className]);
            qDebug() << Q_FUNC_INFO << className << "->" << map[className] << "->" << ret.toString();
            return ret;
        }
    }
    return QApplication::font (className);
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
