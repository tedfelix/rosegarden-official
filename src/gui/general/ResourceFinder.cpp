/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ResourceFinder.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <cstdlib>

namespace Rosegarden
{

/**
   Resource files may be found in three places:

   * Bundled into the application as Qt4 resources.  These may be
     opened using Qt classes such as QFile, with "fake" file paths
     starting with a colon.  For example ":ui/rosegardenui.rc".

   * Installed with the Rosegarden package, or separately by a
     system administrator.  These files are found beneath some root
     directory at a system-dependent location, which on Linux is
     typically /usr/share/rosegarden or /usr/local/share/rosegarden.
     The first part of this (/usr/share/rosegarden) may be specified
     in the ROSEGARDEN environment variable; if this is not set, a
     typical path (/usr/local, /usr) will be searched.

   * Installed by the user in their home directory.  On Linux the
     locations of these files are identical to the system-wide
     ones but with $HOME/.local in place of /usr/local or /usr.

   These locations are searched in reverse order (user-installed
   copies take priority over system-installed copies take priority
   over bundled copies).  Also, /usr/local takes priority over /usr.
*/

QStringList
ResourceFinder::getSystemResourcePrefixList()
{
    // returned in order of priority

    QStringList list;

    static const char *prefixes[] = { "/usr/local/share", "/usr/share" };
    static const char *appname = "rosegarden";
    char *rosegarden = getenv("ROSEGARDEN");

    if (rosegarden) {
        list << rosegarden;
    } else {
        for (unsigned int i = 0; i < sizeof(prefixes)/sizeof(prefixes[0]); ++i) {
            list << QString("%1/%2").arg(prefixes[i]).arg(appname);
        }
    }

    return list;
}

QString
ResourceFinder::getUserResourcePrefix()
{
    static const char *homepath = ".local/share";
    static const char *appname = "rosegarden";
    char *home = getenv("HOME");

    if (home) {
        return QString("%1/%2/%3").arg(home).arg(homepath).arg(appname);
    } else {
        return "";
    }
}    

QStringList
ResourceFinder::getResourcePrefixList()
{
    // returned in order of priority

    QStringList list;

    QString user = getUserResourcePrefix();
    if (user != "") list << user;

    list << getSystemResourcePrefixList();

    list << ":"; // bundled resource location

    return list;
}

QString
ResourceFinder::getResourcePath(QString resourceCat, QString fileName)
{
    // We don't simply call getResourceDir here, because that returns
    // only the "installed file" location.  We also want to search the
    // bundled resources and user-saved files.

    QStringList prefixes = getResourcePrefixList();
    
    if (resourceCat != "") resourceCat = "/" + resourceCat;

    for (QStringList::const_iterator i = prefixes.begin();
         i != prefixes.end(); ++i) {
        
        QString prefix = *i;
        QString path =
            QString("%1%2/%3").arg(prefix).arg(resourceCat).arg(fileName);
        if (QFileInfo(path).exists() && QFileInfo(path).isReadable()) {
            return path;
        }
    }

    return "";
}

QString
ResourceFinder::getResourceDir(QString resourceCat)
{
    // Returns only the "installed file" location

    QStringList prefixes = getSystemResourcePrefixList();
    
    if (resourceCat != "") resourceCat = "/" + resourceCat;

    for (QStringList::const_iterator i = prefixes.begin();
         i != prefixes.end(); ++i) {
        
        QString prefix = *i;
        QString path = QString("%1%2").arg(prefix).arg(resourceCat);
        if (QFileInfo(path).exists() &&
            QFileInfo(path).isDir() &&
            QFileInfo(path).isReadable()) {
            return path;
        }
    }

    return "";
}

QString
ResourceFinder::getResourceSavePath(QString resourceCat, QString fileName)
{
    QString dir = getResourceSaveDir(resourceCat);
    if (dir == "") return "";

    return dir + "/" + fileName;
}

QString
ResourceFinder::getResourceSaveDir(QString resourceCat)
{
    // Returns the "user" location

    QString user = getUserResourcePrefix();
    if (user == "") return "";

    if (resourceCat != "") resourceCat = "/" + resourceCat;

    QDir userDir(user);
    if (!userDir.exists()) userDir.mkpath(user);

    if (resourceCat != "") {
        QString save = QString("%1/%2").arg(user).arg(resourceCat);
        QDir saveDir(save);
        if (!saveDir.exists()) userDir.mkpath(resourceCat);
        return save;
    } else {
        return user;
    }
}

QStringList
ResourceFinder::getResourceFiles(QString resourceCat, QString fileExt)
{
    QStringList results;
    QStringList prefixes = getResourcePrefixList();

    QStringList filters;
    filters << QString("*.%1").arg(fileExt);

    for (QStringList::const_iterator i = prefixes.begin();
         i != prefixes.end(); ++i) {
        
        QString prefix = *i;
        QString path;

        if (resourceCat != "") {
            path = QString("%1/%2").arg(prefix).arg(resourceCat);
        } else {
            path = prefix;
        }
        
        QDir dir(path);
        if (!dir.exists()) continue;

        dir.setNameFilters(filters);
        QStringList entries = dir.entryList
            (QDir::Files | QDir::Readable, QDir::Name);
        
        for (QStringList::const_iterator j = entries.begin();
             j != entries.end(); ++j) {
            results << QString("%1/%2").arg(path).arg(*j);
        }
    }

    return results;
}

}

