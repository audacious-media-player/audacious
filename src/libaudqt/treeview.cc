/*
 * treeview.cc
 * Copyright 2018 John Lindgren
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

#include "treeview.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <libaudcore/index.h>

namespace audqt {

EXPORT TreeView::TreeView (QWidget * parent) :
    QTreeView (parent)
{
    // activate() is perhaps a bit redundant with activated()
    connect (this, & QTreeView::activated, this, & TreeView::activate);
}

EXPORT TreeView::~TreeView () {}

EXPORT void TreeView::keyPressEvent (QKeyEvent * event)
{
    auto CtrlShiftAlt = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier;
    if (event->key () == Qt::Key_Delete && ! (event->modifiers () & CtrlShiftAlt))
    {
        removeSelectedRows ();
        return;
    }

    QTreeView::keyPressEvent (event);
}

EXPORT void TreeView::removeSelectedRows ()
{
    // get all selected rows
    Index<int> rows;
    for (auto & idx : selectionModel ()->selectedRows ())
        rows.append (idx.row ());

    // sort in reverse order
    rows.sort ([] (const int & a, const int & b)
        { return b - a; });

    // remove rows in reverse order
    auto m = model ();
    for (int row : rows)
        m->removeRow (row);
}

// TODO: unnecessary, remove at next API break
EXPORT void TreeView::mouseDoubleClickEvent (QMouseEvent * event)
{
    QTreeView::mouseDoubleClickEvent (event);
}

EXPORT void TreeView::activate (const QModelIndex & index)
{
    (void) index;  // base implementation does nothing
}

} // namespace audqt
