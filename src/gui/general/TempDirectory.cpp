/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2006 Chris
    Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TempDirectory]"
#define RG_NO_DEBUG_PRINT

#include "TempDirectory.h"

#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "misc/Strings.h"

#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QSettings>

#include <iostream>

#include <signal.h>
#include <unistd.h>


namespace Rosegarden
{


DirectoryCreationFailed::DirectoryCreationFailed(QString directory) throw() :
    m_directory(directory)
{
    RG_WARNING << "ERROR: Directory creation failed for directory: " << directory;
}

const char *
DirectoryCreationFailed::what() const throw()
{
    return QString("Directory creation failed for \"%1\"")
        .arg(m_directory).toLocal8Bit().data();
}

DirectoryCreationFailed::~DirectoryCreationFailed() throw() { }

TempDirectory *
TempDirectory::m_instance = new TempDirectory;

TempDirectory *
TempDirectory::getInstance()
{
    return m_instance;
}

TempDirectory::TempDirectory() :
    m_tmpdir("")
{
}

TempDirectory::~TempDirectory()
{
    RG_DEBUG << "dtor";

    cleanup();
}

void
TempDirectory::cleanup()
{
    cleanupDirectory("");
}

QString
TempDirectory::getPath()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_tmpdir != "") return m_tmpdir;

    QSettings settings;
    settings.beginGroup(TempDirectoryConfigGroup);
    QString rgDirParent = settings.value("create-in", "$HOME").toString();
    settings.endGroup();

    rgDirParent.replace("$HOME", QDir::home().absolutePath());

    QString rgDirBase = ".rosegarden.tmp";
    QString rgDir = QDir(rgDirParent).filePath(rgDirBase);
    if (!QFileInfo(rgDir).exists()) {
        if (!QDir(rgDirParent).mkdir(rgDirBase)) {
            throw DirectoryCreationFailed(QString("%1 directory in %2")
                                          .arg(rgDirBase).arg(rgDirParent));
        }
    } else if (!QFileInfo(rgDir).isDir()) {
        throw DirectoryCreationFailed(QString("%1/%2 is not a directory")
                                      .arg(rgDirParent).arg(rgDirBase));
    }

    cleanupAbandonedDirectories(rgDir);

    return createTempDirectoryIn(rgDir);
}

QString
TempDirectory::createTempDirectoryIn(QString dir)
{
    // Entered with mutex held.

    QDir tempDirBase(dir);

    // Generate a temporary directory.  Qt4.1 doesn't seem to be able
    // to do this for us, and mkdtemp is not standard.  This method is
    // based on the way glibc does mkdtemp.

    // NOTE: Qt 5 has QTemporaryDir for this

    static QString chars =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    QString suffix;
    int padlen = 6, attempts = 100;
    unsigned int r = time(nullptr) ^ getpid();

    for (int i = 0; i < padlen; ++i) {
        suffix += "X";
    }
    
    for (int j = 0; j < attempts; ++j) {

        unsigned int v = r;
        
        for (int i = 0; i < padlen; ++i) {
            suffix[i] = chars[v % 62];
            v /= 62;
        }

        QString candidate = QString("rg_%1").arg(suffix);

        if (tempDirBase.mkpath(candidate)) {
            m_tmpdir = tempDirBase.filePath(candidate);
            break;
        }

        r = r + 7777;
    }

    if (m_tmpdir == "") {
        throw DirectoryCreationFailed(QString("temporary subdirectory in %1")
                                      .arg(tempDirBase.canonicalPath()));
    }

    QString pidpath = QDir(m_tmpdir).filePath(QString("%1.pid").arg(getpid()));
    QFile pidfile(pidpath);

    if (!pidfile.open(QIODevice::WriteOnly)) {
        throw DirectoryCreationFailed(QString("pid file creation in %1")
                                      .arg(m_tmpdir));
    } else {
        pidfile.close();
    }

    return m_tmpdir;
}

QString
TempDirectory::getSubDirectoryPath(QString subdir)
{
    QString tmpdirpath = getPath();
    
    QMutexLocker locker(&m_mutex);

    QDir tmpdir(tmpdirpath);
    QFileInfo fi(tmpdir.filePath(subdir));

    if (!fi.exists()) {
        if (!tmpdir.mkdir(subdir)) {
            throw DirectoryCreationFailed(fi.filePath());
        } else {
            return fi.filePath();
        }
    } else if (fi.isDir()) {
        return fi.filePath();
    } else {
        throw DirectoryCreationFailed(fi.filePath());
    }
}

void
TempDirectory::cleanupDirectory(QString tmpdir)
{
    bool isRoot = false;

    if (tmpdir == "") {

        m_mutex.lock();

        isRoot = true;
        tmpdir = m_tmpdir;

        if (tmpdir == "") {
            m_mutex.unlock();
            return;
        }
    }

    QDir dir(tmpdir);
    dir.setFilter(QDir::Dirs | QDir::Files);

    for (unsigned int i = 0; i < dir.count(); ++i) {

        if (dir[i] == "." || dir[i] == "..") continue;
        QFileInfo fi(dir.filePath(dir[i]));

        if (fi.isDir()) {
            cleanupDirectory(fi.absoluteFilePath());
        } else {
            if (!QFile(fi.absoluteFilePath()).remove()) {
                RG_WARNING << "cleanupDirectory(): WARNING: Failed to unlink file" << fi.absoluteFilePath();
            }
        }
    }

    QString dirname = dir.dirName();
    if (dirname != "") {
        if (!dir.cdUp()) {
            RG_WARNING << "cleanupDirectory(): WARNING: Failed to cd to parent directory of" << tmpdir;
            return;
        }
        if (!dir.rmdir(dirname)) {
            RG_WARNING << "cleanupDirectory(): WARNING: Failed to remove directory" << dirname;
        } 
    }

    if (isRoot) {
        m_tmpdir = "";
        m_mutex.unlock();
    }
}

void
TempDirectory::cleanupAbandonedDirectories(QString rgDir)
{
    QDir dir(rgDir, "rg_*", QDir::Name, QDir::Dirs);

    for (unsigned int i = 0; i < dir.count(); ++i) {
        
        QDir subdir(dir.filePath(dir[i]), "*.pid", QDir::Name, QDir::Files);

        for (unsigned int j = 0; j < subdir.count(); ++j) {

            bool ok = false;
            int pid = QFileInfo(subdir[j]).baseName().toInt(&ok);
            if (!ok) continue;

            if (kill(getpid(), 0) == 0 && kill(pid, 0) != 0) {
                RG_DEBUG << "cleanupAbandonedDirectories(): INFO: Found abandoned temporary directory from a previous, defunct process\n(pid=" << pid << ", directory=" << dir.filePath(dir[i]) << ").  Removing it...";
                cleanupDirectory(dir.filePath(dir[i]));
                RG_DEBUG << "...done.";
                break;
            }
        }
    }
}

}


       
