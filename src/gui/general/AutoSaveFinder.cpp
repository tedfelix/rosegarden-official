/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AutoSaveFinder.h"
#include "ResourceFinder.h"

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
    QString dir = getAutoSaveDir();
    if (dir == "") {
        std::cerr << "WARNING: AutoSaveFinder::getAutoSavePath: No auto-save location located!?" << std::endl;
        return "";
    }
    
    // This is just a simple (in terms of code present here, and
    // trustworthiness) way of ensuring there are no unwanted
    // characters in the filename -- although there may be advantages
    // to the filename being readable, so we might want to consider
    // something more like the old way
    QString hashed = QString::fromLocal8Bit
        (QCryptographicHash::hash(filename.toLocal8Bit(),
                                  QCryptographicHash::Sha1).toHex());
    
    return dir + "/" + hashed;
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
