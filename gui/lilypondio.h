// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

/**
 * Lilypond scorefile export
 */

class LilypondExporter : public ProgressReporter
{
public:
    typedef std::multiset<Rosegarden::Event*, Rosegarden::Event::EventCmp> eventstartlist;
    typedef std::multiset<Rosegarden::Event*, Rosegarden::Event::EventEndCmp> eventendlist;
public:
    LilypondExporter(QObject *parent, Rosegarden::Composition *,
                     std::string fileName);
    ~LilypondExporter();

    bool write();

protected:
    Rosegarden::Composition *m_composition;
    std::string m_fileName;

    void writeBar(Rosegarden::Segment *, int barNo, int col,
		  Rosegarden::Key &key, std::string &lilyText, std::string &lilyLyrics,
		  std::string &prevStyle, eventendlist &eventsInProgress,
		  std::ofstream &str);
    
    Rosegarden::timeT calculateDuration(Rosegarden::Segment *s,
					const Rosegarden::Segment::iterator &i,
					Rosegarden::timeT barEnd,
					Rosegarden::timeT &soundingDuration,
					const std::pair<int, int> &tupletRatio,
					bool &overlong);

    void handleStartingEvents(eventstartlist &eventsToStart, std::ofstream &str);
    void handleEndingEvents(eventendlist &eventsInProgress,
			    const Rosegarden::Segment::iterator &j, std::ofstream &str);

    // convert note pitch into Lilypond format note string
    std::string convertPitchToLilyNote(int pitch,
                                       Rosegarden::Accidental accidental,
				       const Rosegarden::Key &key);

    // compose an appropriate Lilypond representation for various Marks
    std::string composeLilyMark(std::string eventMark, bool stemUp);

    // find/protect illegal characters in user-supplied strings
    std::string protectIllegalChars(std::string inStr);

    // return a string full of column tabs
    std::string indent(const int &column);
                  
    void writeSkip(const Rosegarden::TimeSignature &timeSig,
		   Rosegarden::timeT offset,
		   Rosegarden::timeT duration,
		   bool useRests,
		   std::ofstream &);

    void handleText(const Rosegarden::Event *, std::string &lilyText, std::string &lilyLyrics);
    void writePitch(const Rosegarden::Event *note, const Rosegarden::Key &key, std::ofstream &);
    void writeStyle(const Rosegarden::Event *note, std::string &prevStyle, int col, std::ofstream &);
    void writeDuration(Rosegarden::timeT duration, std::ofstream &);
    void writeSlashes(const Rosegarden::Event *note, std::ofstream &);
       
private:
    static const int MAX_DOTS = 4;
    static const Rosegarden::PropertyName SKIP_PROPERTY;
};


#endif /* _LILYPONDIO_H_ */
