/***************************************************************************
                          chord.h  -  description
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
/***************************************************************************
                          chord.h  -  description
                             -------------------
    begin                : Mon Jun 12 2000
    copyright            : (C) 2000 by Guillaume Laurent
    email                : glaurent@telegraph-road.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CHORD_H
#define CHORD_H

#include <qcanvasitemgroup.h>

/**
  *@author Guillaume Laurent
  */

class Chord : public QCanvasItemGroup  {
public:
    Chord(QCanvas*);

    void addNote(int pitch);
    void removeNote(int pitch);

    void setDuration(unsigned int d);

    unsigned int duration() const { return m_duration; }

protected:
    unsigned int m_duration;
    QCanvasPixmapArray *m_notePixmapArray;
    QValueList<int>    m_pitches; // may be use a vector here ?
};

#endif
