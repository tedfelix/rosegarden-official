// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <qcolor.h>

#ifndef _TEMPOCOLOUR_H_
#define _TEMPOCOLOUR_H_

// Get a QColor for a tempo
//

class TempoColour
{

public:
    TempoColour():m_tempo(0) {;}
    TempoColour(double tempo):m_tempo(tempo) {;}

    // Get the colour for a tempo
    //
    QColor getColour() { return getColour(m_tempo); }
    static QColor getColour(double tempo);

private:

    double m_tempo;

};

QColor
TempoColour::getColour(double tempo)
{
    // Get hue value
    //
    int h, s, v;
    RosegardenGUIColours::TextRulerBackground.hsv(&h, &s, &v);

    // Adjusted about default tempo of 120bpm - we can play around
    // with this algorithm
    //
    double adjusted = 120.0 - tempo;
    h = h + int(adjusted);

   return QColor(h, s, v, QColor::Hsv);
}



#endif // _TEMPOCOLOUR_H_

