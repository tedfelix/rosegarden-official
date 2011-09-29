/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

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
#include "document/io/LilyPondLanguage.h"
#include "gui/general/ProgressReporter.h"
#include "gui/editors/notation/NotationView.h"
#include <fstream>
#include <set>
#include <string>
#include <utility>


class QObject;


namespace Rosegarden
{

class TimeSignature;
class Studio;
class RosegardenMainWindow;
class RosegardenMainViewWidget;
class RosegardenDocument;
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
 * LilyPond scorefile export
 */

class LilyPondExporter : public ProgressReporter
{
    //Q_OBJECT
public:
    typedef std::multiset<Event*, Event::EventCmp> eventstartlist;
    typedef std::multiset<Event*, Event::EventEndCmp> eventendlist;

public:
    LilyPondExporter(RosegardenMainWindow *parent, RosegardenDocument *, std::string fileName);
    LilyPondExporter(NotationView *parent, RosegardenDocument *, std::string fileName);
    ~LilyPondExporter();

    bool write();

protected:
    RosegardenMainViewWidget *m_view;
    NotationView *m_notationView;
    RosegardenDocument *m_doc;
    Composition *m_composition;
    Studio *m_studio;
    std::string m_fileName;
    Clef m_lastClefFound;
    LilyPondLanguage *m_language;

    void readConfigVariables(void);

    // Return true if the given segment has to be print
    // (readConfigVAriables() should have been called before)
    bool isSegmentToPrint(Segment *seg);

    void writeBar(Segment *, int barNo, timeT barStart, timeT barEnd, int col,
                  Rosegarden::Key &key, std::string &lilyText,
                  std::string &prevStyle,
                  eventendlist &preEventsInProgress, eventendlist &postEventsInProgress,
                  std::ofstream &str, int &MultiMeasureRestCount,
                  bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                  bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot);
    
    timeT calculateDuration(Segment *s,
                                        const Segment::iterator &i,
                                        timeT barEnd,
                                        timeT &soundingDuration,
                                        const std::pair<int, int> &tupletRatio,
                                        bool &overlong);

    void handleStartingPreEvents(eventstartlist &preEventsToStart, std::ofstream &str);
    void handleEndingPreEvents(eventendlist &preEventsInProgress,
                               const Segment::iterator &j, std::ofstream &str);
    void handleStartingPostEvents(eventstartlist &postEventsToStart, std::ofstream &str);
    void handleEndingPostEvents(eventendlist &postEventsInProgress,
                                const Segment::iterator &j, std::ofstream &str);

    // convert note pitch into LilyPond format note name string
    std::string convertPitchToLilyNoteName(int pitch,
                                           Accidental accidental,
                                           const Rosegarden::Key &key);

    // convert note pitch into LilyPond format note name string with octavation
    std::string convertPitchToLilyNote(int pitch,
                                       Accidental accidental,
                                       const Rosegarden::Key &key);

    // compose an appropriate LilyPond representation for various Marks
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
     * Handle LilyPond directive.  Returns true if the event was a directive,
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

    /** Combo box index starts at 0, but our minimum font size is 6, so we add 6
     * to the index to arrive at the real font size for export
     */
    static const unsigned int FONT_OFFSET = 6;
    static const unsigned int FONT_20 = 20 + FONT_OFFSET;

    unsigned int m_exportLyrics;
    static const unsigned int EXPORT_NO_LYRICS = 0;
    static const unsigned int EXPORT_LYRICS_LEFT = 1;
    static const unsigned int EXPORT_LYRICS_CENTER = 2;
    static const unsigned int EXPORT_LYRICS_RIGHT = 3;

    unsigned int m_exportTempoMarks;
    static const unsigned int EXPORT_NONE_TEMPO_MARKS = 0;
    static const unsigned int EXPORT_FIRST_TEMPO_MARK = 1;
    static const unsigned int EXPORT_ALL_TEMPO_MARKS = 2;
    
    unsigned int m_exportSelection;
    static const unsigned int EXPORT_ALL_TRACKS = 0;
    static const unsigned int EXPORT_NONMUTED_TRACKS = 1;
    static const unsigned int EXPORT_SELECTED_TRACK = 2;
    static const unsigned int EXPORT_SELECTED_SEGMENTS = 3;

    bool m_exportBeams;
    bool m_exportStaffGroup;

    bool m_raggedBottom;

    unsigned int m_exportMarkerMode;

    bool m_chordNamesMode;
    
    static const unsigned int EXPORT_NO_MARKERS = 0;
    static const unsigned int EXPORT_DEFAULT_MARKERS = 1;
    static const unsigned int EXPORT_TEXT_MARKERS = 2;

    unsigned int m_exportNoteLanguage;

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

