/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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
#include <vector>
#include <deque>

namespace Rosegarden
{

/**
 * RecentFiles manages a list of the names of recently-used objects,
 * saving and restoring that list via QSettings.  The names do not
 * actually have to refer to files.
 */

class RecentFiles : public QObject
{
    Q_OBJECT

public:
    /**
     * Construct a RecentFiles object that saves and restores in the
     * given QSettings group and truncates when the given count of
     * strings is reached.
     */
    RecentFiles(QString settingsGroup = "RecentFiles", size_t maxCount = 10);

    virtual ~RecentFiles();

    QString getSettingsGroup() const { return m_settingsGroup; }

    int getMaxCount() const { return m_maxCount; }

    std::vector<QString> getRecent() const;

    /**
     * Add a name that should be treated as a literal string.
     */
    void add(QString name);
    
    /**
     * Add a name that is known to be either a file path or a URL.  If
     * it looks like a URL, add it literally; otherwise treat it as a
     * file path and canonicalise it appropriately.  Also takes into
     * account the user preference for whether to include temporary
     * files in the recent files menu: the file will not be added if
     * the preference is set and the file appears to be a temporary
     * one.
     */
    void addFile(QString name);

signals:
    void recentChanged();

protected:
    QString m_settingsGroup;
    size_t m_maxCount;

    std::deque<QString> m_names;

    void read();
    void write();
    void truncateAndWrite();
};

}

#endif
