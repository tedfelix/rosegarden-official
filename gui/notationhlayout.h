/***************************************************************************
                          notationhlayout.h  -  description
                             -------------------
    begin                : Thu Aug 3 2000
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

#ifndef NOTATIONHLAYOUT_H
#define NOTATIONHLAYOUT_H

#include "layoutengine.h"
#include "quantizer.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class NotationHLayout : public LayoutEngine  {
public:
    /**
     * Create a new NotationHLayout object.
     * barWidth is the length of a bar in pixels
     * beatsPerBar is the nb of beats a bar is in the current time sig
     */
    NotationHLayout(unsigned int barWidth,
                    unsigned int beatsPerBar,
                    unsigned int barMargin,
                    unsigned int noteMargin = 2);
protected:
    typedef vector<unsigned int> NoteLengthMap;

    virtual void layout(Event*);

    void initNoteLengthTable();

    Quantizer m_quantizer;

    unsigned int m_barWidth;
    unsigned int m_beatsPerBar;
    unsigned int m_barMargin;
    /// minimal space between two notes
    unsigned int m_noteMargin;

    unsigned int m_nbBeatsInCurrentBar;
    unsigned int m_currentPos;

    NoteLengthMap m_noteLengthMap;

};

#endif
