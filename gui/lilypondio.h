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
#include "progressreporter.h"

namespace Rosegarden { class Composition; class Event; class Segment; }
using Rosegarden::Event;
using Rosegarden::Segment;

/**
 * Lilypond scorefile export
 */

class LilypondExporter : public ProgressReporter
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventstartlist;
    typedef std::multiset<Event*, Event::EventEndCmp> eventendlist;
public:
    LilypondExporter(QObject *parent, Rosegarden::Composition *,
                     std::string fileName);
    ~LilypondExporter();

    bool write();

protected:
    Rosegarden::Composition *m_composition;
    std::string m_fileName;
    void handleStartingEvents(eventstartlist &eventsToStart, std::ofstream &str);
    void handleEndingEvents(eventendlist &eventsInProgress, Segment::iterator &j, std::ofstream &str);

    // convert note event into Lilypond format note string using combination
    // of pitch, flat/sharp key signature, number of accidentals in key
    // signature, and any extra accidentals
    std::string convertPitchToLilyNote(long pitch,
                                       bool isFlatKeySignature,
                                       int accidentalCount,
                                       Rosegarden::Accidental accidental);

    // compose an appropriate Lilypond representation for various Marks
    std::string composeLilyMark(std::string eventMark, bool stemUp);

    // find/protect illegal characters in user-supplied strings
    std::string protectIllegalChars(std::string inStr);

    // return a string full of column tabs
    std::string indent(const int &column);
                  
    // close chord if necessary, and/or add tie if necessary; can do one or
    // both independantly
    void closeChordWriteTie(bool &addTie, bool &currentlyWritingChord, std::ofstream &str);
    
 private:
    static const int MAX_DOTS = 4;
};


#endif /* _LILYPONDIO_H_ */
