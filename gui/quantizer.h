/***************************************************************************
                          quantizer.h  -  description
                             -------------------
    begin                : Thu Aug 17 2000
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
