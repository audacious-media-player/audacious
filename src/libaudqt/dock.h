/*
 * dock.h
 * Copyright 2020 John Lindgren
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

#ifndef LIBAUDQT_DOCK_H
#define LIBAUDQT_DOCK_H

#include <QPointer>
#include <QWidget>

#include <libaudqt/export.h>

class PluginHandle;

namespace audqt
{

class LIBAUDQT_PUBLIC DockItem
{
public:
    DockItem(const char * id, const char * name, QWidget * widget);
    virtual ~DockItem();

    const char * id() const { return m_id; }
    const char * name() const { return m_name; }
    QWidget * widget() const { return m_widget; }

    void set_host_data(void * data) { m_host_data = data; }
    void * host_data() const { return m_host_data; }

    virtual void grab_focus();
    virtual void user_close() = 0;

    static DockItem * find_by_plugin(PluginHandle * plugin);

private:
    const char *m_id, *m_name;
    QPointer<QWidget> m_widget;
    void * m_host_data = nullptr;
};

class DockHost
{
public:
    virtual void add_dock_item(DockItem * item) = 0;
    virtual void focus_dock_item(DockItem * item) = 0;
    virtual void remove_dock_item(DockItem * item) = 0;
};

void register_dock_host(DockHost * host);
void unregister_dock_host();

} // namespace audqt

#endif // LIBAUDQT_DOCK_H
