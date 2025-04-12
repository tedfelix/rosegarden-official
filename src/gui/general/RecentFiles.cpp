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

#define RG_MODULE_STRING "[RecentFiles]"

#include "RecentFiles.h"

#include "misc/ConfigGroups.h"
#include "misc/Debug.h"

#include <QFileInfo>
#include <QSettings>

#include <algorithm>


namespace
{
    constexpr size_t maxCount = 20;
}


namespace Rosegarden
{


RecentFiles::RecentFiles()
{
    // ??? Only caller.  Inline?
    read();
}

void
RecentFiles::read()
{
    m_names.clear();

    QSettings settings;
    settings.beginGroup(RecentFilesConfigGroup);

    for (size_t i = 0; i < maxCount; ++i) {
        QString key = QString("recent-%1").arg(i);
        QString name = settings.value(key, "").toString();

        // Stop at the first empty one.
        if (name == "")
            break;

        m_names.push_back(name);
    }
}

void
RecentFiles::write()
{
    QSettings settings;
    settings.beginGroup(RecentFilesConfigGroup);

    size_t i = 0;

    // Write out the entries.
    for (const QString &name : m_names) {
        QString key = QString("recent-%1").arg(i++);
        settings.setValue(key, name);
    }

    // Remove any entries beyond the end.
    for (size_t j = i; j < maxCount; ++j) {
        QString key = QString("recent-%1").arg(j);
        settings.remove(key);
    }
}

void
RecentFiles::add(QString name)
{
    // Remove it if it's already in there.
    std::list<QString>::iterator iter =
            std::find(m_names.begin(), m_names.end(), name);
    if (iter != m_names.end())
        m_names.erase(iter);

    // Add it to the top.
    m_names.push_front(name);

    // Truncate m_names to maxCount.
    while (m_names.size() > maxCount) {
        m_names.pop_back();
    }

    write();
}

void
RecentFiles::removeNonExistent()
{
    for (std::list<QString>::iterator i = m_names.begin();
         i != m_names.end();
         /* Increment before use idiom. */) {

        // Increment before use.  So we can delete as we go.
        std::list<QString>::iterator j = i++;

        // If the file doesn't exist, remove it.
        if (!QFileInfo(*j).exists())
            m_names.erase(j);
    }

    // ??? Should we call write() now?  Otherwise we are out of sync.
    //     If we don't write it out, then the user can switch back
    //     to not removing non-existent, then restart rg and the
    //     entries that were removed will reappear.  Not sure that's
    //     valuable since it's pretty obscure.
    //
    //     If we don't write it out, then if a file reappears (e.g.
    //     a flash drive is re-inserted) and rg is restarted, the
    //     file will reappear in the recent files list.  It would
    //     be more helpful if rg didn't need to be restarted.
    //     E.g. if we removed the non-existent only from a temporary
    //     copy to be returned by get().
    //
    //     Let's avoid calling write() for now in the interests of
    //     simplicity and speed.

}


}
