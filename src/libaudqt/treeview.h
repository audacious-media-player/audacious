/*
 * treeview.h
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

#ifndef LIBAUDQT_TREEVIEW_H
#define LIBAUDQT_TREEVIEW_H

#include <QTreeView>
#include <libaudqt/export.h>

namespace audqt
{

// This class extends QTreeView and adds:
//  - A method to remove all selected rows (Delete key)
//  - Some useful QStyle overrides
class LIBAUDQT_PUBLIC TreeView : public QTreeView
{
public:
    TreeView(QWidget * parent = nullptr);
    ~TreeView() override;

    void removeSelectedRows();

protected:
    void keyPressEvent(QKeyEvent * event) override;
};

} // namespace audqt

#endif // LIBAUDQT_TREEVIEW_H
