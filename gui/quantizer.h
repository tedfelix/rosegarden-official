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

#include "Element2.h"
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
    void quantize(EventList::iterator from,
                  EventList::iterator to);

    /**
     * quantize one element to a Note
     * sets the 'Notation::NoteType' and 'QuantizedDuration' properties
     * (@see notepixmapfactory.h for note list)
     */
    void quantize(Event *el);

    unsigned int wholeNoteDuration()           { return m_wholeNoteDuration; }
    void  setWholeNoteDuration(unsigned int d);

    Event::duration noteDuration(Note note);

protected:

    typedef vector<unsigned int> DurationMap;

    /// actual quantizer
    void quantize(Event::duration drt,
                  DurationMap::iterator &high, 
                  DurationMap::iterator &low);

    /// maps Notes to durations
    DurationMap m_durationTable;

    void computeNoteDurations();

    unsigned int m_wholeNoteDuration;
    static unsigned int defaultWholeNoteDuration;
};

#endif
