/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    More or less complete rewrite (Sep 2011)
        Niek van den Berg   <niekjvandenberg@gmail.com>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MusicXmlExportHelper.h"
// #include "PercussionMap.h"

#include "base/BaseProperties.h"
#include "gui/editors/notation/NotationProperties.h"


#include <sstream>

namespace Rosegarden
{

using namespace BaseProperties;

MusicXmlExportHelper::MusicXmlExportHelper(const std::string &name,
                                     const TrackVector &tracklist,
                                     bool percussion,
                                     bool selectedSegmentsOnly,
                                     timeT compositionEndTime,
                                     Composition *composition,
                                     RosegardenMainViewWidget *view,
                                     AccidentalTable::OctaveType octaveType,
                                     AccidentalTable::BarResetType barResetType) :
        m_composition(composition),
        m_view(view),
        m_partName(name),
        m_percussionTrack(percussion),
        m_octaveType(octaveType),
        m_barResetType(barResetType)
{
    instrumentCount = 1;

    // Build a StafInfo for each track. First collects all segments which
    // form the track and keep track of the start and end time of track.
    m_staff = 0;
    std::vector<Segment *> allSegments;
    std::map<TrackId, int> mainvoice;
    for (TrackVector::const_iterator t = tracklist.begin(); t != tracklist.end(); ++t) {
        StaffInfo si(*t);
        mainvoice[si.trackId] = 0; // no mainvoice.
        bool first = true;
        for (Composition::iterator s = m_composition->begin();
             s != m_composition->end(); ++s) {
            if ((*s)->getTrack() == si.trackId) {
                m_voices[(*s)] = 0; // Segment not yet assigned to a voice.
                if (!skipSegment(*s, selectedSegmentsOnly)) {
                    allSegments.push_back((*s));
                    si.segments.push_back((*s));
                }
                if (first) {
                    si.startTime = (*s)->getStartTime();
                    si.endTime = (*s)->getEndMarkerTime();
                    first = false;
                } else {
                    if ((*s)->getEndMarkerTime() > si.endTime) {
                        si.endTime = (*s)->getEndMarkerTime();
                    }
                }
            }
        }
        m_staves[m_staff++] = si;
    }

    // Now all segments must be assigned to voices. These voices represent
    // parallel segments. The mainvoice is the "upper" segment line in the
    // track editor. This mainvoice must consist of a list of joint segments.
    // If there is a gap between two consecutive segments a temporary segment
    // with rests will be generated later.
    int nvoices = 0;
    std::map<int, timeT> endTimes;
    for (std::vector<Segment *>::iterator s = allSegments.begin();
         s != allSegments.end(); ++s) {

        // Count the number of parallel segments.
        int voice = 1;
        while ((voice <= nvoices) && ((*s)->getStartTime() < endTimes[voice])) {
            voice++;
        }
        endTimes[voice] = (*s)->getEndMarkerTime();

        // New voice created?
        nvoices = nvoices < voice ? voice : nvoices;

        // Keep track of the voice of every segment. This is needed when writing
        // the events to the MusicXML file.
        m_voices[(*s)] = voice;

        // If the track to which the segment belong has no mainvoice already
        // voice will become the main voice.
        if (mainvoice[(*s)->getTrack()] == 0) mainvoice[(*s)->getTrack()] = voice;
    }

    // Now we can finish defining the StaffInfo by setting the firstVoice and
    // lastVoice and filling the gap in the mainvoice segment list.
    int generated = 0;
    for (int i = 0; i < getStaffCount(); i++) {
        timeT lastEnd = 0;

        m_staves[i].firstVoice = mainvoice[m_staves[i].trackId];
        m_staves[i].lastVoice = mainvoice[m_staves[i].trackId];

        // The segment vector is not necessarily a consecutive stream of
        // segments. However the algorithme relies on such consecutive list so
        // fill the gap with (rest filled) temporary segments. These temporary
        // segments will be deleted in the destructor.
        std::vector<Segment *> trackSegments = m_staves[i].segments;
        m_staves[i].segments.clear();
        bool repeating = false;
        Segment *repeatingSegment;
        for (std::vector<Segment *>::iterator s = trackSegments.begin();
             s != trackSegments.end(); ++s) {

            // Keep track of the last voice of the track.
            if (m_voices[(*s)] > m_staves[i].lastVoice) {
                m_staves[i].lastVoice = m_voices[(*s)];
            }

            if (repeating) {
                if (m_voices[repeatingSegment] != m_staves[i].firstVoice) {
                    generateRestSegment(i, lastEnd, (*s)->getStartTime(), m_voices[(*s)], generated);
                }
                timeT endTime = repeatingSegment->getEndMarkerTime();
                timeT duration = repeatingSegment->getEndMarkerTime() - repeatingSegment->getStartTime();
                while ((endTime + duration) <= (*s)->getStartTime()) {
                    Segment *tmp = repeatingSegment->clone();
                    tmp->setRepeating(false);
                    tmp->setStartTime(endTime);
                    endTime = tmp->getEndMarkerTime();
                    addTemporarySegment(tmp, i, m_voices[repeatingSegment], generated);
                    m_staves[i].segments.push_back(tmp);
                }
                repeating = false;
                lastEnd = endTime;
                if (m_voices[repeatingSegment] != m_staves[i].firstVoice) {
                    generateRestSegment(i, lastEnd, (*s)->getStartTime(), m_voices[repeatingSegment], generated);
                }
                lastEnd = (*s)->getStartTime();
            }

            // If necessary create a temporary segment and place it before
            // the segment.
            if ((*s)->isRepeating()) {
                repeating = true;
                repeatingSegment = (*s);
            }

            // If the segment is not part of the mainvoice gap are allowed.
            // So push the segment back and continue with the next segment.
            if (m_voices[(*s)] != mainvoice[m_staves[i].trackId]) {
                m_staves[i].segments.push_back((*s));
                continue;
            }

            if (! repeating) {
                // Do not generate rests if this a repeating segment!
                generateRestSegment(i, lastEnd, (*s)->getStartTime(), m_voices[(*s)], generated);
            }
            m_staves[i].segments.push_back((*s));
            lastEnd = (*s)->getEndMarkerTime();
        }
        // Generate rests to the end of the composition.
        generateRestSegment(i, lastEnd, compositionEndTime, m_staves[i].firstVoice, generated);
    }
    // Now we have per track a nice vector of segments, sorted by startTime which is
    // consecutive from the start to the end of the composition.

    // If the track is a percussion track, try to convert the events to "normal"
    // notes.
    if (m_percussionTrack) {
        quantizePercussion();
    }

    // Now initialise some running variables.
    m_staff = 0;

    m_curtime = 0;
    m_syllabic.clear();
    m_tupletGroup = "none";
    m_group       = -1;
    m_actualNotes = 1;
    m_normalNotes = 1;
    m_prvbeam     = -1;
    m_curbeam     = -1;
    m_nxtbeam     = -1;
    for (int i = 0; i < MAXSLURS; i++) m_slurEndTimes[i] = -1;

    // Attribute variables.
    m_pendingAttributes = true;
    m_pendingNote = false;
    m_pendingDirections = false;
    m_attributesTime = 0;
    m_directionTime = 0;

    std::stringstream tmp;
    tmp << "        <divisions>" << divisions << "</divisions>\n";
    m_strDivisions = tmp.str();
    if (isMultiStave()) {
        tmp.str("");
        tmp << "        <staves>" << getStaffCount() << "</staves>\n";
        m_strStaves = tmp.str();
    } else {
        m_strStaves = "";
    }
    m_strTimesignature = "";
    m_strKey = "";
    m_strClef = "";
    m_strStaffDetails = "";
    m_strTranspose = "";

    // Note variables.
    m_strSlurs = "";
    m_strLyrics = "";
}

MusicXmlExportHelper::~MusicXmlExportHelper()
{
    for (std::vector<Segment *>::iterator s = m_restSegments.begin();
         s != m_restSegments.end(); ++s)
        m_composition->deleteSegment(*s);
    m_restSegments.clear();
}

void
MusicXmlExportHelper::addTemporarySegment(Segment *segment, int staff, int voice, int &count)
{
    std::stringstream label;
    label << "G" << m_staves[staff].trackId << "/" << count++;
    segment->setTrack(m_staves[staff].trackId);
    segment->setLabel(label.str());
    m_composition->addSegment(segment);
    m_voices[segment] = voice;
    m_restSegments.push_back(segment);
}

void
MusicXmlExportHelper::generateRestSegment(int staff, timeT begin, timeT end, int voice, int &count)
{
    if (begin < end) {
        std::stringstream label;
        label << "G" << m_staves[staff].trackId << "/" << count++;
        Segment *tmp = new Segment();
        tmp->setTrack(m_staves[staff].trackId);
        tmp->setLabel(label.str());
        m_composition->addSegment(tmp);
        tmp->fillWithRests(begin, end);
        m_staves[staff].segments.push_back(tmp);
        m_restSegments.push_back(tmp);
        m_voices[tmp] = voice;
    }
}

void
MusicXmlExportHelper::quantizePercussion()
{
    PercussionMap pm;
    pm.loadDefaultPercussionMap();

    int voice = 1;
    for (int i = 0; i < getStaffCount(); i++) {
        // Since we have to split up the events over two layer
        // usr defined layers are not allowed.
        if (m_staves[i].firstVoice != m_staves[i].lastVoice) {
            std::cerr << "MusicXmlExportHelper::quantizePercussion: can not handle multi layer tracks.\n";
            continue;
        }

        std::vector<Segment *> segments = m_staves[i].segments;
        m_staves[i].segments.clear();

        m_voices.clear();

        bool stem = true;
        for (int tmpvoice = 1; tmpvoice <= 2; tmpvoice++) {
            Segment *segment = new Segment();
            Event *tmp = Clef().getAsEvent(0);
            segment->insert(tmp);

            segment->setTrack(m_staves[i].trackId);
            m_composition->addSegment(segment);
            m_restSegments.push_back(segment);

            timeT lastEnd = m_staves[i].startTime;
            std::vector<Event *> events;
            bool empty = true;
            for (std::vector<Segment *>::iterator s = segments.begin();
                s != segments.end(); ++s) {
                for (Segment::iterator e = (*s)->begin(); e != (*s)->end(); ++e) {
                    int pp;
                    if ((*e)->isa(Rosegarden::Note::EventType)) {
                        pp = (*e)->get<Int>(BaseProperties::PITCH);
                        if (pm.getVoice(pp) != tmpvoice) continue;
                    } else if ((*e)->isa(Rosegarden::Note::EventRestType)) {
                        continue;
                    } else {
                        continue;
                    }

                    if (lastEnd < (*e)->getNotationAbsoluteTime()) {
                        if (emptyQuantizeQueue(pm, segment, events, lastEnd,
                                           (*e)->getNotationAbsoluteTime(), stem)) {
                            empty = false;
                        }
                        lastEnd = (*e)->getNotationAbsoluteTime();
                    }
                    events.push_back(*e);
                }
            }
            if (emptyQuantizeQueue(pm, segment, events, lastEnd,
                m_staves[i].endTime, stem)) {
                empty = false;
            }

            if (!empty) {
                m_staves[i].segments.push_back(segment);
                m_voices[segment] = voice;
                if (stem) m_staves[i].firstVoice = voice;
                m_staves[i].lastVoice = voice;
                if (stem) voice++;
                stem = false;
            }
        }
    }
}

bool
MusicXmlExportHelper::emptyQuantizeQueue(PercussionMap &pm,
                                               Segment *segment,
                                               std::vector<Event *> &events,
                                               timeT begin, timeT end, bool stem)
{
    TimeSignature ts = m_composition->getTimeSignatureAt(begin);
    timeT beat = ts.getBeatDuration();

    timeT duration = end - begin;
    if (duration >= beat) {
        timeT end = ((begin + beat)/beat)*beat;
        duration = end - begin;
    }
    Note note = Note::getNearestNote(duration);
    bool empty = true;
    for (std::vector<Event *>::iterator v = events.begin();
         v != events.end(); ++v) {
        int pp = (*v)->get<Int>(BaseProperties::PITCH);
        timeT t = (*v)->getNotationAbsoluteTime();
        Event *tmp = new Event(*(*v), t, duration);
        tmp->set<Int>("MxmlPitch", pm.getPitch(pp));
        tmp->set<String>("MxmlNoteHead", pm.getNoteHead(pp));
        tmp->set<Bool>(NotationProperties::BEAM_ABOVE, stem);
        segment->insert(tmp);
        empty = false;
    }
    events.clear();
    if (empty) duration = 0;
    if (begin + duration < end) {
        segment->fillWithRests(begin + duration, end);
    }
    return ! empty;
}

int
MusicXmlExportHelper::getNumberOfActiveVoices(timeT time)
{
    int activeVoices = 0;
    for (int v = m_staves[m_staff].firstVoice; v < m_staves[m_staff].lastVoice; v++) {
        for (std::vector<Segment *>::iterator s = m_staves[m_staff].segments.begin();
             s != m_staves[m_staff].segments.end(); ++s) {
            if ((*s)->getStartTime() > time) break;
            if ((*s)->getEndMarkerTime() <= time) continue;
            activeVoices++;
        }
    }
    return activeVoices;
}

void
MusicXmlExportHelper::setInstrumentCount(int count)
{
    instrumentCount = count;
    if ((count == 1) && m_percussionTrack) {
        m_strStaffDetails = "        <staff-details>\n"
                          "          <staff-lines>1</staff-lines>\n"
                          "        </staff-details>\n";
        m_pendingAttributes = true;
        m_attributesTime = 0;
    } else {
        m_strStaffDetails = "";
    }
}

bool
MusicXmlExportHelper::skipSegment(Segment *segment, bool selectedSegmentsOnly)
{
    if (selectedSegmentsOnly) {
        bool segmentSelected = true;
        if ((m_view != NULL) && (m_view->haveSelection())) {
            //
            // Check whether the current segment is in the list of selected segments.
            //
            SegmentSelection selection = m_view->getSelection();
            for (SegmentSelection::iterator it = selection.begin(); it != selection.end(); ++it) {
                if ((*it) == segment) {
                    segmentSelected = false;
                    break;
                }
            }
        }
        return segmentSelected;
    } else {
        return false;
    }
}

void
MusicXmlExportHelper::writeEvents(int bar, std::ostream &str)
{
    timeT startTime = m_composition->getBarStart(bar);
    timeT endTime = m_composition->getBarEnd(bar);

    bool newTimeSignature = false;
    TimeSignature ts = m_composition->getTimeSignatureInBar(bar, newTimeSignature);
    if (newTimeSignature) addTimeSignature(startTime, ts);

    for (m_staff = 0; m_staff < getStaffCount(); m_staff++) {
        m_staves[m_staff].accTable.newBar();
        if ((m_staff > 0) && ((m_curtime - startTime) > 0)) {
            std::stringstream tmp;
            tmp << "      <backup>\n";
            tmp << "        <duration>" << m_curtime - startTime << "</duration>\n";
            tmp << "      </backup>\n";
            str << tmp.str();
        }
        m_curtime = startTime;
        for (m_curVoice = m_staves[m_staff].firstVoice;
             m_curVoice <= m_staves[m_staff].lastVoice; m_curVoice++) {
            for (std::vector<Segment *>::iterator s = m_staves[m_staff].segments.begin();
                s != m_staves[m_staff].segments.end(); ++s) {
                if (m_voices[(*s)] != m_curVoice) continue;
                if ((*s)->getEndMarkerTime() < startTime) continue;
                if ((*s)->getStartTime() > endTime) break;
                if (((*s)->getStartTime() >= startTime) &&
                    ((*s)->getStartTime() < endTime)) {
                    int transpose = (*s)->getTranspose();
                    if (transpose != 0) {
                        addTransposition((*s)->getStartTime(), transpose);
                    }
                }
                if (m_curVoice > m_staves[m_staff].firstVoice) {
                    timeT t = (*s)->getStartTime() > startTime
                              ? (*s)->getStartTime() : startTime;
                    if ((m_curtime - t) > 0) {
                        std::stringstream tmp;
                        tmp << "      <backup>\n";
                        tmp << "        <duration>" << m_curtime - t << "</duration>\n";
                        tmp << "      </backup>\n";
                        str << tmp.str();
                    }
                    m_curtime = startTime;
                }
                timeT end = endTime < (*s)->getEndMarkerTime()
                            ? endTime : (*s)->getEndMarkerTime();
                for (Segment::iterator e = (*s)->findTime(startTime);
                     e != (*s)->findTime(end); ++e) {
                    handleEvent(*s, **e);
                    flush(str);
                }
            }
        }
    }
}

void
MusicXmlExportHelper::handleEvent(Segment *segment, Event &event)
{
    if (event.isa(Rosegarden::Key::EventType)) {
        addKey(event);
    } else if (event.isa(Rosegarden::Clef::EventType)) {
        addClef(event);
    } else if (event.isa(Rosegarden::Text::EventType)) {
        Text text(event);
        if (text.getTextType() == Text::Dynamic) {
            addDynamic(event);
        } else if (text.getTextType() == Text::Lyric) {
            addLyric(event);
        } else if (text.getTextType() == Text::Direction) {
            addDirection(event);
        } else if (text.getTextType() == Text::LocalDirection) {
            addDirection(event);
        } else if (text.getTextType() == Text::Tempo) {
            addDirection(event);
        } else if (text.getTextType() == Text::LocalTempo) {
            addDirection(event);
        } else if (text.getTextType() == Text::Chord) {
            addChord(event);
        } else {
            std::cerr << "WARNING: MusicXmlExportHelper::handleEvent: unsupported TextEvent \""
                    << text.getTextType() << "\"." << std::endl;
        }
    } else if (event.isa(Rosegarden::Indication::EventType)) {
        Indication indication(event);
        if (indication.getIndicationType() == Indication::Slur) {
            addSlur(event, false);
        } else if (indication.getIndicationType() == Indication::PhrasingSlur) {
            addSlur(event, true);
        } else if (indication.getIndicationType() == Indication::Crescendo) {
            addWedge(event, true);
        } else if (indication.getIndicationType() == Indication::Decrescendo) {
            addWedge(event, false);
        } else if (indication.getIndicationType() == Indication::QuindicesimaUp) {
            addOctaveShift(event);
        } else if (indication.getIndicationType() == Indication::OttavaUp) {
            addOctaveShift(event);
        } else if (indication.getIndicationType() == Indication::OttavaDown) {
            addOctaveShift(event);
        } else if (indication.getIndicationType() == Indication::QuindicesimaDown) {
            addOctaveShift(event);
        } else if (indication.getIndicationType() == Indication::TrillLine) {
            addTrillLine(event);
        } else if (indication.getIndicationType() == Indication::Glissando) {
            addGlissando(event);
        } else  {
            std::cerr << "WARNING: MusicXmlExportHelper::handleEvent: unsupported IndicationEvent \""
                    << indication.getIndicationType() << "\"." << std::endl;
        }
    } else if (event.isa(Rosegarden::Note::EventRestType) ||
            (event.isa(Rosegarden::Note::EventType))) {
        updatePart(segment, event);
        addNote(*segment, event);
    } else {
        std::cerr << "WARNING: MusicXmlExportHelper::handleEvent: Unknown EventType \""
                << event.getType() << "\"." << std::endl;
    }
}

void
MusicXmlExportHelper::printSummary()
{
    std::cerr << "Part " << m_partName << " : m_staves = " << m_staves.size();
    if (m_percussionTrack) std::cerr << " (percussion, " << instrumentCount << ")";
    std::cerr << std::endl;
    if (getStaffCount() == 0) std::cerr << "  No staves found.\n";
    else {
        for (int i = 0; i < getStaffCount(); i++) {
            Track *track = m_composition->getTrackById(m_staves[i].trackId);
            std::cerr << "  Staff " << i+1 << " (track \""
                      << track->getLabel() << "\")   "
                      << m_staves[i].startTime << " - "
                      << m_staves[i].endTime
                      << "   voices " << m_staves[i].firstVoice << " - "
                      << m_staves[i].lastVoice << std::endl;
            for (std::vector<Segment *>::iterator s = m_staves[i].segments.begin();
                 s != m_staves[i].segments.end(); ++s) {
                Segment *segment = *s;
                std::cerr << "     " << m_voices[segment] << " : \""
                          << segment->getLabel() << "\"   "
                          << segment->getStartTime() << " <> "
                          << segment->getEndMarkerTime() << std::endl;
            }
        }
    }
}

void
MusicXmlExportHelper::addTimeSignature(timeT time, const TimeSignature &ts)
{
    std::stringstream tmp;
    if (ts.getDenominator() == 2 && ts.isCommon()) {
        tmp << "        <time symbol=\"cut\">";
    } else if (ts.getDenominator() == 4 && ts.isCommon()) {
        tmp << "        <time symbol=\"common\">\n";
    } else {
        tmp << "        <time>\n";
    }
    tmp << "          <beats>" << ts.getNumerator() << "</beats>\n";
    tmp << "          <beat-type>" << ts.getDenominator() << "</beat-type>\n";
    tmp << "        </time>\n";
    m_strTimesignature = tmp.str();
    m_pendingAttributes = true;
    m_attributesTime = time;
}

void
MusicXmlExportHelper::addTransposition(timeT time, int transpose)
{
    if (transpose != 0) {
        std::stringstream tmp;
        int octave = transpose/12;
        transpose = transpose < 0 ? transpose%-12 : transpose%12;
        int diatonic = transpose < 0 ? (transpose-1)/2 : (transpose+1)/2;
        if (!m_useOctaveShift) {
            diatonic += octave*7;
            transpose += octave*12;
            octave = 0;
        }
        tmp << "        <transpose>\n";
        tmp << "          <diatonic>" << diatonic << "</diatonic>\n";
        tmp << "          <chromatic>" << transpose << "</chromatic>\n";
        if (octave != 0) {
            tmp << "          <octave-change>" << octave << "</octave-change>\n";
        }
        tmp << "        </transpose>\n";
        m_strTranspose = tmp.str();
        m_pendingAttributes = true;
        m_attributesTime = time;
    } else {
        m_strTranspose = "";
    }
}

void
MusicXmlExportHelper::addKey(const Event &event)
{
    // Keys have no meaning for percussion tracks.
    if (m_percussionTrack) return;

    Key key(event);

    std::stringstream tmp;
    tmp << "        <key>\n";
    tmp << "          <fifths>" << (key.isSharp() ? "" : "-")
                                << (key.getAccidentalCount()) << "</fifths>\n";
    tmp << "          <mode>" << (key.isMinor() ? "minor" : "major")
                              << "</mode>\n";
    tmp << "        </key>\n";
    m_strKey = tmp.str();
    m_pendingAttributes = true;
    m_attributesTime = event.getNotationAbsoluteTime();

    for (StaffMap::iterator i = m_staves.begin(); i != m_staves.end(); ++i) {
        (*i).second.key = key;
       (*i).second.accTable = AccidentalTable(key, (*i).second.clef,
                                              m_octaveType, m_barResetType);
    }
}

void
MusicXmlExportHelper::addClef(const Event &event)
{
    Clef clef(event);

    if (clef == m_staves[m_staff].clef) return;

    std::stringstream tmp;
    tmp << "        <clef";
    if (isMultiStave()) tmp << " number=\"" << m_staff+1 << "\"";
    tmp << ">\n";
    if (m_percussionTrack) {
        tmp << "          <sign>percussion</sign>\n"
            << "          <line>2</line>\n";
    } else {
        if (clef.getClefType() == Clef::Treble ||
            clef.getClefType() == Clef::French) {
            tmp << "          <sign>G</sign>\n";
        } else if (clef.getClefType() == Clef::Bass ||
                clef.getClefType() == Clef::Subbass ||
                clef.getClefType() == Clef::Varbaritone) {
            tmp << "          <sign>F</sign>\n";
        } else if (clef.getClefType() == Clef::Soprano ||
                clef.getClefType() == Clef::Mezzosoprano ||
                clef.getClefType() == Clef::Alto ||
                clef.getClefType() == Clef::Tenor ||
                clef.getClefType() == Clef::Baritone) {
            tmp << "          <sign>C</sign>\n";
        } else
            std::cerr << "WARNING: MusicXmlExportHelper::addClef: bad clef \""
                    <<  clef.getClefType() << "\"." << std::endl;
        tmp << "          <line>" << clef.getAxisHeight()/2+1 << "</line>\n";
        if (clef.getOctaveOffset() != 0) {
            tmp << "          <clef-octave-change>" << clef.getOctaveOffset()
                                                    << "</clef-octave-change>\n";
        }
    }
    tmp << "        </clef>\n";
    m_strClef = tmp.str();

    m_pendingAttributes = true;
    m_attributesTime = event.getNotationAbsoluteTime();

    m_staves[m_staff].clef = clef;
    m_staves[m_staff].accTable.newClef(m_staves[m_staff].clef);
}

void
MusicXmlExportHelper::addDynamic(const Event &event)
{
    Text text(event);
    static const char *dynamics[] = {
        "pppppp", "ppppp", "pppp", "ppp", "pp", "p",
        "ffffff", "fffff", "ffff", "fff", "ff", "f",
        "mp", "mf", "sf", "sfp", "sfpp", "fp", "rf", "rfz", "sfz", "sffz", "fz"
    };

    int d = 0;
    while((d < int(sizeof(dynamics) / sizeof(dynamics[0]))) &&
          (text.getText() != dynamics[d])) d++;

    std::stringstream tmp;
    tmp << "      <direction placement=\"below\">\n";
    tmp << "        <direction-type>\n";
    tmp << "          <dynamics>\n";
    if (d < int(sizeof(dynamics) / sizeof(dynamics[0]))) {
        tmp << "            <" << text.getText() << "/>\n";
    } else {
        tmp << "            <other-dynamics>\n";
        tmp << "              " << text.getText() << "\n";
        tmp << "            </other-dynamics>\n";
    }
    tmp << "          </dynamics>\n";
    tmp << "        </direction-type>\n";
    tmp << "      </direction>\n";

    m_strDirection += tmp.str();
    m_pendingDirections = true;
    m_directionTime = event.getNotationAbsoluteTime();
}

void
MusicXmlExportHelper::addDirection(const Event &event)
{
    //  * UnspecifiedType:    Nothing known, use small roman
    //  * StaffName:          Large roman, to left of start of staff
    //  * ChordName:          Not normally shown in score, use small roman
    //  * KeyName:            Not normally shown in score, use small roman
    //  * Lyric:              Small roman, below staff and dynamic texts
    //  * Chord:              Small bold roman, above staff
    //  * Dynamic:            Small italic, below staff
    //  * Direction:          Large roman, above staff (by barline?)
    //  * LocalDirection:     Small bold italic, below staff (by barline?)
    //  * Tempo:              Large bold roman, above staff
    //  * LocalTempo:         Small bold roman, above staff
    //  * Annotation:         Very small sans-serif, in a yellow box
    //  * LilyPondDirective:  Very small sans-serif, in a green box
    Text text(event);

    std::string italic = "";
    std::string bold = "";
    std::string size = "";
    std:: string placement = " placement=\"above\"";
    if (text.getTextType() == Text::Direction) {
        size = " font-size=\"7.9\"";
    } else if (text.getTextType() == Text::LocalDirection) {
        placement = " placement=\"below\"";
        italic = " font-style=\"italic\"";
        size = " font-size=\"6.3\"";
        bold = " font-weight=\"bold\"";
    } else if (text.getTextType() == Text::Tempo) {
        size = " font-size=\"7.9\"";
        bold = " font-weight=\"bold\"";
    } else if (text.getTextType() == Text::LocalTempo) {
        size = " font-size=\"6.3\"";
        bold = " font-weight=\"bold\"";
    }

    std::stringstream tmp;
    tmp << "      <direction" << placement << ">\n";
    tmp << "        <direction-type>\n";
    tmp << "          <words" << size << bold << italic << ">"
                              << text.getText() << "</words>\n";
    tmp << "        </direction-type>\n";
    tmp << "      </direction>\n";

    m_strDirection += tmp.str();
    m_pendingDirections = true;
    m_directionTime = event.getNotationAbsoluteTime();
}

void
MusicXmlExportHelper::addChord(const Event &event)
{
    Text text(event);

    // Since Rosegarden doesn't know "real" chords and chords are in fact
    // free format text strings a very simple algorithme is used.
    // Just a limited subset is implemented:
    //      C
    //      Cm
    //      CM
    //      Cdim
    //      Caug
    //      C7
    //      Cm7
    //      CM7
    //      Cdim7
    //      Caug7

    QString txt = QString::fromStdString(text.getText()).trimmed();
    QRegExp rx("([A-G])([#b])?(m|M|dim|aug)?(7)?");
    rx.indexIn(txt, 0);

    std::string kind;
    if (rx.cap(4) == "") {
        if (rx.cap(3) == "") kind = "major";
        else if (rx.cap(3) == "m") kind = "minor";
        else if (rx.cap(3) == "dim") kind = "diminished";
        else if (rx.cap(3) == "aug") kind = "augmented";
    } else if (rx.cap(4) == "7") {
        if (rx.cap(3) == "") kind = "dominant";
        else if (rx.cap(3) == "M") kind = "major-seventh";
        else if (rx.cap(3) == "m") kind = "minor-seventh";
        else if (rx.cap(3) == "dim") kind = "diminished-seventh";
    }

    if ((rx.cap(1) == "") || (kind == "")) {
        std::cerr << "WARNING: MusicXmlExportHelper::addChord: bad chord \""
                  << text.getText() << "\"." << std::endl;
    } else {
        std::stringstream tmp;
        tmp << "      <harmony>\n";
        tmp << "        <root>\n";
        tmp << "          <root-step>" << rx.cap(1) << "</root-step>\n";
        if (rx.cap(2) == "b") {
            tmp << "          <root-alter>-1</root-alter>\n";
        } else if (rx.cap(2) == "#") {
            tmp << "          <root-alter>1</root-alter>\n";
        }
        tmp << "        </root>\n";
        tmp << "        <kind>" << kind << "</kind>\n";
        tmp << "      </harmony>\n";

        m_strDirection += tmp.str();
        m_pendingDirections = true;
        m_directionTime = event.getNotationAbsoluteTime();
    }
}

void
MusicXmlExportHelper::addSlur(const Event &event, bool dashed)
{
    Indication indication(event);

    int number = getSlurNumber(indication);
    if (number == 0) return;

    std::stringstream tmp;
    tmp << "          <slur type=\"start\" number=\"" << number << "\"";
    if (dashed) tmp << " line-type=\"dashed\"";
    tmp << "/>\n";
    m_strSlurs += tmp.str();

    tmp.str("");
    tmp << "          <slur type=\"stop\" number=\"" << number << "\"/>\n";
    queue(false, event.getNotationAbsoluteTime()+indication.getIndicationDuration(),
          tmp.str());
}

void
MusicXmlExportHelper::addTrillLine(const Event &event)
{
    Indication indication(event);

    std::stringstream tmp;
    tmp << "          <ornaments>\n";
    tmp << "            <trill-mark/>\n";
    tmp << "            <wavy-line type=\"start\" number=\"1\"/>\n";
    tmp << "          </ornaments>\n";
    m_strSlurs += tmp.str();

    tmp.str("");
    tmp << "          <ornaments>\n";
    tmp << "            <wavy-line type=\"stop\" number=\"1\"/>\n";
    tmp << "          </ornaments>\n";
    queue(false, event.getNotationAbsoluteTime()+indication.getIndicationDuration(),
          tmp.str());
}

void
MusicXmlExportHelper::addGlissando(const Event &event)
{
    Indication indication(event);

    std::stringstream tmp;
    tmp << "          <glissando type=\"start\" number=\"1\"/>\n";
    m_strSlurs += tmp.str();

    tmp.str("");
    tmp << "          <glissando type=\"stop\" number=\"1\"/>\n";
    queue(false, event.getNotationAbsoluteTime()+indication.getIndicationDuration(),
          tmp.str());
}

void
MusicXmlExportHelper::addWedge(const Event &event, bool crescendo)
{
    Indication indication(event);
    timeT time = event.getNotationAbsoluteTime();

    std::stringstream tmp;
    tmp << "      <direction placement=\"below\">\n";
    tmp << "        <direction-type>\n";
    tmp << "          <wedge type=\"" << (crescendo ? "crescendo" : "diminuendo" )
                                      << "\" number=\"1\"/>\n";
    tmp << "        </direction-type>\n";
    if (isMultiStave()) {
        tmp << "        <staff>" << m_staff + 1 << "</staff>\n";
    }
    tmp << "      </direction>\n";

    m_strDirection += tmp.str();
    m_pendingDirections = true;
    m_directionTime = time;

    tmp.str("");
    tmp << "      <direction>\n";
    tmp << "        <direction-type>\n";
    tmp << "          <wedge type=\"stop\" number=\"1\"/>\n";
    tmp << "        </direction-type>\n";
    if (isMultiStave()) {
        tmp << "        <staff>" << m_staff + 1 << "</staff>\n";
    }
    tmp << "      </direction>\n";
    queue(true, time+indication.getIndicationDuration(), tmp.str());
}

void
MusicXmlExportHelper::addOctaveShift(const Event &event)
{
    Indication indication(event);
    timeT time = event.getNotationAbsoluteTime();

    int size = 0;
    std::string updown = "" ;
    switch (indication.getOttavaShift()) {

    case -2 :
        size = 15;
        updown = "up";
        break;

    case -1 :
        size = 8;
        updown = "up";
        break;

    case 1 :
        size = 8;
        updown = "down";
        break;

    case 2 :
        size = 15;
        updown = "down";
        break;
    }

    std::stringstream tmp;
    tmp << "       <direction>\n";
    tmp << "        <direction-type>\n";
    tmp << "          <octave-shift size=\"" << size << "\" type=\""
                                             << updown << "\"/>\n";
    tmp << "        </direction-type>\n";
    tmp << "      </direction>\n";

    m_strDirection += tmp.str();
    m_pendingDirections = true;
    m_directionTime = time;

    tmp.str("");
    tmp << "       <direction>\n";
    tmp << "        <direction-type>\n";
    tmp << "          <octave-shift size=\"" << size << "\" type=\"stop\"/>\n";
    tmp << "        </direction-type>\n";
    tmp << "      </direction>\n";
    queue(true, time+indication.getIndicationDuration()-1, tmp.str());
}

void
MusicXmlExportHelper::addLyric(const Event &event)
{
    //!!! Should m_syllabics staff dependend? Several staffs of a multistaff
    //!!! part can have different lyrics!

    Text text = Text(event);

    QString txt = QString::fromStdString(text.getText()).trimmed();
    int verse = text.getVerse();
    bool single = ! txt.contains(QRegExp(" *-$"));
    if (single) {
        if ((m_syllabic[verse] == "begin") ||
            (m_syllabic[verse] == "middle")) {
            m_syllabic[verse] = "end";
        } else {
            m_syllabic[verse] = "single";
        }
    } else {
        if ((m_syllabic[verse] == "begin") ||
            (m_syllabic[verse] == "middle")) {
            m_syllabic[verse] = "middle";
        } else {
            m_syllabic[verse] = "begin";
        }
    }
    std::stringstream tmp;
    tmp << "        <lyric number=\"" << verse+1 << "\">\n"
        << "          <syllabic>" << m_syllabic[verse] << "</syllabic>\n"
        << "          <text>" << txt.remove(QRegExp(" *-$")) << "</text>\n"
        << "        </lyric>\n";
    m_strLyrics += tmp.str();
}

void
MusicXmlExportHelper::addNote(const Segment &segment, const Event &event)
{
    timeT time = event.getNotationAbsoluteTime();

    std::stringstream tmpNote;
    std::stringstream tmpInstrument;
    std::stringstream tmpAccidental;
    std::stringstream tmpNotehead;
    m_pendingNote = true;

    tmpNote << "      <note>\n";

    // grace
    bool isGrace = event.has(IS_GRACE_NOTE) && event.get<Bool>(IS_GRACE_NOTE);
    tmpNote << (isGrace ? "        <grace/>\n" : "");

    // rest
    // chord
    // pitch
    // duration
    bool isChord = false;
    timeT duration = isGrace ? 0 : event.getNotationDuration();
    if (time < m_curtime) {
        m_curtime -= duration;
        isChord = true;
    }
    if (m_curtime + duration > segment.getEndMarkerTime()) {
        duration = segment.getEndMarkerTime() - m_curtime;
    }

    if (event.isa(Rosegarden::Note::EventRestType)) {
        timeT barStart = m_composition->getBarStartForTime(time);
        timeT barEnd = m_composition->getBarEndForTime(time);
        std::string measureRest = barEnd - barStart == duration ? " measure=\"yes\"" : "";
        if (getNumberOfActiveVoices(time) > 1) {
            // Staff has multiple voices, place the rest of voices 1 and 3 higher
            // and voices 2 and 4 lower as usual.
            // Rests will be placed on height of F4 (pitch 65) or F5 (77).
            Pitch pitch(77 - 12 * ((m_curVoice - m_staves[m_staff].firstVoice) % 2));
            tmpNote << "        <rest" << measureRest << ">\n";
            tmpNote << "          <display-step>" << pitch.getNoteName(Key())
                                                  << "</display-step>\n";
            tmpNote << "          <display-octave>" << pitch.getOctaveAccidental(-1)
                                                    << "</display-octave>\n";
            tmpNote << "        </rest>\n";
        } else {
            tmpNote << "        <rest" << measureRest << "/>\n";
        }
        if (duration > 0) {
            tmpNote << "        <duration>" << duration << "</duration>\n";
        }
    } else {
        if (isChord) {
            tmpNote << "        <chord/>\n";
        }
        if (m_percussionTrack) {
            Pitch pitch(event);
            if (event.has("MxmlPitch")) {
                pitch = Pitch(event.get<Int>("MxmlPitch"));
            }
            tmpNote << "        <unpitched>\n";
            tmpNote << "          <display-step>" << pitch.getNoteName(Key())
                                                  << "</display-step>\n";
            tmpNote << "          <display-octave>" << pitch.getOctaveAccidental(-1)
                                                    << "</display-octave>\n";
            tmpNote << "        </unpitched>\n";
            tmpNotehead << "        <notehead>" << event.get<String>("MxmlNoteHead")
                                                << "</notehead>\n";

            if (duration > 0) {
                tmpNote << "        <duration>" << duration << "</duration>\n";
            }

            int pp =  pitch.getPerformancePitch();
            tmpInstrument << "        <instrument id=\"" << getPartName() << "-I"
                                                         << pp+1 << "\"/>\n";
        } else {
            Pitch pitch(event);
            tmpNote << "        <pitch>" << std::endl;
            tmpNote << "          <step>" << pitch.getNoteName(m_staves[m_staff].key)
                                          << "</step>" << std::endl;
            Accidental acc = pitch.getAccidental(m_staves[m_staff].key);
            if (acc == Accidentals::DoubleFlat) {
                tmpNote << "          <alter>-2</alter>\n";
            } else if (acc == Accidentals::Flat) {
                tmpNote << "          <alter>-1</alter>\n";
            } else if (acc == Accidentals::Sharp) {
                tmpNote << "          <alter>1</alter>\n";
            } else if (acc == Accidentals::DoubleSharp) {
                tmpNote << "          <alter>2</alter>\n";
            }
            tmpNote << "          <octave>" << pitch.getOctaveAccidental(-1, acc)
                                            << "</octave>\n";
            tmpNote << "        </pitch>\n";
            if (duration > 0) {
                tmpNote << "        <duration>" << duration << "</duration>\n";
            }
            bool cautionary = false;
            event.get<Bool>(NotationProperties::USE_CAUTIONARY_ACCIDENTAL, cautionary);

            std::string editorial = "";
            if (cautionary) {
                editorial = " editorial=\"yes\"";
            }
            acc = pitch.getDisplayAccidental(m_staves[m_staff].key);
            acc = m_staves[m_staff].accTable.processDisplayAccidental(acc,
                        pitch.getHeightOnStaff(m_staves[m_staff].clef, m_staves[m_staff].key),
                        cautionary);
            m_staves[m_staff].accTable.update();
            if (acc == Accidentals::Natural) {
                tmpAccidental << "        <accidental" << editorial
                                                       << ">natural</accidental>\n";
            } else if (acc == Accidentals::Sharp) {
                tmpAccidental << "        <accidental" << editorial
                                                       << ">sharp</accidental>\n";
            } else if (acc == Accidentals::Flat) {
                tmpAccidental << "        <accidental" << editorial
                                                       << ">flat</accidental>\n";
            } else if (acc == Accidentals::DoubleSharp) {
                tmpAccidental << "        <accidental" << editorial
                                                       << ">sharp-sharp</accidental>\n";
            } else if (acc == Accidentals::DoubleFlat) {
                tmpAccidental << "        <accidental" << editorial
                                                       << ">flat-flat</accidental>\n";
            }
        }
    }

    // tie
    if (event.has(TIED_BACKWARD) && event.get<Bool>(TIED_BACKWARD)) {
        tmpNote << "        <tie type=\"stop\"/>\n";
    }
    if (event.has(TIED_FORWARD) && event.get<Bool>(TIED_FORWARD)) {
        tmpNote << "        <tie type=\"start\"/>\n";
    }

    // instrument
    tmpNote << tmpInstrument.str();

    // voice
    tmpNote << "        <voice>" << m_curVoice << "</voice>\n";

    // type
    // dots
    int noteDuration = event.getNotationDuration();
    if (m_curtime + noteDuration > segment.getEndMarkerTime()) {
        noteDuration = segment.getEndMarkerTime() - m_curtime;
    }
    if ( !isGrace) {
        noteDuration = noteDuration*m_actualNotes/m_normalNotes;
    }
    Note note = Note::getNearestNote(noteDuration);
    tmpNote << "        <type>" << getNoteName(note.getNoteType()) << "</type>\n";
    for (int i = 0; i < note.getDots(); ++i) {
        tmpNote << "        <dot/>\n";
    }

    // accidental
    tmpNote << tmpAccidental.str();

    // time-modification
    if (m_tupletGroup != "none") {
        tmpNote << "        <time-modification>\n";
        tmpNote << "          <actual-notes>" << m_actualNotes << "</actual-notes>\n";
        tmpNote << "          <normal-notes>" << m_normalNotes << "</normal-notes>\n";
        long base = 0;
        if (event.has(BEAMED_GROUP_TUPLET_BASE)) {
            event.get<Int>(BEAMED_GROUP_TUPLET_BASE, base);
            if (base != noteDuration) {
                Note note = Note::getNearestNote(base);
                tmpNote << "          <normal-type>" << getNoteName(note.getNoteType())
                                                     << "</normal-type>\n";
            }
        }
        tmpNote << "        </time-modification>\n";
    }

    // stem
    if (event.has(NotationProperties::BEAM_ABOVE)) {
        tmpNote << "        <stem>"
               << (event.get<Bool>(NotationProperties::BEAM_ABOVE) ? "up" : "down")
               << "</stem>\n";
    } else {
        if (event.has(NotationProperties::STEM_UP)) {
            tmpNote << "        <stem>"
                   << (event.get<Bool>(NotationProperties::STEM_UP) ? "up" : "down")
                   << "</stem>\n";
        }
    }

    // notehead
    tmpNote << tmpNotehead.str();

    // staff
    if (isMultiStave()) {
        tmpNote << "        <staff>" << m_staff + 1 << "</staff>\n";
    }

    // beam
    if ((m_curbeam > 0) && !isChord) {
        for (int number = 1; number <= 4; number++) {
            int prv = m_prvbeam & (1 << (number-1));
            int cur = m_curbeam & (1 << (number-1));
            int nxt = m_nxtbeam & (1 << (number-1));
            if (cur == 0) break;
            std::string s;
            if (!prv && cur && nxt) s = "begin";
            else if (prv && cur && !nxt) s = "end";
            else if (prv && cur && nxt) s = "continue";
            else {
                if (m_prvbeam > m_nxtbeam) s = "backward hook";
                else s = "forward hook";
            }
            tmpNote << "        <beam number=\"" << number << "\">" << s
                                                 << "</beam>\n";
        }
    }

    // notations
    std::vector<Mark> marks = Marks::getMarks(event);
    std::vector<std::string> articulations;
    std::vector<std::string> ornaments;
    std::vector<std::string> technical;
    std::vector<std::string> dynamics;
    int notations = 0;
    bool fermata = false;
    for (std::vector<Mark>::iterator m = marks.begin(); m != marks.end(); ++m) {
        notations++;
        if (*m == Marks::Accent) {
            articulations.push_back("<accent/>");
        } else if (*m == Marks::Tenuto) {
            articulations.push_back("<tenuto/>");
        } else if (*m == Marks::Staccato) {
            articulations.push_back("<staccato/>");
        } else if (*m == Marks::Staccatissimo) {
            articulations.push_back("<staccatissimo/>");
        } else if (*m == Marks::Marcato) {
            articulations.push_back("<strong-accent type=\"up\"/>");
        } else if (*m == Marks::Trill) {
            ornaments.push_back("<trill-mark/>");
        } else if (*m == Marks::Turn) {
            ornaments.push_back("<turn/>");
        } else if (*m == Marks::Mordent) {
            ornaments.push_back("<mordent/>");
        } else if (*m == Marks::MordentInverted) {
            ornaments.push_back("<inverted-mordent/>");
        } else if (*m == Marks::LongTrill) {
            ornaments.push_back("<trill-mark/>\n<wavy-line type=\"start\" number=\"1\"/>\n<wavy-line type=\"stop\" number=\"1\"/>");
        } else if (*m == Marks::TrillLine) {
            ornaments.push_back("<wavy-line type=\"start\" number=\"1\"/>\n<wavy-line type=\"stop\" number=\"1\"/>");
        } else if (*m == Marks::UpBow) {
            technical.push_back("<up-bow/>");
        } else if (*m == Marks::DownBow) {
            technical.push_back("<down-bow/>");
        } else if (*m == Marks::Harmonic) {
            technical.push_back("<harmonic placement=\"above\"/>");
        } else if (*m == Marks::Open) {
            technical.push_back("<open-string/>");
        } else if (*m == Marks::Stopped) {
            technical.push_back("<stopped placement=\"above\"/>");
        } else if (*m == Marks::Sforzando) {
            dynamics.push_back("<sf/>");
        } else if (*m == Marks::Rinforzando) {
            dynamics.push_back("<rf/>");
        } else if (*m == Marks::Pause) {
            fermata = true;
        } else {
            std::cerr << "WARNING: MusicXmlExportHelper::addNote: mark \""
                      << *m << "\" not supported." << std::endl;
            notations--;
        }
    }
    long slashes = 0;
    event.get<Int>(NotationProperties::SLASHES, slashes);
    if (slashes > 0) {
        std::stringstream tmp;
        tmp << "<tremolo>" << slashes << "</tremolo>";
        ornaments.push_back(tmp.str());
        notations++;
    }

//     const Mark NoMark = "no-mark";
//
//     const Mark MordentLong = "mordent-long";
//     const Mark MordentLongInverted = "mordent-long-inverted";


    if (notations) {
        tmpNote << "        <notations>\n";
        if (!articulations.empty()) {
            tmpNote << "          <articulations>\n";
            for (std::vector<std::string>::iterator s = articulations.begin();
                 s != articulations.end(); ++s)
                tmpNote << "            " << *s << "\n";
            tmpNote << "          </articulations>\n";
        }
        if (!ornaments.empty()) {
            tmpNote << "          <ornaments>\n";
            for (std::vector<std::string>::iterator s = ornaments.begin();
                 s != ornaments.end(); ++s)
                tmpNote << "            "  << *s << "\n";
            tmpNote << "          </ornaments>\n";
        }
        if (!technical.empty()) {
            tmpNote << "          <technical>\n";
            for (std::vector<std::string>::iterator s = technical.begin();
                 s != technical.end(); ++s)
                tmpNote << "            "  << *s << "\n";
            tmpNote << "          </technical>\n";
        }
        if (!dynamics.empty()) {
            tmpNote << "          <dynamics>\n";
            for (std::vector<std::string>::iterator s = dynamics.begin();
                 s != dynamics.end(); ++s)
                tmpNote << "            "  << *s << "\n";
            tmpNote << "          </dynamics>\n";
        }
    }
    if (fermata) {
        if (!notations) {
            tmpNote << "        <notations>\n";
            notations = true;
        }
        tmpNote << "          <fermata type=\"upright\"/>\n";
    }
    if ((m_tupletGroup == "start") || (m_tupletGroup == "stop")) {
        if (!notations) {
            tmpNote << "        <notations>\n";
            notations = true;
        }
        tmpNote << "          <tuplet number=\"1\" type=\"" << m_tupletGroup
                                                            << "\"/>\n";
    }
    if ((event.has(TIED_BACKWARD)) || (event.has(TIED_FORWARD))) {
        if (!notations) {
            tmpNote << "        <notations>\n";
            notations = true;
        }
        if (event.has(TIED_BACKWARD) && event.get<Bool>(TIED_BACKWARD))
            tmpNote << "          <tied type=\"stop\"/>\n";
        if (event.has(TIED_FORWARD) && event.get<Bool>(TIED_FORWARD))
            tmpNote << "          <tied type=\"start\"/>\n";
    }
    std::string endSlurs = retrieve(false, m_curtime+duration);
    if (endSlurs != "") {
        if (!notations) {
            tmpNote << "        <notations>\n";
            notations = true;
        }
        tmpNote << endSlurs;
    }
    if (m_strSlurs != "") {
        if (!notations) {
            tmpNote << "        <notations>\n";
            notations = true;
        }
        tmpNote << m_strSlurs;
        m_strSlurs = "";
    }

    if (notations) tmpNote << "        </notations>\n";

    // lyric
    tmpNote << m_strLyrics;
    m_strLyrics = "";

    tmpNote << "      </note>\n";

    m_strNote += tmpNote.str();

    m_curtime = time + duration;
}

void
MusicXmlExportHelper::flush(std::ostream &str)
{
    if (m_pendingAttributes && (m_pendingDirections || m_pendingNote) &&
        (m_curtime >= m_attributesTime)) {
        str << "      <attributes>\n";
        str << m_strDivisions;
        str << m_strKey;
        str << m_strTimesignature;
        str << m_strStaves;
        str << m_strClef;
        str << m_strStaffDetails;
        str << m_strTranspose;
        str << "      </attributes>\n";

        m_strDivisions = "";
        m_strKey = "";
        m_strTimesignature = "";
        m_strStaves = "";
        m_strClef = "";
        m_strStaffDetails = "";
        m_strTranspose = "";
        m_pendingAttributes = false;
    }

    if (m_pendingDirections && (m_curtime >= m_directionTime)) {
        str << m_strDirection;
        m_strDirection = "";
        m_pendingDirections = false;
    }

    str << retrieve(true, m_curtime);

    if (m_pendingNote) {
        str << m_strNote;
        m_strNote = "";
        m_pendingNote = false;
    }
}

std::string
MusicXmlExportHelper::getNoteName(int noteType) const
{
    static const char *noteNames[] = {
        "64th", "32nd", "16th", "eighth", "quarter", "half", "whole", "breve"
    };

    if (noteType < 0 || noteType >= int(sizeof(noteNames) / sizeof(noteNames[0]))) {
        std::cerr << "WARNING: MusicXmlExportHelper::getNoteName: bad note type "
                  << noteType << std::endl;
        noteType = 4;
    }

    return noteNames[noteType];
}

void
MusicXmlExportHelper::queue(bool direction, timeT time, std::string str)
{
    SimpleQueue sq;
    sq.direction = direction;
    sq.staff = m_staff;
    sq.voice = m_curVoice;
    sq.time = time;
    sq.string = str;
    tmp_queue.push_back(sq);
}

std::string
MusicXmlExportHelper::retrieve(bool direction, timeT time)
{
    std::string result = "";
    std::vector<std::vector<SimpleQueue>::iterator> toErase;
    for (std::vector<SimpleQueue>::iterator i = tmp_queue.begin();
         i != tmp_queue.end(); ++i) {
        SimpleQueue sq = *i;
        if ((sq.direction == direction) && (sq.staff == m_staff) &&
            (sq.voice == m_curVoice) && (sq.time <= time)) {
            result += sq.string;
            toErase.push_back(i);
        }
    }
    for (std::vector<std::vector<SimpleQueue>::iterator>::iterator i = toErase.begin();
         i != toErase.end(); ++i) {
        tmp_queue.erase(*i);
    }

    return result;
}

void
MusicXmlExportHelper::updatePart(Segment *segment, Event &event)
{
    Segment::iterator cur = segment->findSingle(&event);
    Segment::iterator last = segment->end();
    timeT barEnd = m_composition->getBarEndForTime((*cur)->getNotationAbsoluteTime());
    m_tupletGroup = "none";
    m_normalNotes = 1;
    m_actualNotes = 1;
    std::string groupType = "";
    if (!(*cur)->get<String>(BEAMED_GROUP_TYPE, groupType)) {
        m_prvbeam = m_curbeam = m_nxtbeam = 0;
        m_group = -1;
        return;
    }

    m_prvbeam = m_curbeam;
    int duration = (*cur)->getNotationDuration();
    if (groupType == GROUP_TYPE_TUPLED) {
        (*cur)->get<Int>(BEAMED_GROUP_TUPLED_COUNT, m_normalNotes);
        (*cur)->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT, m_actualNotes);
        duration = duration * m_actualNotes / m_normalNotes;
    }
    Note note = Note::getNearestNote(duration);
    m_curbeam = note.getNoteType() <= 3 ? (1 << (4 - note.getNoteType())) - 1 : 0;
    m_nxtbeam = 0;

    bool isLast = false;
    long groupId = -1, newGroupId = -1;
    (void) (*cur)->get<Int>(BEAMED_GROUP_ID, groupId);
    bool isLastEvent = true;
    for (Segment::iterator i = cur; i != last; ++i) {
        if ((i == cur) || ((*i)->getNotationAbsoluteTime() == (*cur)->getNotationAbsoluteTime())) {
            continue;
        }
        if ((*i)->isa(Rosegarden::Note::EventType) ||
            (*i)->isa(Rosegarden::Note::EventRestType)) {
            if ((*i)->get<Int>(BEAMED_GROUP_ID, newGroupId)) {
                isLast = newGroupId != groupId;
            } else {
                isLast = true;
            }
            if (isLast || ((*i)->getNotationAbsoluteTime() >= barEnd)) {
                m_nxtbeam = 0;
            } else {
                Note note = Note::getNearestNote((*i)->getNotationDuration());
                m_nxtbeam = note.getNoteType() <= 3
                          ? (1 << (4 - note.getNoteType())) - 1 : 0;
            }
            isLastEvent = false;
            break;
        }
    }
    if (isLastEvent) {
        isLast = true;
        m_nxtbeam = 0;
    }

    if (groupType == GROUP_TYPE_TUPLED) {
        if (isLast) {
            m_tupletGroup = "stop";
        } else if (groupId != m_group) {
            m_tupletGroup = "start";
        } else {
            m_tupletGroup = "running";
        }
    } else if (groupType == GROUP_TYPE_BEAMED) {
       if (groupId != m_group) {
            m_prvbeam = 0;
        }
    }
    m_group = groupId;
}

}
