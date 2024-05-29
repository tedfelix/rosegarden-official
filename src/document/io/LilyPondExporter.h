/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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

#ifndef RG_LILYPONDEXPORTER_H
#define RG_LILYPONDEXPORTER_H

#include "base/Event.h"
#include "base/PropertyName.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "document/io/LilyPondLanguage.h"
#include "gui/editors/notation/NotationView.h"
#include <fstream>
#include <set>
#include <string>
#include <utility>

#include <QCoreApplication>
#include <QPointer>

class QObject;
class QProgressDialog;
class QString;


namespace Rosegarden
{

// This is now in the Rosegarden namespace so it can be used
// in LilyPondOptionsDialog with the LilyVersionAwareCheckBox.
enum {
  LILYPOND_VERSION_2_12,
  LILYPOND_VERSION_2_14,
  LILYPOND_VERSION_2_16,
  LILYPOND_VERSION_2_18,
  LILYPOND_VERSION_2_19,
  LILYPOND_VERSION_2_20,
  LILYPOND_VERSION_2_21,
  LILYPOND_VERSION_2_22,
  LILYPOND_VERSION_2_23
};

class TimeSignature;
class Studio;
class RosegardenMainWindow;
class RosegardenMainViewWidget;
class RosegardenDocument;
class NotationView;
class Key;
class Composition;
class LilyPondSegmentsContext;

const char* headerDedication();
const char* headerTitle();
const char* headerSubtitle();
const char* headerSubsubtitle();
const char* headerPoet();
const char* headerComposer();
const char* headerMeter();
const char* headerOpus();
const char* headerArranger();
const char* headerInstrument();
const char* headerPiece();
const char* headerCopyright();
const char* headerTagline();

/**
 * LilyPond scorefile export
 */

class ROSEGARDENPRIVATE_EXPORT LilyPondExporter
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::LilyPondExporter)

public:
    typedef EventContainer eventstartlist;
    typedef std::multiset<Event*, Event::EventEndCmp> eventendlist;

public:
    LilyPondExporter(RosegardenDocument *doc,
                     const SegmentSelection &selection,
                     const std::string &fileName,
                     NotationView *parent = nullptr);
    ~LilyPondExporter();

   /**
    * Write the LilyPond score into the specified file.
    * @return true if successfull, false otherwise.
    */
    bool write();

   /**
    * @return an explanatory message on what goes wrong on the last
    * call to write().
    */
    QString getMessage() const { return m_warningMessage; }

    void setProgressDialog(QPointer<QProgressDialog> progressDialog)
            { m_progressDialog = progressDialog; }

private:
    NotationView *m_notationView;
    RosegardenDocument *m_doc;
    Composition *m_composition;
    Studio *m_studio;
    std::string m_fileName;
    Clef m_lastClefFound;
    LilyPondLanguage *m_language;
    SegmentSelection m_selection;

    void readConfigVariables();

    Event *nextNoteInGroup(Segment *s, Segment::iterator it, const std::string &groupType, int barEnd) const;

    // Return true if the given segment has to be print
    // (readConfigVAriables() should have been called before)
    bool isSegmentToPrint(Segment *seg);

    void writeBar(Segment *, int barNo, timeT barStart, timeT barEnd, int col,
                  Rosegarden::Key &key, std::string &lilyText,
                  std::string &prevStyle,
                  eventendlist &preEventsInProgress, eventendlist &postEventsInProgress,
                  std::ofstream &str, int &MultiMeasureRestCount,
                  bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                  bool &nextBarIsDouble, bool &nextBarIsEnd,
                  bool &nextBarIsDot, bool noTimeSignature);

    timeT calculateDuration(Segment *s,
                                        const Segment::iterator &i,
                                        timeT barEnd,
                                        timeT &soundingDuration,
                                        const std::pair<int, int> &tupletRatio,
                                        bool &overlong);

    void handleStartingPreEvents(eventstartlist &preEventsToStart,
                                 const Segment *seg,
                                 const Segment::iterator &j,
                                 std::ofstream &str);
    void handleEndingPreEvents(eventendlist &preEventsInProgress,
                               const Segment::iterator &j,
                               std::ofstream &str);
    void handleStartingPostEvents(eventstartlist &postEventsToStart,
                                  const Segment *seg,
                                  const Segment::iterator &j,
                                  std::ofstream &str);
    void handleEndingPostEvents(eventendlist &postEventsInProgress,
                                const Segment *seg,
                                const Segment::iterator &j,
                                std::ofstream &str);

    // convert note pitch into LilyPond format note name string
    std::string convertPitchToLilyNoteName(int pitch,
                                           Accidental accidental,
                                           const Rosegarden::Key &key) const;

    // convert note pitch into LilyPond format note name string with octavation
    std::string convertPitchToLilyNote(int pitch,
                                       Accidental accidental,
                                       const Rosegarden::Key &key) const;

    // compose an appropriate LilyPond representation for various Marks
    static std::string composeLilyMark(std::string eventMark, bool stemUp);

    // find/protect illegal characters in user-supplied strings
    static std::string protectIllegalChars(const std::string& inStr);

    // return a string full of column tabs
    static std::string indent(const int &column);

    // write a time signature
    static void writeTimeSignature(const TimeSignature& timeSignature,
                                   int col,
                                   std::ofstream &str);

    std::pair<int,int> writeSkip(const TimeSignature &timeSig,
				 timeT offset,
				 timeT duration,
				 bool useRests,
				 std::ofstream &);

    /*
     * Handle LilyPond directive.  Returns true if the event was a directive,
     * so subsequent code does not bother to process the event twice
     */
    static bool handleDirective(const Event *textEvent,
                                std::string &lilyText,
                                bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                                bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot);

    void handleText(const Event *, std::string &lilyText) const;
    static void handleGuitarChord(Segment::iterator i, std::ofstream &str);
    void writePitch(const Event *note, const Rosegarden::Key &key, std::ofstream &);
    void writeStyle(const Event *note, std::string &prevStyle, int col, std::ofstream &, bool isInChord);
    std::pair<int,int> writeDuration(timeT duration, std::ofstream &);
    void writeSlashes(const Event *note, std::ofstream &);

    /*
     * Return the verse with index currentVerse from the givenSegment ready to
     * be write in LilyPond file with indentCol indentation.
     */
    QString getVerseText(Segment *seg, int currentVerse, int indentCol);

    /*
     * Write in str the lyrics verse of index verseIndex from the given
     * segment with the indentation indentCol or an appropriate LilyPond
     * skip sequence if the verse doesn't exist.
     */
    void writeVerse(Segment *seg, int verseIndex, int indentCol, std::ofstream &str);

    /*
     * Write in str all the lyrics verses using volta and alternativete endings
     * of a given line and cycle with the indentation indentCol.
     * Repeatedly call writeVerse() for each segment.
     */
    void writeVersesWithVolta(LilyPondSegmentsContext & lsc,
                              int verseLine, int cycle,
                              int indentCol, std::ofstream &str);
    /*
     * Write in str all the lyrics verses of an unfolded score (i.e. without
     * volta) for a given line and cycle with the indentation indentCol.
     * Repeatedly call writeVerse() for each segment.
     */
    void writeVersesUnfolded(LilyPondSegmentsContext & lsc,
                             std::map<Segment *, int> & verseIndexes,
                             int verseLine, int cycle,
                             int indentCol, std::ofstream &str);

    /* Used to embed a lyric syllable with a bar number */
    struct Syllable {

    Syllable(const QString& syllable, int bar) :
        syllableString(syllable),
        syllableBar(bar)
        {
        }

        /**
         *  Protect the syllable with quotes if needed.
         *  Return true if the string is modified.
         */
        bool protect();

        /**
         *  Always add double quotes around the string.
         */
        void addQuotes();

        QString syllableString;
        int syllableBar;
    };


private:
    static const int MAX_DOTS = 4;
    const PropertyName SKIP_PROPERTY;

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
    static const unsigned int EXPORT_EDITED_SEGMENTS = 4;

    bool m_exportBeams;
    bool m_exportStaffGroup;

    bool m_raggedBottom;
    bool m_exportEmptyStaves;
    bool m_useShortNames;

    unsigned int m_exportMarkerMode;

    bool m_chordNamesMode;

    enum {
        EXPORT_NO_MARKERS,
        EXPORT_DEFAULT_MARKERS,
        EXPORT_TEXT_MARKERS,
    };


    unsigned int m_exportNoteLanguage;

    int m_languageLevel;
//  There is now an enum global within the Rosegarden namespace (declared
//  at the beggining of this file) that contains values such as:
//     enum {
//         LILYPOND_VERSION_2_6,
//         LILYPOND_VERSION_2_8,
//         ...
//     };
// These are the values used with m_languageLevel.

    bool m_useVolta;          // If true, use volta whenever possible
    bool m_altBar;            // Draw a bar at the beginning of an alternative
    bool m_cancelAccidentals;
    bool m_fingeringsInStaff;
    QString m_warningMessage;

    QPointer<QProgressDialog> m_progressDialog;

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
    static bool fractionSmaller(std::pair<int,int> x,std::pair<int,int> y) {
	return (x.first * y.second < x.second * y.first);
    }
    std::pair<int,int> fractionSimplify(std::pair<int,int> x) {
	return std::pair<int,int>(x.first/gcd(x.first,x.second),
				  x.second/gcd(x.first,x.second));
    }
    static int gcd(int a, int b) {
	// Euclid's algorithm to find the greatest common divisor
	while ( b != 0) {
	    int t = b;
            b = a % b;
            a = t;
	}
        return a;
    }
};


}

#endif


/*

HOW THE VERSES RELATED TO A VOICE SHOULD BE WRITTEN IN LILYPOND FILE
====================================================================

The verses are written in the Lilypond file after the musical notes of
a voice. Each verse, or, more exactly, each line, is written in a
"\new Lyrics" sequence and connected to the voice using a "\lyricsto"
command.

PART 1 - The number of verses is exactly what each volta requires
-----------------------------------------------------------------

Rosegarden unfolded segments (X, X',... are linked segments)

A B B1 B' B1' B" B2 C D D' D" E F F1 F' F2 F" F1' F"' F3 G


Musical segments (Lilypond order):
                       ____________        __        _________________
Volta sequences:      /            \      /  \      /                 \
Nb of repeats:   0    2    -    -    0    2    0    3    -    -    -    0
                 ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
Segments:       |A   |B   |B1  |B2  |C   |D   |E   |F   |F1  |F2  |F3  |G   |
                 ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
Alternates #               1,2  3   :         :          1,3  2    4   :
                                    :         :                        :
                                    :         :                        :
Lyrics text (Lilypond order):       :         :                        :
                                    :         :                        :
                 A    B    B1       :         :                        :
Line 1          |xxxx|xxxx|xxxx|....|....|....|....|....|....|....|....|....|
Verse #          1    1    1        :         :                        :
                                    :         :                        :
                      B    B1       :         :                        :
Line 2          |....|xxxx|xxxx|....|....|....|....|....|....|....|....|....|
Verse #               2    2        :         :                        :
                                    :         :                        :
                      B         B2  :C    D   :                        :
Line 3          |....|xxxx|xxxx|xxxx|xxxx|xxxx|....|....|....|....|....|....|
Verse #               3         1   :1    1   :                        :
                                    :         :                        :
                                    :     D   :                        :
Line 4          |....|....|....|....|....|xxxx|....|....|....|....|....|....|
Verse #                             :     2   :                        :
                                    :         :                        :
                                    :     D   :E    F    F1            :
Line 5          |....|....|....|....|....|xxxx|xxxx|xxxx|xxxx|....|....|....|
Verse #                             :     3   :1    1    1             :
                                    :         :                        :
                                    :         :     F         F2       :
Line 6          |....|....|....|....|....|....|....|xxxx|....|xxxx|....|....|
Verse #                             :         :     2         1        :
                                    :         :                        :
                                    :         :     F    F1            :
Line 7          |....|....|....|....|....|....|....|xxxx|xxxx|....|....|....|
Verse #                             :         :     3    2             :
                                    :         :                        :
                                    :         :     F              F3  :G
Line 8          |....|....|....|....|....|....|....|xxxx|....|....|xxxx|xxxx|
Verse #                             :         :     4              1   :1
                                    :         :                        :
VoltaCount       1                  |3        |5                       |8

 xxx: Lyrics
 ...: Skipped notes (blank lines)

This sequence of lyrics and skipped notes gives a good vertical alignment
of the lyrics:

    AAAA AAAA BBBB BBBB BBBB B1B1 B1B1
              BBBB BBBB BBBB B1B1 B1B1
              BBBB BBBB BBBB           B2B2 B2B2 CCCC CCCC CCCC etc...

LilyPond automatically removes the unnecessary blank lines.

VoltaCount start from one at the beginning of the composition and is
incremented by the number of voltas after each repeating sequence (i.e. volta
and possible alternate endings segments).

Number of lines = 1 + sum(volta_repeats_number)

In the here above example:
    segment B is repeated 2 times,
    segment D is repeated 2 times,
    segment F is repeated 3 times

So number of lines = 1 + 2 + 2 + 3 = 8

Note : When a segment is played 3 times (or have 3 voltas), it can be seen as
       played once and repeated twice.
       So its number of repeats used above is 2.



In a given line :

The printed verse number of a volta segment is:
    verse = lineNumber + 1 - voltaCount
If this verse doesn't exist, the lyrics are skipped.

The lyrics of an alternate segment are only printed if
    alternate number == lineNumber + 1 - voltaCount
If this alternate number doesn't exist, the lyrics of the segment are skipped.

The verses of an alternate segment are successively printed, from the first to
the last, each time the lyrics have to be printed following the previous rule.

(Note: For an easy understanding, lines and verses numbers start from one.
This may not be the case in the code.)



PART 2 - There is more verses than required by each volta
---------------------------------------------------------

This typically happens when there is no indicated volta and several verses
(for example, "himno_de_riego.rg" in RG examples).

In such a case, all the supplementary verses are written below the first one.

The same policy can be applied in more complex situations involving volta.

The "number of verses" and the "number of cycles" values are used here.
In "himno_de_riego.rg" example:
   - there is only one sequence in the musical score: so "number of verse" is 1
   - there is 3 verses stored as lyrics: so "number of cycles" is 3.
It means that the musical score have to be played three times (three cycles) to
consume all ther lyrics.

    Number of verses:
        It's how many lines are needed to write the verses under
        the score with the correct vertical alignment and with
        one and only one verse for each musical volta.

        For example:
            the score "aaa ||: bbb x3 :|| ccc"
            has versesNumber = 3
            and "aaa ||: bbb x3 :|| ccc ||: ddd x2 :|| eee"
            has versesNumber = 4

        It's because the verse under ccc follows the third verse
        of bbb and is on the third line.
        The first verse of ddd follows the verse of ccc and is
        on the fourth line. The verse of eee follows the second
        verse of ddd and is on the fourth and last line.

        When the voltas are unfolded, the number of verses
        is always 1

        Number of cycles:
            This is how many verses are written under the score
            although no repetition is indicated in this score.
            This can be seen as how many times the musical score has
            to be played to exhaust all the verses.
            ("Himno de Riego", in the RG examples, has no notated
            repetition but has three verses. Which gives
            versesNumber = 1 and cyclesNumber = 3)

            For example the following score has cyclesNumber = 2
                aaaaaaaaa
                    verse 1
                    verse 2

            and the following one also has cyclesNumber = 2
                aaaaaaaaa ||: bbbbb x2 :|| cccccccccc
                    verse 1a     verse 1b
                                verse 2b     verse 1c
                    verse 2a     verse 3b
                                verse 4b     verse 2c

    Note: The number of cycles is always computed as if the set
            of verses is complete. In the last example, if verses
            4b and 2c are omitted, cyclesNumber = 4 is nevertheless
            computed.
            The exporter is going to write blank verses, but
            LilyPond will happily ignore them.

    Note: When volta are printed unfolded (m_useVolta is false)
            the "number of verses" as previously described is
            always 1 and the "number of cycles" is the maximum
            number of verses found in one segment.

*/
