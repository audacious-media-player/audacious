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

#include "log-inspector.h"
#include "libaudqt.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QTreeView>
#include <QWidget>

#include <libaudcore/i18n.h>
#include <libaudcore/index.h>
#include <libaudcore/runtime.h>

namespace audqt {

const int LOGENTRY_MAX = 1000;

enum LogEntryColumn {
    Level,
    File,
    Line,
    Function,
    Message,
    Count
};

struct LogEntry {
    audlog::Level level;
    const char * filename;
    unsigned int line;
    const char * function;
    char * message = nullptr;
    
    ~LogEntry ()
        { free (message); }
};

static Index<SmartPtr<LogEntry>> entries;

static void log_handler (audlog::Level level, const char * file, int line,
 const char * func, const char * message);

/* log entry model */
LogEntryModel::LogEntryModel (QObject * parent) : QAbstractListModel (parent)
{
}

LogEntryModel::~LogEntryModel ()
{
}

int LogEntryModel::rowCount (const QModelIndex & parent) const
{
    return entries.len ();
}

int LogEntryModel::columnCount (const QModelIndex & parent) const
{
    return LogEntryColumn::Count;
}

QVariant LogEntryModel::data (const QModelIndex & index, int role) const
{
    int row = index.row ();
    if (row < 0 || row >= entries.len ())
        return QVariant ();

    auto & e = entries[row];

    if (role == Qt::DisplayRole)
    {
        switch (index.column ())
        {
            case LogEntryColumn::Level: return QString (audlog::get_level_name (e->level));
            case LogEntryColumn::File: return QString (e->filename);
            case LogEntryColumn::Line: return e->line;
            case LogEntryColumn::Function: return QString (e->function);
            case LogEntryColumn::Message: return QString (e->message);
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
            case LogEntryColumn::File: return QString (_("Filename"));
            case LogEntryColumn::Line: return QString (_("Line"));
            case LogEntryColumn::Function: return QString (_("Function"));
            case LogEntryColumn::Message: return QString (_("Message"));
        }
    }

    return QVariant ();
}

void LogEntryModel::addEntry (LogEntry * entry)
{
    if (entries.len () >= LOGENTRY_MAX)
    {
        beginRemoveRows (QModelIndex (), 0, 0);
        entries.remove (0, 1);
        endRemoveRows ();
    }

    beginInsertRows (QModelIndex (), entries.len (), entries.len ());
    entries.append (SmartPtr<LogEntry> (entry));
    endInsertRows ();
}

/* log entry inspector */
class LogEntryInspector : public QDialog
{
public:
    LogEntryInspector (QWidget * parent = nullptr);
    ~LogEntryInspector ();

    audlog::Level m_level;

    void addEntry (LogEntry * entry)
        { m_model->addEntry (entry); }

private:
    QVBoxLayout m_layout;
    LogEntryModel * m_model;
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

    m_model = new LogEntryModel (this);
    m_view = new QTreeView (this);
    m_view->setModel (m_model);

    m_layout.addWidget (m_view);

    m_bottom_layout.setContentsMargins (0, 0, 0, 0);

    m_level_label.setText (_("Log Level:"));
    m_bottom_layout.addWidget (& m_level_label);

    m_level_combobox.addItem (_("Debug"), audlog::Debug);
    m_level_combobox.addItem (_("Info"), audlog::Info);
    m_level_combobox.addItem (_("Warning"), audlog::Warning);
    m_level_combobox.addItem (_("Error"), audlog::Error);

    QObject::connect (& m_level_combobox,
                      static_cast <void (QComboBox::*) (int)> (&QComboBox::currentIndexChanged),
                      [this] (int idx) { setLogLevel ((audlog::Level) idx); });

    m_bottom_layout.addWidget (& m_level_combobox);

    m_bottom_container.setLayout (& m_bottom_layout);
    m_layout.addWidget (& m_bottom_container);

    resize (800, 350);

    setLogLevel (audlog::Info);
}

LogEntryInspector::~LogEntryInspector ()
{
    audlog::unsubscribe (log_handler);
}

static LogEntryInspector * s_inspector = nullptr;

static void log_handler (audlog::Level level, const char * file, int line,
 const char * func, const char * message)
{
    LogEntry * l = new LogEntry;

    l->level = level;
    l->filename = file;
    l->line = line;
    l->function = func;

    l->message = strdup (message);
    l->message[strlen (l->message) - 1] = 0;

    s_inspector->addEntry (l);
}

void LogEntryInspector::setLogLevel (audlog::Level level)
{
    m_level = level;

    audlog::unsubscribe (log_handler);
    audlog::subscribe (log_handler, level);

    m_level_combobox.setCurrentIndex (m_level);
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
