/***************************************************************************
                          staff.h  -  description
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

#ifndef STAFF_H
#define STAFF_H

#include <vector>

#include "qcanvasitemgroup.h"

class QCanvasLineGroupable;

/**
 * A Staff (treble and bass clef + lines) as displayed on screen.
 *
 *@author Guillaume Laurent
 */

class Staff : public QCanvasItemGroup
{
public:
    enum Clef { Treble, Bass, Alto, Tenor };

    typedef vector<QCanvasLineGroupable*> barlines;
    
    Staff(QCanvas*, Clef clef = Treble);
    ~Staff();

    /**
     * Returns the Y offset at which a note with pitch 'p'
     * should be displayed on this staff
     */
    int pitchYOffset(int p) const;

    /// Returns the height of a bar line
    unsigned int barLineHeight() const { return m_barLineHeight; }

    void insertBar(unsigned int barPos);
    void deleteBars(unsigned int fromPos);
    void deleteBars();

    static const unsigned int noteHeight;
    static const unsigned int noteWidth;
    static const unsigned int lineWidth;
    static const unsigned int accidentWidth;
    static const unsigned int stalkLen;
    static const unsigned int nbLines;
    static const unsigned int linesOffset;

protected:

    void makeInvisibleLine(int y, int pitch);

    Clef m_currentKey;
    unsigned int m_barLineHeight;
    unsigned int m_horizLineLength;
    vector<int> m_pitchToHeight;

    barlines m_barLines;
};

#endif
