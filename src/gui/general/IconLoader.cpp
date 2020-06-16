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

#define RG_MODULE_STRING "[IconLoader]"

#include "IconLoader.h"

#include <QColor>
#include <QImage>
#include <QRgb>

#include <map>

namespace {

    std::map<QString, QPixmap> pixmapCache;

    QPixmap
    loadPixmap2(QString dir, QString name)
    {
        // Try to load assuming extension is included.
        QPixmap pmap(QString("%1/%2").arg(dir).arg(name));

        // Not found?  Try .png.
        if (pmap.isNull())
            pmap = QPixmap(QString("%1/%2.png").arg(dir).arg(name));
        // Not found?  Try .jpg.
        if (pmap.isNull())
            pmap = QPixmap(QString("%1/%2.jpg").arg(dir).arg(name));
        // Not found?  Try .xpm.
        if (pmap.isNull())
            pmap = QPixmap(QString("%1/%2.xpm").arg(dir).arg(name));

        return pmap;
    }

}

namespace Rosegarden
{


QIcon
IconLoader::load(QString name)
{
    QPixmap pmap(loadPixmap(name));
    if (pmap.isNull())
        return QIcon();

    return QIcon(pmap);
}

QPixmap
IconLoader::loadPixmap(QString name)
{
    // Check the cache.
    std::map<QString, QPixmap>::const_iterator it = pixmapCache.find(name);
    // If found in the cache, return it.
    if (it != pixmapCache.end())
        return it->second;

    // Try the various possible directories in :pixmaps.
    QPixmap pixmap = loadPixmap2(":pixmaps/toolbar", name);
    if (pixmap.isNull())
        pixmap = loadPixmap2(":pixmaps/transport", name);
    if (pixmap.isNull())
        pixmap = loadPixmap2(":pixmaps/misc", name);
    if (pixmap.isNull())
        pixmap = loadPixmap2(":pixmaps/stock", name);
    if (pixmap.isNull())
        pixmap = loadPixmap2(":pixmaps/icons", name);
    if (pixmap.isNull())
        pixmap = loadPixmap2(":pixmaps/style", name);
    if (pixmap.isNull())
        pixmap = loadPixmap2(":pixmaps", name);

    // Store whatever we found, or didn't, in the cache.
    pixmapCache[name] = pixmap;

    return pixmap;
}


}
