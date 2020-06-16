/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ICON_LOADER_H
#define RG_ICON_LOADER_H

#include <QIcon>
#include <QPixmap>
#include <QString>

#include <map>
#include <rosegardenprivate_export.h>

namespace Rosegarden {

class ROSEGARDENPRIVATE_EXPORT IconLoader
{
public:
    QIcon load(QString name);
    QPixmap loadPixmap(QString name);

    // process the given pixmap so as to try to make it visible if the
    // background is very dark
    static QPixmap invert(QPixmap);

protected:
    static QPixmap loadPixmap(QString dir, QString name);
    std::map<QString, QPixmap> m_cache;
};

}

#endif

	
