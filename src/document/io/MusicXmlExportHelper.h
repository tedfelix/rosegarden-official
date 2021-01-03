
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    More or less complete rewrite (Aug 2011)
        Niek van den Berg   <niekjvandenberg@gmail.com>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MUSICXMLEXPORTHELPER_H
#define RG_MUSICXMLEXPORTHELPER_H

#include "PercussionMap.h"

#include "base/NotationTypes.h"
#include "base/Track.h"
#include "base/Segment.h"
#include "gui/application/RosegardenMainViewWidget.h"

// MusicXML supports only 6 slurs at thesame time is a single part.
#define MAXSLURS 6


class QObject;

namespace Rosegarden
{

class Key;
class Clef;

typedef std::vector<TrackId> TrackVector;

/**
 */

class MusicXmlExportHelper
{
public:
    // The exporter doesn't try to find a reasonable divisions number but
    // simply uses the samen number which is used internally.
    static const int divisions = 960;

    typedef std::map<Segment *, int> VoiceMap;

    //! NOTE Temporary solution to store direction/notations for the future.
    //! A better construction is possible however this might influence the old
    //! MxmlEvent algorithme. First clean up this part and then a better solution
    //! will be implemented.
    class SimpleQueue {
    public:
        bool        direction;
        int         staff;
        int         voice;
        timeT       time;
        std::string string;
    };

    /**
     * This class contains some data to support multi stave part. A MusicXmlExporter
     * will have a StaffInfo for each staff (=track) in the part.
     */
    class StaffInfo {
    public:
        StaffInfo(TrackId id=0) :
                trackId(id)
        {
            voice = 0;
            time = 0;
            startTime = 0;
            endTime = 0;
            firstVoice = 0;
            lastVoice = 0;
        }

        TrackId                 trackId;
        int                     voice;   // Primary voice for the staff.
        std::vector<Segment *>  segments;
        timeT                   time;
        timeT                   startTime;
        timeT                   endTime;
        int                     firstVoice;
        int                     lastVoice;
        Key                     key;
        Clef                    clef;
        AccidentalTable         accTable;
    };
    typedef std::map<int, StaffInfo> StaffMap;

    /**
     * MusicXmlExporter represents a MusicXML part.
     */

public:
    MusicXmlExportHelper(const std::string &name, const TrackVector &tracklist, bool percussion,
                    bool selectedSegmentsOnly,
                    timeT compositionEndTime, Composition *composition,
                    RosegardenMainViewWidget *view,
                    AccidentalTable::OctaveType octaveType,
                    AccidentalTable::BarResetType barResetType);
    ~MusicXmlExportHelper();

    /**
     * Returns the part name.
     */
    std::string getPartName() {return m_partName;}

    /**
     * Returns true if the part is a multi staff part.
     */
    bool isMultiStave() {return m_staves.size() > 1;}

    /**
     * Returns the number of staves in the part.
     */
    int getStaffCount() {return m_staves.size();}

    /**
     * Returns the number of active voices at give time.
     *
     * @param time the time the number of voices is requested.
     */
    int getNumberOfActiveVoices(timeT time);

    /**
     * Sets the number of instruments on this staff. The default is
     * 1 instrument. However, on percussion parts multiple instruments
     * might be used.
     */
    void setInstrumentCount(int count);

    /**
     * When true the <octave-shift> element will be used in the
     * transpose attribute.
     */
    void setUseOctaveShift(bool use) {m_useOctaveShift = use;}

    /**
     * Write all events between startTime and endTime to the MusicXML file.
     **/
    void writeEvents(int bar, std::ostream &str);

    /**
     * Handle the events.
     **/
    void handleEvent(Segment *segment, Event &event);


    void printSummary();

protected:
    /**
     * Creates the <time> element. This member is called every time
     * a new time signature changes.
     */
    void addTimeSignature(timeT time, const TimeSignature &ts);

    /**
     * Creates the <transpose> element and is called for each new
     * segment.
     * Please note it is supposed every time all segments of a track  will
     * have the same transpostion. However, this is not checked!
     */
    void addTransposition(timeT time, int transpose);

    /**
     * Implements the Key events and creates the <key> element.
     */
    void addKey(const Event &event);

    /**
     * Implements the Clef event and creates the <clef> element.
     */
    void addClef(const Event &event);

    /**
     * Handles the Text::Dynamic events and creates a <dynamics>
     * element.
     */
    void addDynamic(const Event &event);

    /**
     * Handles the Text::Direction, Text::LocalDirection, Text::Tempo and
     * the Text::LocalTempo events. It use the same font weight, style and
     * size as Lilypond does.
     * For now the sizes are fixed.
     */
    void addDirection(const Event &event);

    /**
     * Handles the Text::Chord event and create a <harmony> element. It
     * tries to parse the chord text to a suiteble harmony.
     */
    void addChord(const Event &event);

    /**
     * Handles the Indication::Slur and Indication::PhrasingSlur events.
     */
    void addSlur(const Event &event, bool dashed);

    /**
     * Handles the Indication::TrillLine event.
     */
    void addTrillLine(const Event &event);

    /**
     * Handles the Indication::Glissando event.
     */
    void addGlissando(const Event &event);

    /**
     * Handles the Indication::Crescendo and Indication::Decrescendo events.
     */
    void addWedge(const Event &event, bool crescendo);

    /**
     * Handles the Indication::QuindicesimaUp, Indication::OttavaUp,
     * Indication::OttavaDown and Indication::QuindicesimaDown events.
     */
    void addOctaveShift(const Event &event);

    /**
     * Handles the Text::Lyric event. Since lyrics are part of the <note>
     * element, created by addNote(), addLyric() creates the necessary
     * MusicXML code and stores it to be used by addNote().
     */
    void addLyric(const Event &event);

    /**
     * Handles the Note::EventType and Note::EventRestType and creates
     * the <note> element.
     */
    void addNote(const Segment &segment, const Event &event);

    /**
     * All addXxx() members, handling several events, stores all MusicXML
     * code in string. This member will write all collect strings in the
     * correct order to the ouput file.
     */
    void flush(std::ostream &str);

    /**
     * Converts the Rosegarden notetype to MusicXML notenames.
     */
    std::string getNoteName(int noteType) const;

    /**
     * Queue and retrieve delayed events.
     */
    void queue(bool direction, timeT time, std::string str);
    std::string retrieve(bool direction, timeT time);

    /**
     * Scan the surrounding of the event to see it is part of a tuplet or
     * beamgroup.
     */
    void updatePart(Segment *segment, Event &event);

    /**
     * Add a temporary segment to the composition.
     */
    void addTemporarySegment(Segment *segment, int staff, int voice, int &count)  ;

    /**
     * Creates new segment, filled with rests to fill the gap between
     * two segments of the first voice of a staff.
     */
    void generateRestSegment(int staff, timeT begin, timeT end, int voice, int &count);

    /**
     * Returns the number of the next available slur. MusicXML allows
     * 6 parallel slurs at the same time. If the number of parallel
     * slurs exceed this value, -1 is returned.
     */
    int  getSlurNumber(const Indication &indication)
    {
        timeT endSlur = m_curtime + indication.getIndicationDuration();
        int number = -1;
        for (int i = 0; i < MAXSLURS; i++) {
            if (m_slurEndTimes[i] < m_curtime)
                m_slurEndTimes[i] = -1;
            if ((number < 0) && (m_slurEndTimes[i] < 0)) {
                number = i;
                m_slurEndTimes[i] = endSlur;
            }
        }
        return number+1;
    }

    /**
     * The note duration as create by the Percussion Matrix Editor are note
     * the durations are expected in the Notation Editor. This member tries
     * to create durations which should be me more usual in percussion
     * sheets.
     * Please note this part is still experimental and doesn't support all
     * kinds of more complex rithms (like tuplets!).
     */
    void quantizePercussion();
    bool emptyQuantizeQueue(PercussionMap &pm, Segment *segment,
                            std::vector<Event *> &events,
                            timeT begin, timeT end, bool stem);

    /**
     * Returns true is selectedSegmentsOnly is true and the segment is not
     * selected.
     */
    bool skipSegment(Segment *segment, bool selectedSegmentsOnly);

    Composition     *m_composition;
    RosegardenMainViewWidget *m_view;

    std::string     m_partName;
    bool            m_percussionTrack;
    int             m_curVoice;
    long            m_group;
    std::string     m_tupletGroup;
    long            m_actualNotes;
    long            m_normalNotes;
    int             m_prvbeam;
    int             m_curbeam;
    int             m_nxtbeam;

    int             m_staff;
    StaffMap        m_staves;
    VoiceMap        m_voices;
    timeT           m_curtime;

    int             instrumentCount;

    bool            m_pendingAttributes;
    timeT           m_attributesTime;
    std::string     m_strDivisions;
    std::string     m_strKey;
    std::string     m_strTimesignature;
    std::string     m_strStaves;
    std::string     m_strClef;
    std::string     m_strStaffDetails;
    std::string     m_strTranspose;
    bool            m_useOctaveShift;

    bool            m_pendingNote;
    std::string     m_strNote;
    std::string     m_strSlurs;
    std::string     m_strLyrics;
    timeT           m_slurEndTimes[MAXSLURS];
    std::map<int, std::string>  m_syllabic;

    bool            m_pendingDirections;
    timeT           m_directionTime;
    std::string     m_strDirection;

    std::vector<SimpleQueue> tmp_queue;

    std::vector<Segment *> m_restSegments;

    AccidentalTable::OctaveType m_octaveType;
    AccidentalTable::BarResetType m_barResetType;
};

}
#endif
