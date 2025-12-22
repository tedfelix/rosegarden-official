/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AutoSaveFinder]"
#define RG_NO_DEBUG_PRINT

#include "AutoSaveFinder.h"

#include "ResourceFinder.h"
#include "misc/Debug.h"

#include <iostream>
#include <QCryptographicHash>
#include <QFileInfo>

namespace Rosegarden
{

QString
AutoSaveFinder::getAutoSaveDir()
{
    // This will (try to) create the directory if it doesn't exist
    return ResourceFinder().getResourceSaveDir("autosave");
}

QString
AutoSaveFinder::getAutoSavePath(QString filename)
{
    RG_DEBUG << "getAutoSavePath(): " << filename;

    const QString autoSaveDir = getAutoSaveDir();
    if (autoSaveDir == "") {
        RG_WARNING << "WARNING: getAutoSavePath(): No auto-save location located!?";
        return "";
    }

    // Use a hash to make sure the filename has no slashes.
    // The incoming filename has the complete path of the file.  So we need
    // to get rid of the slashes so we can save it here.  This is important
    // if you have multiple files with the same name in different directories.
    QString hashed;
    if (filename.isEmpty()) { // unsaved file
        hashed = "Untitled";
    } else {
        hashed = QString::fromLocal8Bit(QCryptographicHash::hash
                                        (filename.toLocal8Bit(),
                                         QCryptographicHash::Sha1).toHex());
    }

    return autoSaveDir + "/" + hashed;
}

QString
AutoSaveFinder::checkAutoSaveFile(QString filename)
{
    QString path = getAutoSavePath(filename);
    if (path == "") return "";

    if (QFileInfo(path).exists() && QFileInfo(path).size() > 0) return path;
    else return "";
}

}
