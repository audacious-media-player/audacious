/*
 * util-qt.cc
 * Copyright 2017 René Bertin and John Lindgren
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

#include "libaudqt-internal.h"
#include "libaudqt.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QMenu>
#include <QMimeData>
#include <QScreen>
#include <QWindow>

#include <libaudcore/i18n.h>

namespace audqt
{

PopupWidget::PopupWidget(QWidget * parent) : QWidget(parent)
{
    qApp->installEventFilter(this);
}

// This event filter mimics QToolTip by hiding the popup widget when
// certain events are received by any widget.
bool PopupWidget::eventFilter(QObject *, QEvent * e)
{
    switch (e->type())
    {
    case QEvent::Leave:
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::Close:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        deleteLater();
        break;

    default:
        break;
    }

    return false;
}

void PopupWidget::showEvent(QShowEvent *)
{
    auto pos = QCursor::pos();
    auto geom = QApplication::primaryScreen()->geometry();

    /* find the screen the cursor is on */
    if (!geom.contains(pos))
    {
        for (auto screen : QApplication::screens())
        {
            auto geom2 = screen->geometry();
            if (geom2.contains(pos))
            {
                geom = geom2;
                break;
            }
        }
    }

    int x = pos.x();
    int y = pos.y();
    int w = width();
    int h = height();

    /* If we show the popup right under the cursor, the underlying window gets
     * a leaveEvent and immediately hides the popup again.  So, we offset the
     * popup slightly. */
    if (x + w > geom.x() + geom.width())
        x -= w + 3;
    else
        x += 3;

    if (y + h > geom.y() + geom.height())
        y -= h + 3;
    else
        y += 3;

    move(x, y);
    adjustSize();
}

void show_copy_context_menu(QWidget * parent, const QPoint & global_pos,
                            const QString & text_to_copy)
{
    auto menu = new QMenu(parent);
    auto action = new QAction(QIcon::fromTheme("edit-copy"), _("Copy"), menu);

    QObject::connect(action, &QAction::triggered, action, [text_to_copy]() {
        auto data = new QMimeData;
        data->setText(text_to_copy);
        QApplication::clipboard()->setMimeData(data);
    });

    menu->addAction(action);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(global_pos);
}

} // namespace audqt
