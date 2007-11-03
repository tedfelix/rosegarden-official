
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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
class RosegardenGUIApp;
class RosegardenGUIView;
class RosegardenGUIDoc;
class NotationView;
class Key;
class Composition;

const std::string headerDedication = "dedication";
const std::string headerTitle = "title";
const std::string headerSubtitle = "subtitle";
const std::string headerSubsubtitle = "subsubtitle";
const std::string headerPoet = "poet";
const std::string headerComposer = "composer";
const std::string headerMeter = "meter";
const std::string headerOpus = "opus";
const std::string headerArranger = "arranger";
const std::string headerInstrument = "instrument";
const std::string headerPiece = "piece";
const std::string headerCopyright = "copyright";
const std::string headerTagline = "tagline";

/**
 * Lilypond scorefile export
 */

class LilypondExporter : public ProgressReporter
{
public:
    typedef std::multiset<Event*, Event::EventCmp> eventstartlist;
    typedef std::multiset<Event*, Event::EventEndCmp> eventendlist;

public:
    LilypondExporter(RosegardenGUIApp *parent, RosegardenGUIDoc *, std::string fileName);
    LilypondExporter(NotationView *parent, RosegardenGUIDoc *, std::string fileName);
    ~LilypondExporter();

    bool write();

protected:
    RosegardenGUIView *m_view;
    NotationView *m_notationView;
    RosegardenGUIDoc *m_doc;
    Composition *m_composition;
    Studio *m_studio;
    std::string m_fileName;

    void readConfigVariables(void);
    void writeBar(Segment *, int barNo, int barStart, int barEnd, int col,
                  Rosegarden::Key &key, std::string &lilyText,
                  std::string &prevStyle, eventendlist &eventsInProgress,
                  std::ofstream &str, int &MultiMeasureRestCount,
                  bool &nextBarIsAlt1, bool &nextBarIsAlt2,
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
                  
    std::pair<int,int> writeSkip(const TimeSignature &timeSig,
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

    void handleText(const Event *, std::string &lilyText);
    void writePitch(const Event *note, const Rosegarden::Key &key, std::ofstream &);
    void writeStyle(const Event *note, std::string &prevStyle, int col, std::ofstream &, bool isInChord);
    std::pair<int,int> writeDuration(timeT duration, std::ofstream &);
    void writeSlashes(const Event *note, std::ofstream &);
       
private:
    static const int MAX_DOTS = 4;
    static const PropertyName SKIP_PROPERTY;
    
    unsigned int m_paperSize;
    static const unsigned int PAPER_A3      = 0;
    static const unsigned int PAPER_A4      = 1;
    static const unsigned int PAPER_A5      = 2;
    static const unsigned int PAPER_A6      = 3;
    static const unsigned int PAPER_LEGAL   = 4;
    static const unsigned int PAPER_LETTER  = 5;
    static const unsigned int PAPER_TABLOID = 6;
    static const unsigned int PAPER_NONE    = 7;

    bool m_paperLandscape;
    unsigned int m_fontSize;
    static const unsigned int FONT_11	= 0;
    static const unsigned int FONT_13	= 1;
    static const unsigned int FONT_16	= 2;
    static const unsigned int FONT_19	= 3;
    static const unsigned int FONT_20	= 4;
    static const unsigned int FONT_23	= 5;
    static const unsigned int FONT_26	= 6;

    bool m_exportLyrics;
    bool m_exportMidi;

    unsigned int m_lyricsHAlignment;
    static const unsigned int LEFT_ALIGN   = 0;
    static const unsigned int CENTER_ALIGN = 1;
    static const unsigned int RIGHT_ALIGN  = 2;
    
    unsigned int m_exportTempoMarks;
    static const unsigned int EXPORT_NONE_TEMPO_MARKS = 0;
    static const unsigned int EXPORT_FIRST_TEMPO_MARK = 1;
    static const unsigned int EXPORT_ALL_TEMPO_MARKS = 2;
    
    unsigned int m_exportSelection;
    static const unsigned int EXPORT_ALL_TRACKS = 0;
    static const unsigned int EXPORT_NONMUTED_TRACKS = 1;
    static const unsigned int EXPORT_SELECTED_TRACK = 2;
    static const unsigned int EXPORT_SELECTED_SEGMENTS = 3;

    bool m_exportPointAndClick;
    bool m_exportBeams;
    bool m_exportStaffGroup;
    bool m_exportStaffMerge;
    bool m_raggedBottom;

    int m_languageLevel;
    static const int LILYPOND_VERSION_2_6  = 0;
    static const int LILYPOND_VERSION_2_8  = 1;
    static const int LILYPOND_VERSION_2_10 = 2;
    static const int LILYPOND_VERSION_2_12 = 3;

    std::pair<int,int> fractionSum(std::pair<int,int> x,std::pair<int,int> y) {
	std::pair<int,int> z(
	    x.first * y.second + x.second * y.first,
	    x.second * y.second);
	return fractionSimplify(z);
    }
    std::pair<int,int> fractionProduct(std::pair<int,int> x,std::pair<int,int> y) {
	std::pair<int,int> z(
	    x.first * y.first,
	    x.second * y.second);
	return fractionSimplify(z);
    }
    std::pair<int,int> fractionProduct(std::pair<int,int> x,int y) {
	std::pair<int,int> z(
	    x.first * y,
	    x.second);
	return fractionSimplify(z);
    }
    bool fractionSmaller(std::pair<int,int> x,std::pair<int,int> y) {
	return (x.first * y.second < x.second * y.first);
    }
    std::pair<int,int> fractionSimplify(std::pair<int,int> x) {
	return std::pair<int,int>(x.first/gcd(x.first,x.second),
				  x.second/gcd(x.first,x.second));
    }
    int gcd(int a, int b) {
	// Euclid's algorithm to find the greatest common divisor
	while ( 1 ) {
	    int r = a % b;
	    if ( r == 0 )
		return (b == 0 ? 1 : b);
	    a = b;
	    b = r; 
	}
    }
};



}

#endif

