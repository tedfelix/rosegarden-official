/***************************************************************************
                          notepixmapfactory.cpp  -  description
                             -------------------
    begin                : Mon Jun 19 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "chord.h"
#include "pitchtoheight.h"
#include "staff.h"
#include "qcanvasspritegroupable.h"

Chord::Chord(QCanvas *c)
    : QCanvasItemGroup(c),
      m_duration(2), // totally dummy value here - don't care yet
      m_notePixmapArray(0)
{
    m_notePixmapArray = new QCanvasPixmapArray("pixmaps/note-bodyfilled.xpm");
    setActive(true);
}

void
Chord::addNote(int pitch)
{
    const vector<int>& pitchToHeight(PitchToHeight::instance());

    if(m_pitches.contains(pitch)) return;

    m_pitches.append(pitch);

    QCanvasSpriteGroupable *newNote = new QCanvasSpriteGroupable(m_notePixmapArray,
                                                                 canvas(),
                                                                 this);
    newNote->setY(pitchToHeight[pitch]);
}

void
Chord::removeNote(int pitch)
{
    if(!m_pitches.contains(pitch)) return;
}

void
Chord::setDuration(unsigned int d)
{
    if (d == m_duration) return;

    m_duration = d;

    delete m_notePixmapArray;
    m_notePixmapArray = new QCanvasPixmapArray("pixmaps/note-bodyfilled.xpm");
    // do something more complex later on
}
