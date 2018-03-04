/*
 * info-widget.h
 * Copyright 2006-2014 William Pitcock, Tomasz Moń, Eugene Zagidullin,
 *                     John Lindgren, and Thomas Lange
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

#ifndef LIBAUDQT_INFO_WIDGET_H
#define LIBAUDQT_INFO_WIDGET_H

#include <QTreeView>
#include <libaudqt/export.h>

class PluginHandle;
class Tuple;

namespace audqt {

class InfoModel;

class LIBAUDQT_PUBLIC InfoWidget : public QTreeView
{
public:
    InfoWidget (QWidget * parent = nullptr);
    ~InfoWidget ();

    void fillInfo (const char * filename, const Tuple & tuple,
     PluginHandle * decoder, bool updating_enabled);
    bool updateFile ();

private:
    InfoModel * m_model;
};

} // namespace audqt

#endif // LIBAUDQT_INFO_WIDGET_H
