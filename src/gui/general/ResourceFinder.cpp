/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ResourceFinder]"

#include "ResourceFinder.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QProcess>

#include "misc/Debug.h"
#include "misc/Strings.h"

#include <cstdlib>

namespace Rosegarden
{

/**
   Resource files may be found in three places:

   * Bundled into the application as Qt4 resources.  These may be
     opened using Qt classes such as QFile, with "fake" file paths
     starting with a colon.  For example ":rc/rosegardenui.rc".

   * Installed with the Rosegarden package, or separately by a
     system administrator.  These files are found beneath some root
     directory at a system-dependent location, which on Linux is
     typically /usr/share/rosegarden or /usr/local/share/rosegarden.
     The first part of this (/usr/share/rosegarden) may be specified
     in the ROSEGARDEN environment variable; if this is not set, a
     typical path (/usr/local, /usr) will be searched.  (Note that
     now that we have a proper install target, we're installing any
     such files to $PREFIX/share/rosegarden-$VERSION now, with the
     aim of making it easier for users to keep a stable usable copy
     around and play with an experimental one at the same time; in
     order to encourage pre-release testing.)

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

    // Qt5: use QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)

    QStringList list;

    static const char *prefixes[] = { "/usr/local/share", "/usr/share" };
    static const char *appname = "rosegarden";
    char *rosegarden = getenv("ROSEGARDEN");

    if (rosegarden) {
        list << rosegarden;
    } else {
        for (size_t i = 0; i < sizeof(prefixes)/sizeof(prefixes[0]); ++i) {
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
    const QString home = QDir::homePath();

    // Qt5: use QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/rosegarden";

    if (!home.isEmpty()) {
        return home + '/' + QString(homepath) + '/' + QString(appname);
    } else {
        RG_WARNING << "ResourceFinder::getUserResourcePrefix: ERROR: No home directory available?";
        return QString();
    }
}

QStringList
ResourceFinder::getResourcePrefixList()
{
    // returned in order of priority

    QStringList list;

    QString user = getUserResourcePrefix();
    if (!user.isEmpty()) list << user;

    list << getSystemResourcePrefixList();

    list << ":"; // bundled resource location

    return list;
}

QString
ResourceFinder::getResourcePath(QString resourceCat, const QString &fileName)
{
    // We don't simply call getResourceDir here, because that returns
    // only the "installed file" location.  We also want to search the
    // bundled resources and user-saved files.

    const QStringList prefixes = getResourcePrefixList();
    
    if (!resourceCat.isEmpty()) resourceCat.prepend('/');

    foreach (const QString &prefix, prefixes) {
        //RG_DEBUG << "ResourceFinder::getResourcePath: Looking up file \"" << fileName << "\" for category \"" << resourceCat << "\" in prefix \"" << prefix << "\"";

        const QString path = prefix + resourceCat + '/' + fileName;
        if (QFileInfo(path).isReadable()) {
            //RG_DEBUG << "Found it!";
            return path;
        }
    }

    RG_WARNING << "getResourcePath(): Resource file \"" << fileName << "\" for category \"" << resourceCat << "\" not found.";

    return QString();
}

QString
ResourceFinder::getResourceDir(QString resourceCat)
{
    // Returns only the "installed file" location

    const QStringList prefixes = getSystemResourcePrefixList();
    
    if (!resourceCat.isEmpty()) resourceCat.prepend('/');

    foreach (const QString &prefix, prefixes) {
        const QString path = prefix + resourceCat;
        const QFileInfo fileInfo(path);
        if (fileInfo.isDir() &&
            fileInfo.isReadable()) {
            return path;
        }
    }

    return QString();
}

QString
ResourceFinder::getResourceSavePath(QString resourceCat, QString fileName)
{
    const QString dir = getResourceSaveDir(resourceCat);
    if (dir.isEmpty()) return QString();

    return dir + '/' + fileName;
}

QString
ResourceFinder::getResourceSaveDir(QString resourceCat)
{
    // Returns the "user" location

    const QString user = getUserResourcePrefix();
    if (user.isEmpty()) return QString();

    if (!resourceCat.isEmpty()) resourceCat.prepend('/');

    QDir userDir(user);
    if (!userDir.exists()) {
        if (!userDir.mkpath(user)) {
            RG_WARNING << "ResourceFinder::getResourceSaveDir: ERROR: Failed to create user resource path \"" << user << "\"";
            return QString();
        }
    }

    if (resourceCat != "") {
        QString save = QString("%1%2").arg(user).arg(resourceCat);
        QDir saveDir(save);
        if (!saveDir.exists()) {
            if (!userDir.mkpath(save)) {
                RG_WARNING << "ResourceFinder::getResourceSaveDir: ERROR: Failed to create user resource path \"" << save << "\"";
                return QString();
            }
        }
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
    filters << QString("*.") + fileExt;

    foreach (const QString &prefix, prefixes) {
        QString path;
        if (!resourceCat.isEmpty()) {
            path = prefix + '/' + resourceCat;
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

QString
ResourceFinder::getAutoloadPath()
{
    if (!unbundleResource("autoload", "autoload.rg")) return "";
    return getResourcePath("autoload", "autoload.rg");
}

QString
ResourceFinder::getAutoloadSavePath()
{
    return getResourceSavePath("autoload", "autoload.rg");
}

bool
ResourceFinder::unbundleResource(QString resourceCat, QString fileName)
{
    QString path = getResourcePath(resourceCat, fileName);
    
    if (!path.startsWith(':')) return true;

    // This is the lowest-priority alternative path for this
    // resource, so we know that there must be no installed copy.
    // Install one to the user location.
    RG_DEBUG << "ResourceFinder::unbundleResource: File " << fileName << " is bundled, un-bundling it";
    QString target = getResourceSavePath(resourceCat, fileName);
    QFile file(path);
    if (!file.copy(target)) {
        RG_WARNING << "ResourceFinder::unbundleResource: ERROR: Failed to un-bundle resource file \"" << fileName << "\" to user location \"" << target << "\"";
        return false;
    }

    // Now since the file is in the user's editable space, the user should get
    // to edit it.  The chords.xml file I unbundled came out 444 instead of 644
    // which won't do.  Rather than put the chmod code there, I decided to put
    // it here, because I think it will always be appropriate to make unbundled
    // files editable.  That's rather the point in many cases, and for the rest,
    // nobody will likely notice they could have edited their font files or what
    // have you that were unbundled to improve performance.  (Dissenting
    // opinions welcome.  We can always shuffle this somewhere else if
    // necessary.  There are many possibilities.)
    QFile chmod(target);
    chmod.setPermissions(QFile::ReadOwner |
                         QFile::ReadUser  | /* for potential platform-independence */
                         QFile::ReadGroup |
                         QFile::ReadOther |
                         QFile::WriteOwner|
                         QFile::WriteUser); /* for potential platform-independence */

    return true;
}


}

