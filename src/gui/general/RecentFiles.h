/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file from Sonic Visualiser, copyright 2006 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_RECENT_FILES_H
#define RG_RECENT_FILES_H

#include <QObject>
#include <QString>

#include <deque>


namespace Rosegarden
{


/// Manages a list of recently used files.
/**
 * The list is saved and restored via QSettings.  The names do not
 * actually have to refer to files.
 */
class RecentFiles : public QObject
{
    Q_OBJECT

public:
    RecentFiles();

    /// Add a file name to the list of recent files.
    void add(QString name);

    /// Get the list of recent file names.
    const std::deque<QString> &get() const  { return m_names; }

signals:
    void changed();

private:
    std::deque<QString> m_names;

    void read();
    void write();
};

}

#endif
