// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#endif // _TEMPOCOLOUR_H_

