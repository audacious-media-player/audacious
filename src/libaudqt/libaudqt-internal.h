/*
 * libaudqt-internal.h
 * Copyright 2016-2017 John Lindgren
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

#ifndef LIBAUDQT_INTERNAL_H
#define LIBAUDQT_INTERNAL_H

#include <QWidget>

class QPoint;
class QScreen;
class QString;
class QStyle;

namespace audqt
{

/* audqt.cc */
void set_icon_theme();

/* dark-theme.cc */
QStyle *create_dark_style();
void enable_dark_theme();
void disable_dark_theme();

/* dock.cc */
void dock_show_simple(const char * id, const char * name, QWidget * create());
void dock_hide_simple(const char * id);

/* infopopup.cc */
void infopopup_hide_now();

/* log-inspector.cc */
void log_init();
void log_cleanup();

/* prefs-plugin.cc */
void plugin_prefs_hide();

/* util-qt.cc */
class PopupWidget : public QWidget
{
public:
    PopupWidget(QWidget * parent = nullptr);

protected:
    bool eventFilter(QObject *, QEvent * e) override;
    void showEvent(QShowEvent *) override;
};

void show_copy_context_menu(QWidget * parent, const QPoint & global_pos,
                            const QString & text_to_copy);

} // namespace audqt

#endif // LIBAUDQT_INTERNAL_H
