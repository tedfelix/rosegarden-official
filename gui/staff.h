
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef STAFF_H
#define STAFF_H

#include <vector>

#include "qcanvasitemgroup.h"

class QCanvasLineGroupable;


class Staff : public QCanvasItemGroup
{
public:
    typedef vector<QCanvasLineGroupable*> barlines;
    
    Staff(QCanvas*);
    ~Staff();

    int yCoordOfHeight(int height) const;

    /// Returns the height of a bar line
    unsigned int barLineHeight() const { return m_barLineHeight; }

    void insertBar(unsigned int barPos, bool correct);
    void deleteBars(unsigned int fromPos);
    void deleteBars();

    static const int noteHeight;     // height of a note body
    static const int noteWidth;      // width of a note body
    static const int lineWidth;      // delta-y from one staff line to next
    static const int accidentWidth;  // width of an accidental
    static const int stalkLen;       // length of the stalk of a note (bad!)
    static const int nbLines;        // number of main lines on the staff
    static const int linesOffset;    // from top of canvas to top line (bad!)

protected:
    int m_barLineHeight;
    int m_horizLineLength;

    barlines m_barLines;
};

#endif
