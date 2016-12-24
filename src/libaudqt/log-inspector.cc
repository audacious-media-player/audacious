/*
 * log-inspector.cc
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

#include "libaudqt.h"
#include "libaudqt-internal.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QTreeView>
#include <QWidget>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/index.h>
#include <libaudcore/runtime.h>

#define LOGENTRY_MAX 1000

namespace audqt {

enum LogEntryColumn {
    Level,
    Function,
    Message,
    Count
};

struct LogEntry {
    audlog::Level level;
    String function;
    String message;
};

class LogEntryModel : public QAbstractListModel
{
public:
    LogEntryModel (QObject * parent = nullptr) :
        QAbstractListModel (parent) {}

protected:
    int rowCount (const QModelIndex & parent = QModelIndex ()) const
        { return m_entries.len (); }
    int columnCount (const QModelIndex & parent = QModelIndex ()) const
        { return LogEntryColumn::Count; }

    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    Index<LogEntry> m_entries;

    void addEntry (const LogEntry * entry);
    HookReceiver<LogEntryModel, const LogEntry *>
     log_hook {"audqt log entry", this, & LogEntryModel::addEntry};
};

/* log entry model */
void LogEntryModel::addEntry (const LogEntry * entry)
{
    if (m_entries.len () >= LOGENTRY_MAX)
    {
        beginRemoveRows (QModelIndex (), 0, 0);
        m_entries.remove (0, 1);
        endRemoveRows ();
    }

    beginInsertRows (QModelIndex (), m_entries.len (), m_entries.len ());
    m_entries.append (* entry);
    endInsertRows ();
}

QVariant LogEntryModel::data (const QModelIndex & index, int role) const
{
    int row = index.row ();
    if (row < 0 || row >= m_entries.len ())
        return QVariant ();

    auto & e = m_entries[row];

    if (role == Qt::DisplayRole)
    {
        switch (index.column ())
        {
            case LogEntryColumn::Level: return QString (audlog::get_level_name (e.level));
            case LogEntryColumn::Function: return QString (e.function);
            case LogEntryColumn::Message: return QString (e.message);
        }
    }

    return QVariant ();
}

QVariant LogEntryModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case LogEntryColumn::Level: return QString (_("Level"));
            case LogEntryColumn::Function: return QString (_("Function"));
            case LogEntryColumn::Message: return QString (_("Message"));
        }
    }

    return QVariant ();
}

/* static model */
static SmartPtr<LogEntryModel> s_model;
static audlog::Level s_level = audlog::Warning;

static void log_handler (audlog::Level level, const char * file, int line,
 const char * func, const char * message)
{
    auto messages = str_list_to_index (message, "\n");

    for (auto & message : messages)
    {
        auto entry = new LogEntry;

        entry->level = level;
        entry->function = String (str_printf ("%s (%s:%d)", func, file, line));
        entry->message = std::move (message);

        event_queue ("audqt log entry", entry, aud::delete_obj<LogEntry>);
    }
}

void log_init ()
{
    s_model.capture (new LogEntryModel);
    audlog::subscribe (log_handler, s_level);
}

void log_cleanup ()
{
    audlog::unsubscribe (log_handler);
    event_queue_cancel ("audqt log entry");
    s_model.clear ();
}

/* log entry inspector */
class LogEntryInspector : public QDialog
{
public:
    LogEntryInspector (QWidget * parent = nullptr);

private:
    QVBoxLayout m_layout;
    QTreeView * m_view;

    QWidget m_bottom_container;
    QHBoxLayout m_bottom_layout;

    QComboBox m_level_combobox;
    QLabel m_level_label;

    void setLogLevel (audlog::Level level);
};

LogEntryInspector::LogEntryInspector (QWidget * parent) :
    QDialog (parent)
{
    setWindowTitle (_("Log Inspector"));
    setLayout (& m_layout);

    m_view = new QTreeView (this);
    m_view->setModel (s_model.get ());

    m_view->setAllColumnsShowFocus (true);
    m_view->setIndentation (0);
    m_view->setUniformRowHeights (true);

    m_layout.addWidget (m_view);

    m_bottom_layout.setContentsMargins (0, 0, 0, 0);

    m_level_label.setText (_("Log Level:"));
    m_bottom_layout.addWidget (& m_level_label);

    m_level_combobox.addItem (_("Debug"), audlog::Debug);
    m_level_combobox.addItem (_("Info"), audlog::Info);
    m_level_combobox.addItem (_("Warning"), audlog::Warning);
    m_level_combobox.addItem (_("Error"), audlog::Error);

    m_level_combobox.setCurrentIndex (s_level);

    QObject::connect (& m_level_combobox,
                      static_cast <void (QComboBox::*) (int)> (&QComboBox::currentIndexChanged),
                      [this] (int idx) { setLogLevel ((audlog::Level) idx); });

    m_bottom_layout.addWidget (& m_level_combobox);

    m_bottom_container.setLayout (& m_bottom_layout);
    m_layout.addWidget (& m_bottom_container);

    resize (800, 350);
}

static LogEntryInspector * s_inspector = nullptr;

void LogEntryInspector::setLogLevel (audlog::Level level)
{
    s_level = level;

    audlog::unsubscribe (log_handler);
    audlog::subscribe (log_handler, level);

    m_level_combobox.setCurrentIndex (level);
}

EXPORT void log_inspector_show ()
{
    if (! s_inspector)
    {
        s_inspector = new LogEntryInspector;
        s_inspector->setAttribute (Qt::WA_DeleteOnClose);

        QObject::connect (s_inspector, & QObject::destroyed, [] () {
            s_inspector = nullptr;
        });
    }

    window_bring_to_front (s_inspector);
}

EXPORT void log_inspector_hide ()
{
    delete s_inspector;
}

} // namespace audqt
