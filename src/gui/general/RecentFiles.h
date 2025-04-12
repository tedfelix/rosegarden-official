/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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

#include <list>


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

    /// Remove any files that don't actually exist on the filesystem.
    void removeNonExistent();

    /// Get the list of recent file names.
    const std::list<QString> &get() const  { return m_names; }

private:
    std::list<QString> m_names;

    void read();
    void write();
};

}

#endif
