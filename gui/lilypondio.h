// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _LILYPONDIO_H_
#define _LILYPONDIO_H_

#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include "Event.h"
#include "Segment.h"

namespace Rosegarden { class Composition; class Event; class Segment; }
using Rosegarden::Event;
using Rosegarden::Segment;

/**
 * Lilypond scorefile export
 */

class LilypondExporter
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventstartlist;
    typedef std::multiset<Event*, Event::EventEndCmp> eventendlist;
public:
    LilypondExporter(Rosegarden::Composition *, std::string fileName);
    ~LilypondExporter();

    bool write();

protected:
    Rosegarden::Composition *m_composition;
    std::string m_fileName;
    void handleStartingEvents(eventstartlist &eventsToStart, bool &addTie, std::ofstream &str);
    void handleEndingEvents(eventendlist &eventsInProgress, Segment::iterator &j, std::ofstream &str);
 private:
    static const int MAX_DOTS = 4;
};


#endif /* _LILYPONDIO_H_ */
