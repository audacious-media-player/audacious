/*
 * infowin.cc
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

#include <QtGui>
#include <QtWidgets>
#include <QAbstractTableModel>

#include <libaudcore/audstrings.h>
#include <libaudcore/playlist.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>
#include <libaudcore/interface.h>
#include <libaudcore/tuple.h>
#include <libaudcore/hook.h>

#include <libaudqt/libaudqt.h>

namespace audqt {

struct TupleFieldMap {
    const char * name;
    int field;
    bool editable;
};
const TupleFieldMap tuple_field_map[] = {
    {N_("Metadata"),		-1,				false},
    {N_("Artist"),		FIELD_ARTIST,			true},
    {N_("Album"),		FIELD_ALBUM,			true},
    {N_("Title"),		FIELD_TITLE,			true},
    {N_("Track Number"),	FIELD_TRACK_NUMBER,		true},
    {N_("Genre"),		FIELD_GENRE,			true},
    {N_("Comment"),		FIELD_COMMENT,			true},
    {N_("Song Artist"),		FIELD_SONG_ARTIST,		true},
    {N_("Composer"),		FIELD_COMPOSER,			true},
    {N_("Performer"),		FIELD_PERFORMER,		true},
    {N_("Recording Year"),	FIELD_YEAR,			true},
    {N_("Recording Date"),	FIELD_DATE,			true},

    {"",			-1,				false},
    {N_("Technical"),		-1,				false},
    {N_("Length"),		FIELD_LENGTH,			false},
    {N_("MIME Type"),		FIELD_MIMETYPE,			false},
    {N_("Codec"),		FIELD_CODEC,			false},
    {N_("Quality"),		FIELD_QUALITY,			false},
    {N_("Bitrate"),		FIELD_BITRATE,			false},
};

class InfoModel : public QAbstractTableModel {
public:
    InfoModel (QObject * parent = nullptr);

    int rowCount (const QModelIndex & parent = QModelIndex()) const;
    int columnCount (const QModelIndex & parent = QModelIndex()) const;
    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData (const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags (const QModelIndex & index) const;

    void setTupleData (const Tuple & tuple, String filename, PluginHandle * plugin);

    Tuple m_tuple;
    String m_filename;
    PluginHandle * m_plugin;
};

class InfoWindow : public QDialog {
public:
    InfoWindow (QWidget * parent = nullptr);
    ~InfoWindow ();

    void fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
     PluginHandle * decoder, bool updating_enabled);
    void displayImage (const char * filename);

private:
    QHBoxLayout m_layout;
    QLabel m_image;
    QTreeView m_treeview;
    InfoModel m_model;
};

static InfoWindow * m_infowin = nullptr;

static void infowin_display_image_async (const char * filename, InfoWindow * infowin)
{
    infowin->displayImage (filename);
}

InfoWindow::InfoWindow (QWidget * parent) : QDialog (parent)
{
    m_layout.addWidget (& m_image);
    m_layout.addWidget (& m_treeview);
    setLayout (& m_layout);
    setWindowTitle (_("Track Information"));

    hook_associate ("art ready", (HookFunction) infowin_display_image_async, this);

    m_treeview.setModel (& m_model);
    m_treeview.header ()->hide ();
    m_treeview.setEditTriggers (QAbstractItemView::SelectedClicked);
    m_treeview.setIndentation (0);
}

InfoWindow::~InfoWindow ()
{
    hook_dissociate ("art ready", (HookFunction) infowin_display_image_async);
}

void InfoWindow::fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
 PluginHandle * decoder, bool updating_enabled)
{
    displayImage (filename);
    m_model.setTupleData (tuple, String (filename), decoder);
}

void InfoWindow::displayImage (const char * filename)
{
    QImage img = art_request (filename).scaled (256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_image.setPixmap (QPixmap::fromImage (img));
}

InfoModel::InfoModel (QObject * parent) : QAbstractTableModel (parent)
{
}

int InfoModel::rowCount (const QModelIndex & parent) const
{
    auto r = ArrayRef<TupleFieldMap> (tuple_field_map);
    return r.len;
}

int InfoModel::columnCount (const QModelIndex & parent) const
{
    return 2;
}

bool InfoModel::setData (const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole)
        return false;

    int field_id = tuple_field_map [index.row ()].field;
    if (field_id == -1)
        return false;

    auto t = Tuple::field_get_type (field_id);
    if (t == TUPLE_STRING)
    {
        m_tuple.set_str (field_id, value.toString ().toLocal8Bit ());
        emit dataChanged (index, index, {role});
        return true;
    }
    else if (t == TUPLE_INT)
    {
        m_tuple.set_int (field_id, value.toInt ());
        emit dataChanged (index, index, {role});
        return true;
    }

    return false;
}

QVariant InfoModel::data (const QModelIndex & index, int role) const
{
    int field_id = tuple_field_map [index.row ()].field;

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column () == 0)
            return translate_str (tuple_field_map [index.row ()].name);
        else if (index.column () == 1 && m_tuple)
        {
            if (field_id == -1)
                return QVariant ();

            auto t = Tuple::field_get_type (field_id);

            if (t == TUPLE_STRING)
            {
                const char * res = m_tuple.get_str (field_id);
                if (res)
                    return QString (res);
            }
            else if (t == TUPLE_INT)
            {
                int res = m_tuple.get_int (field_id);
                if (res == -1)
                    return QVariant ();
                return res;
            }
        }
    }
    else if (role == Qt::FontRole)
    {
        if (field_id == -1)
        {
            QFont f;
            f.setBold (true);
            return f;
        }
        return QVariant ();
    }

    return QVariant ();
}

Qt::ItemFlags InfoModel::flags (const QModelIndex & index) const
{
    if (index.column () == 1)
    {
        auto & t = tuple_field_map [index.row ()];

        if (t.field == -1)
            return Qt::ItemNeverHasChildren;
        else if (t.editable)
            return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;

        return Qt::ItemIsEnabled;
    }

    return Qt::ItemNeverHasChildren;
}

void InfoModel::setTupleData (const Tuple & tuple, String filename, PluginHandle * plugin)
{
    m_tuple = tuple.ref ();
    m_filename = filename;
    m_plugin = plugin;
}

EXPORT void infowin_show (int playlist, int entry)
{
    if (! m_infowin)
        m_infowin = new InfoWindow;

    String filename = aud_playlist_entry_get_filename (playlist, entry);
    if (! filename)
        return;

    PluginHandle * decoder = aud_playlist_entry_get_decoder (playlist, entry, false);
    if (! decoder)
        return;

    Tuple tuple = aud_playlist_entry_get_tuple (playlist, entry, false);
    if (tuple)
        m_infowin->fillInfo (playlist, entry, filename, tuple, decoder,
            aud_file_can_write_tuple (filename, decoder));
    else
        aud_ui_show_error (str_printf (_("No info available for %s.\n"),
            (const char *) filename));

    m_infowin->resize (700, 300);

    window_bring_to_front (m_infowin);
}

EXPORT void infowin_show_current ()
{
    int playlist = aud_playlist_get_playing ();
    int position;

    if (playlist == -1)
        playlist = aud_playlist_get_active ();

    position = aud_playlist_get_position (playlist);

    if (position == -1)
        return;

    infowin_show (playlist, position);
}

EXPORT void infowin_hide ()
{
    if (! m_infowin)
        return;

    m_infowin->hide ();
}

}
