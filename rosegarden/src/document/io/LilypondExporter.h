
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    Numerous additions and bug fixes by
        Michael McIntyre    <dmmcintyr@users.sourceforge.net>

    Some restructuring by Chris Cannam.

    Brain surgery to support LilyPond 2.x export by Heikki Junes.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_LILYPONDEXPORTER_H_
#define _RG_LILYPONDEXPORTER_H_

#include "base/Event.h"
#include "base/PropertyName.h"
#include "base/Segment.h"
#include "gui/general/ProgressReporter.h"
#include <fstream>
#include <set>
#include <string>
#include <utility>


class QObject;


namespace Rosegarden
{

class TimeSignature;
class Studio;
class RosegardenGUIDoc;
class Key;
class Composition;


/**
 * Lilypond scorefile export
 */

class LilypondExporter : public ProgressReporter
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventstartlist;
    typedef std::multiset<Event*, Event::EventEndCmp> eventendlist;

public:
    LilypondExporter(QObject *parent, RosegardenGUIDoc *, std::string fileName);
    ~LilypondExporter();

    bool write();

protected:
    RosegardenGUIDoc *m_doc;
    Composition *m_composition;
    Studio *m_studio;
    std::string m_fileName;
    bool m_pitchBorked;

    void writeBar(Segment *, int barNo, int barStart, int barEnd, int col,
                  Rosegarden::Key &key, std::string &lilyText, std::string &lilyLyrics,
                  std::string &prevStyle, eventendlist &eventsInProgress,
                  std::ofstream &str, bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                  bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot);
    
    timeT calculateDuration(Segment *s,
                                        const Segment::iterator &i,
                                        timeT barEnd,
                                        timeT &soundingDuration,
                                        const std::pair<int, int> &tupletRatio,
                                        bool &overlong);

    void handleStartingEvents(eventstartlist &eventsToStart, std::ofstream &str);
    void handleEndingEvents(eventendlist &eventsInProgress,
                            const Segment::iterator &j, std::ofstream &str);

    // convert note pitch into Lilypond format note string
    std::string convertPitchToLilyNote(int pitch,
                                       Accidental accidental,
                                       const Rosegarden::Key &key);

    // compose an appropriate Lilypond representation for various Marks
    std::string composeLilyMark(std::string eventMark, bool stemUp);

    // find/protect illegal characters in user-supplied strings
    std::string protectIllegalChars(std::string inStr);

    // return a string full of column tabs
    std::string indent(const int &column);
                  
    void writeSkip(const TimeSignature &timeSig,
                   timeT offset,
                   timeT duration,
                   bool useRests,
                   std::ofstream &);

    /*
     * Handle Lilypond directive.  Returns true if the event was a directive,
     * so subsequent code does not bother to process the event twice
     */
    bool handleDirective(const Event *textEvent,
                         std::string &lilyText,
                         bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                         bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot);

    void handleText(const Event *, std::string &lilyText, std::string &lilyLyrics);
    void writePitch(const Event *note, const Rosegarden::Key &key, std::ofstream &);
    void writeStyle(const Event *note, std::string &prevStyle, int col, std::ofstream &, bool isInChord);
    void writeDuration(timeT duration, std::ofstream &);
    void writeSlashes(const Event *note, std::ofstream &);
       
private:
    static const int MAX_DOTS = 4;
    static const PropertyName SKIP_PROPERTY;
    
    unsigned int m_paperSize;
    unsigned int m_fontSize;
    bool m_exportLyrics;
    bool m_exportHeaders;
    bool m_exportMidi;

        // exportTempoMarks meaning:
        // 0 -> none
        // 1 -> first
        // 2 -> all
    unsigned int m_exportTempoMarks;
    bool m_exportUnmuted;
    bool m_exportPointAndClick;
    bool m_exportBarChecks;
    bool m_exportBeams;
    bool m_exportStaffGroup;
    bool m_exportStaffMerge;

        // languagelevel meaning:
        // 0 -> Lilypond 2.2
        // 1 -> Lilypond 2.4
        // 2 -> Lilypond 2.6
        // 3 -> Lilypond 2.8
    int m_languageLevel;
};



}

#endif
