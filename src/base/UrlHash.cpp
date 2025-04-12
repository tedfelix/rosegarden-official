/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "UrlHash.h"

#include <QUrl>
#include <QHash> // to ensure correct qHash(const QString &) is found
#if QT_VERSION < 0x040700
unsigned int
qHash(const QUrl &u)
{
    return qHash(u.toString());
}
#endif
