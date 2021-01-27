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

#define RG_MODULE_STRING "[RecentFiles]"

#include "RecentFiles.h"

#include "misc/Debug.h"

#include <QSettings>

#include <algorithm>


namespace
{
    const QString settingsGroup("RecentFiles");
    constexpr size_t maxCount = 20;
}


namespace Rosegarden
{


RecentFiles::RecentFiles()
{
    read();
}

void
RecentFiles::read()
{
    m_names.clear();

    QSettings settings;
    settings.beginGroup(settingsGroup);

    // ??? Why clear up to 100?  I think we can just do up to maxCount.
    for (size_t i = 0; i < 100; ++i) {
        QString key = QString("recent-%1").arg(i);
        QString name = settings.value(key, "").toString();
        if (name == "")
            break;
        if (i < maxCount) {
            m_names.push_back(name);
        } else {
            // Clear out any beyond maxCount
            // ??? We could also use remove() to remove these.
            settings.setValue(key, "");
        }
    }
}

void
RecentFiles::write()
{
    QSettings settings;
    settings.beginGroup(settingsGroup);

    for (size_t i = 0; i < maxCount; ++i) {
        QString key = QString("recent-%1").arg(i);
        QString name = "";
        if (i < m_names.size())
            name = m_names[i];
        // ??? We could also use remove() to remove those that do not
        //     exist.
        settings.setValue(key, name);
    }
}

void
RecentFiles::truncateAndWrite()
{
    // ??? Inline this routine into its only caller.

    // Truncate m_names to maxCount.
    while (m_names.size() > maxCount) {
        m_names.pop_back();
    }

    write();
}

void
RecentFiles::add(QString name)
{
    // Remove it if it's already in there.
    std::deque<QString>::iterator iter =
            std::find(m_names.begin(), m_names.end(), name);
    if (iter != m_names.end())
        m_names.erase(iter);

    // Add it to the top.
    m_names.push_front(name);

    truncateAndWrite();

    emit recentChanged();
}


}
