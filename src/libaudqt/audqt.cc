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

// wrap QApplication only so we can generate a debug trace
class AudApplication : public QApplication
{
public:
    AudApplication (int &argc, char **argv)
        : QApplication (argc, argv)
    {
        setAttribute (Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
        setAttribute (Qt::AA_ForceRasterWidgets);
#endif

        setApplicationName (_("Audacious"));
        if (windowIcon().isNull()) {
            QIcon appIcon = get_icon ("audacious.svg");
            setWindowIcon (appIcon);
        }
        setQuitOnLastWindowClosed (true);
    }
    ~AudApplication()
    {
        qDebug() << Q_FUNC_INFO << "Destroying" << this;
    }
};

static AudApplication * qapp;

static PixelSizes sizes_local;
static PixelMargins margins_local;

EXPORT const PixelSizes & sizes = sizes_local;
EXPORT const PixelMargins & margins = margins_local;

EXPORT void init ()
{
    if (init_count ++ || qapp)
        return;

    static char app_name[] = "audacious";
    static int dummy_argc = 1;
    static char * dummy_argv[] = {app_name, nullptr};

    qapp = new AudApplication (dummy_argc, dummy_argv);

    auto desktop = qapp->desktop ();
    sizes_local.OneInch = aud::max (96, (desktop->logicalDpiX () + desktop->logicalDpiY ()) / 2);
    sizes_local.TwoPt = aud::rescale (2, 72, sizes_local.OneInch);
    sizes_local.FourPt = aud::rescale (4, 72, sizes_local.OneInch);
    sizes_local.EightPt = aud::rescale (8, 72, sizes_local.OneInch);

    margins_local.TwoPt = QMargins (sizes.TwoPt, sizes.TwoPt, sizes.TwoPt, sizes.TwoPt);
    margins_local.FourPt = QMargins (sizes.FourPt, sizes.FourPt, sizes.FourPt, sizes.FourPt);
    margins_local.EightPt = QMargins (sizes.EightPt, sizes.EightPt, sizes.EightPt, sizes.EightPt);

    log_init ();
}

EXPORT void run ()
{
    qapp->exec ();
}

EXPORT void quit ()
{
    qapp->quit ();
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
    // we should delete the application instance here rather than allowing that
    // to be done at some time outside our control during global destruction.
    delete qapp;
    // avoid leaving a stale global variable around
    qapp = nullptr;

    log_cleanup ();
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
    QString currentStyle = qapp->style ()->objectName();
    if (currentStyle.compare ("Macintosh", Qt::CaseInsensitive) == 0
        || currentStyle.contains ("Aqua", Qt::CaseInsensitive))
    {
        static QHash<const QString, const char *> map = {
            { "QListView", "QSmallFont" },
            { "QListBox", "QSmallFont" },
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

// icon lookup function that returns the requested icon
// from the embedded Qt resource. This function has to be
// provided by the library that contains the resource.
// If the resource lookup fails, try to find the icon in the
// images datadir.
EXPORT QIcon get_icon (const char *filename)
{
    QString resourceName = QStringLiteral (":/%1").arg (filename);
    QIcon icon (resourceName);
    if (icon.isNull ())
    {
        const char * data_dir = aud_get_path (AudPath::DataDir);
        const char * img_path = filename_build ({data_dir, "images", filename});
        icon = QIcon (img_path);
        qDebug() << Q_FUNC_INFO << filename << "->" << img_path << "->" << icon;
    }
    else
    {
        qDebug() << Q_FUNC_INFO << filename << "->" << resourceName << "->" << icon;
    }
    return icon;
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
