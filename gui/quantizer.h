
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

#ifndef QUANTIZER_H
#define QUANTIZER_H

#include "Event.h"
#include "notepixmapfactory.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class Quantizer
{
public:
    Quantizer();

    /**
     * sets the 'Notation::NoteType' and 'QuantizedDuration' properties
     * does not change the element duration
     */
    void quantize(Track::iterator from,
                  Track::iterator to);

    /**
     * quantize one element to a Note
     * sets the 'Notation::NoteType' and 'QuantizedDuration' properties
     * (@see notepixmapfactory.h for note list)
     */
    void quantize(Event *el);

protected:

    /// actual quantizer
    void quantize(Event::timeT duration, int &high, int &low);
};

#endif
