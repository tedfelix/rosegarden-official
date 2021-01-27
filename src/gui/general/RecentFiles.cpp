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

#include <QFileInfo>
#include <QSettings>
#include <QRegExp>


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
        if (i < maxCount)
            m_names.push_back(name);
        else  // Clear out any beyond maxCount
            settings.setValue(key, "");
    }
}

void
RecentFiles::write()
{
    //RG_DEBUG << "write()...";

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
    while (m_names.size() > maxCount) {
        m_names.pop_back();
    }
    write();
}

#if 0
std::vector<QString>
RecentFiles::getRecent() const
{
    std::vector<QString> names;
    for (size_t i = 0; i < maxCount; ++i) {
        if (i < m_names.size()) {
            names.push_back(m_names[i]);
        }
    }
    return names;
}
#endif

void
RecentFiles::add(QString name)
{
    bool have = false;
    for (size_t i = 0; i < m_names.size(); ++i) {
        if (m_names[i] == name) {
            have = true;
            break;
        }
    }
    
    if (!have) {
        m_names.push_front(name);
    } else {
        std::deque<QString> newnames;
        newnames.push_back(name);
        for (size_t i = 0; i < m_names.size(); ++i) {
            if (m_names[i] == name) continue;
            newnames.push_back(m_names[i]);
        }
        m_names = newnames;
    }

    truncateAndWrite();
    emit recentChanged();
}

void
RecentFiles::addFile(QString name)
{
    //RG_DEBUG << "addFile(" << name << ")...";

    static QRegExp schemeRE("^[a-zA-Z]{2,5}://");
    static QRegExp tempRE("[\\/][Tt]e?mp[\\/]");
    if (schemeRE.indexIn(name) == 0) {
        add(name);
    } else {
        QString absPath = QFileInfo(name).absoluteFilePath();
        if (tempRE.indexIn(absPath) != -1) {
//            Preferences *prefs = Preferences::getInstance();
//            if (prefs && !prefs->getOmitTempsFromRecentFiles()) {
//                add(absPath);
//            }
        } else {
            add(absPath);
        }
    }
}


}
