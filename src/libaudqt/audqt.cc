/*
 * audqt.cc
 * Copyright 2014-2021 Ariadne Conill and John Lindgren
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

#include <QApplication>
#include <QLibraryInfo>
#include <QProxyStyle>
#include <QPushButton>
#include <QScreen>
#include <QStyleFactory>
#include <QTranslator>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/runtime.h>

#include "libaudqt-internal.h"
#include "libaudqt.h"

namespace audqt
{

static int init_count;

static PixelSizes sizes_local;
static PixelMargins margins_local;

EXPORT const PixelSizes & sizes = sizes_local;
EXPORT const PixelMargins & margins = margins_local;

/* clang-format off */
static const char * const audqt_defaults[] = {
    "eq_presets_visible", "FALSE",
    "equalizer_visible", "FALSE",
    "queue_manager_visible", "FALSE",
    "close_jtf_dialog", "TRUE",
#ifdef _WIN32
    "theme", "dark",
    "icon_theme", "audacious-flat-dark",
#endif
    nullptr
};
/* clang-format on */

static void load_qt_translations()
{
    static QTranslator translators[2];

    QLocale locale = QLocale::system();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QString dir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#else
    QString dir = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#endif

    if (translators[0].load(locale, "qt", "_", dir))
        QApplication::installTranslator(&translators[0]);
    if (translators[1].load(locale, "qtbase", "_", dir))
        QApplication::installTranslator(&translators[1]);
}

void set_icon_theme()
{
    // Make sure that ":/icons" is in the theme search path. In Qt 6 it
    // is added at startup (see QIconLoader::themeSearchPaths()) but in
    // some cases gets removed later (see QIconLoader::setThemeName()).
    // This seems to be a regression since Qt 5.
    auto themePaths = QIcon::themeSearchPaths();
    if (!themePaths.contains(":/icons"))
        QIcon::setThemeSearchPaths(themePaths << ":/icons");

    QIcon::setThemeName((QString)aud_get_str("audqt", "icon_theme"));

    // make sure we have a valid icon theme
    auto isValid = [](QString theme) {
        return !theme.isEmpty() && theme != "hicolor";
    };

    if (!isValid(QIcon::themeName()))
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        QString fallback = QIcon::fallbackThemeName();
        if (isValid(fallback))
            QIcon::setThemeName(fallback);
        else
#endif
            QIcon::setThemeName("audacious-flat");
    }

    // add fallback icons just to be sure
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    auto paths = QIcon::fallbackSearchPaths();
    auto path = ":/icons/audacious-flat/scalable";
    if (!paths.contains(path))
        QIcon::setFallbackSearchPaths(paths << path);
#endif

    qApp->setWindowIcon(QIcon::fromTheme("audacious"));
}

EXPORT void init()
{
    if (init_count++)
        return;

    aud_config_set_defaults("audqt", audqt_defaults);
    log_init();

    // The QApplication instance is created only once and is not deleted
    // by audqt::cleanup(). If it already exists, we are done here.
    if (qApp)
        return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && defined(_WIN32)
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::Floor);
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Use X11/XWayland by default, but allow to overwrite it.
    // Especially the Winamp interface is not usable yet on Wayland
    // due to limitations regarding application-side window positioning.
    auto platform = qgetenv("QT_QPA_PLATFORM");
    if (platform.isEmpty() && qEnvironmentVariableIsSet("DISPLAY"))
        qputenv("QT_QPA_PLATFORM", "xcb");
    else if (platform != "xcb")
        AUDWARN("X11/XWayland was not detected. This is unsupported, "
                "please do not report bugs.\n");
#endif

    static char app_name[] = "audacious";
    static int dummy_argc = 1;
    static char * dummy_argv[] = {app_name, nullptr};

    auto qapp = new QApplication(dummy_argc, dummy_argv);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qapp->setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    qapp->setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif // >= 5.10
#endif // < 6.0
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    qapp->setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);
#endif

    qapp->setApplicationName(_("Audacious"));
    qapp->setQuitOnLastWindowClosed(false);

    sizes_local.OneInch =
        aud::max(96, (int)qapp->primaryScreen()->logicalDotsPerInch());
    sizes_local.TwoPt = aud::rescale(2, 72, sizes_local.OneInch);
    sizes_local.FourPt = aud::rescale(4, 72, sizes_local.OneInch);
    sizes_local.EightPt = aud::rescale(8, 72, sizes_local.OneInch);

    margins_local.TwoPt =
        QMargins(sizes.TwoPt, sizes.TwoPt, sizes.TwoPt, sizes.TwoPt);
    margins_local.FourPt =
        QMargins(sizes.FourPt, sizes.FourPt, sizes.FourPt, sizes.FourPt);
    margins_local.EightPt =
        QMargins(sizes.EightPt, sizes.EightPt, sizes.EightPt, sizes.EightPt);

    load_qt_translations();
    set_icon_theme();

    if (!strcmp(aud_get_str("audqt", "theme"), "dark"))
        enable_dark_theme();

#ifdef _WIN32
    // On Windows, Qt uses 9 pt in specific places (such as QMenu) but
    // 8 pt as the application font, resulting in an inconsistent look.
    // First-party Windows applications (and GTK applications too) seem
    // to use 9 pt in most places so let's try to do the same.
    QApplication::setFont(QApplication::font("QMenu"));
#endif
#ifdef Q_OS_MAC
    // Mac-specific font tweaks
    QApplication::setFont(QApplication::font("QSmallFont"), "QDialog");
    QApplication::setFont(QApplication::font("QSmallFont"), "QTreeView");
    QApplication::setFont(QApplication::font("QTipLabel"), "QStatusBar");

    // Handle MacOS dock activation (AppKit applicationShouldHandleReopen)
    QObject::connect(qapp, &QApplication::applicationStateChanged, [](auto state) {
        if (state == Qt::ApplicationState::ApplicationActive)
            aud_ui_show(true);
    });
#endif
}

EXPORT void cleanup()
{
    if (--init_count)
        return;

    aboutwindow_hide();
    infopopup_hide_now();
    infowin_hide();
    log_inspector_hide();
    plugin_prefs_hide();
    prefswin_hide();
    songwin_hide();

    log_cleanup();

    // We do not delete the QApplication here due to issues that arise
    // if it is deleted and then re-created. Instead, it is deleted
    // later during shutdown; see mainloop_cleanup() in libaudcore.
}

EXPORT QGradientStops dark_bg_gradient(const QColor & base)
{
    constexpr int s[4] = {40, 28, 16, 24};

    QColor c[4];
    for (int i = 0; i < 4; i++)
        c[i] = QColor(s[i], s[i], s[i]);

    /* in a dark theme, try to match the tone of the base color */
    int v = base.value();
    if (v >= 10 && v < 80)
    {
        int r = base.red(), g = base.green(), b = base.blue();

        for (int i = 0; i < 4; i++)
        {
            c[i] = QColor(r * s[i] / v, g * s[i] / v, b * s[i] / v);
        }
    }

    return {{0, c[0]}, {0.45, c[1]}, {0.55, c[2]}, {1, c[3]}};
}

EXPORT QColor vis_bar_color(const QColor & hue, int bar, int n_bars)
{
    decltype(hue.hueF()) h, s, v;
    hue.getHsvF(&h, &s, &v);

    if (s < 0.1) /* monochrome? use blue instead */
        h = 0.67;

    s = 1 - 0.9 * bar / (n_bars - 1);
    v = 0.75 + 0.25 * bar / (n_bars - 1);

    return QColor::fromHsvF(h, s, v);
}

EXPORT QHBoxLayout * make_hbox(QWidget * parent, int spacing)
{
    auto layout = new QHBoxLayout(parent);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing);
    return layout;
}

EXPORT QVBoxLayout * make_vbox(QWidget * parent, int spacing)
{
    auto layout = new QVBoxLayout(parent);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing);
    return layout;
}

EXPORT void setup_proxy_style(QProxyStyle * style)
{
    // set the correct base style (dark or native)
    if (!strcmp(aud_get_str("audqt", "theme"), "dark"))
        style->setBaseStyle(create_dark_style());
    else
        style->setBaseStyle(nullptr);

    // detect and respond to application-wide style change
    QObject::connect(qApp->style(), &QObject::destroyed, style,
                     [style]() { setup_proxy_style(style); });
}

EXPORT void enable_layout(QLayout * layout, bool enabled)
{
    int count = layout->count();
    for (int i = 0; i < count; i++)
    {
        auto item = layout->itemAt(i);
        if (QLayout * layout2 = item->layout())
            enable_layout(layout2, enabled);
        if (QWidget * widget = item->widget())
            widget->setEnabled(enabled);
    }
}

EXPORT void clear_layout(QLayout * layout)
{
    while (QLayoutItem * item = layout->takeAt(0))
    {
        if (QLayout * layout2 = item->layout())
            clear_layout(layout2);
        if (QWidget * widget = item->widget())
            delete widget;

        delete item;
    }
}

/* the goal is to force a window to come to the front on any Qt platform */
EXPORT void window_bring_to_front(QWidget * window)
{
    window->show();

    Qt::WindowStates state = window->windowState();

    state &= ~Qt::WindowMinimized;
    state |= Qt::WindowActive;

    window->setWindowState(state);
    window->raise();
    window->activateWindow();
}

EXPORT void simple_message(const char * title, const char * text)
{
    simple_message(title, text, QMessageBox::NoIcon);
}

EXPORT void simple_message(const char * title, const char * text,
                           QMessageBox::Icon icon)
{
    auto msgbox = new QMessageBox(icon, title, text, QMessageBox::Close);
    msgbox->button(QMessageBox::Close)->setText(translate_str(N_("_Close")));
    msgbox->setAttribute(Qt::WA_DeleteOnClose);
    msgbox->setTextInteractionFlags(Qt::TextSelectableByMouse);
    msgbox->setWindowRole("message");
    msgbox->show();
}

/* translate GTK+ accelerators and also handle dgettext() */
EXPORT QString translate_str(const char * str, const char * domain)
{
    /* handle null and empty strings */
    if (!str || !str[0])
        return QString(str);

    /* translate the GTK+ accelerator (_) into a Qt accelerator (&) */
    return QString(dgettext(domain, str)).replace('_', '&');
}

} // namespace audqt
