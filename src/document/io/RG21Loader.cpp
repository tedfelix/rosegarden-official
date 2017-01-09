/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RG21Loader]"

#include "RG21Loader.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/BaseProperties.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/general/ProgressReporter.h"
#include <QFile>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <string>
#include <vector>
#include <iostream>

using std::vector;
using std::string;


namespace Rosegarden
{

using namespace BaseProperties;
using namespace Accidentals;
using namespace Marks;

RG21Loader::RG21Loader(Studio *studio,
                       QObject *parent)
        : ProgressReporter(parent),
        m_stream(0),
        m_studio(studio),
        m_composition(0),
        m_currentSegment(0),
        m_currentSegmentTime(0),
        m_currentSegmentNb(0),
        m_currentClef(Clef::Treble),
        m_currentInstrumentId(MidiInstrumentBase),
        m_inGroup(false),
        m_tieStatus(0),
        m_nbStaves(0)
{}

RG21Loader::~RG21Loader()
{}

bool RG21Loader::parseClef()
{
    if (m_tokens.count() != 3 || !m_currentSegment)
        return false;

    std::string clefName = qstrtostr(m_tokens[2].toLower());

    m_currentClef = Clef(clefName);
    Event *clefEvent = m_currentClef.getAsEvent(m_currentSegmentTime);
    m_currentSegment->insert(clefEvent);

    return true;
}

bool RG21Loader::parseKey()
{
    if (m_tokens.count() < 3 || !m_currentSegment)
        return false;

    QString keyBase = m_tokens[2];
    if (keyBase.length() > 1) {
        // Deal correctly with e.g. Bb major
        keyBase =
            keyBase.left(1).toUpper() +
            keyBase.right(keyBase.length() - 1).toLower();
    } else {
        keyBase = keyBase.toUpper();
    }

    QString keyName = QString("%1 %2or")
                      .arg(keyBase)
                      .arg(m_tokens[3].toLower());

    m_currentKey = Rosegarden::Key(qstrtostr(keyName));
    Event *keyEvent = m_currentKey.getAsEvent(m_currentSegmentTime);
    m_currentSegment->insert(keyEvent);

    return true;
}

bool RG21Loader::parseMetronome()
{
    if (m_tokens.count() < 2)
        return false;
    if (!m_composition)
        return false;

    QStringList::Iterator i = m_tokens.begin();
    timeT duration = convertRG21Duration(i);

    bool isNumeric = false;
    int count = (*i).toInt(&isNumeric);
    if (!count || !isNumeric)
        return false;

    // we need to take into account the fact that "duration" might not
    // be a crotchet

    double qpm = (count * duration) / Note(Note::Crotchet).getDuration();
    m_composition->addTempoAtTime(m_currentSegmentTime,
                                  m_composition->getTempoForQpm(qpm));
    return true;
}

bool RG21Loader::parseChordItem()
{
    if (m_tokens.count() < 4)
        return false;

    QStringList::Iterator i = m_tokens.begin();
    timeT duration = convertRG21Duration(i);

    // get chord mod flags and nb of notes.  chord mod is hex
    int chordMods = (*i).toInt(0, 16);
    ++i;
    /*int nbNotes   = (*i).toInt();*/
    ++i;

    vector<string> marks = convertRG21ChordMods(chordMods);

    // now get notes
    for (;i != m_tokens.end(); ++i) {

        long pitch = (*i).toInt();
        ++i;

        // The noteMods field is nominally a hex integer.  As it
        // happens its value can never exceed 7, but I guess we
        // should do the right thing anyway
        int noteMods = (*i).toInt(0, 16);
        pitch = convertRG21Pitch(pitch, noteMods);

        Event *noteEvent = new Event(Note::EventType,
                                     m_currentSegmentTime, duration);
        noteEvent->set
        <Int>(PITCH, pitch);

        if (m_tieStatus == 1) {
            noteEvent->set
            <Bool>(TIED_FORWARD, true);
        } else if (m_tieStatus == 2) {
            noteEvent->set
            <Bool>(TIED_BACKWARD, true);
        }

        if (!marks.empty()) {
            noteEvent->set
            <Int>(MARK_COUNT, int(marks.size()));
            for (unsigned int j = 0; j < (unsigned int)marks.size(); ++j) {
                noteEvent->set
                <String>(getMarkPropertyName(j), marks[j]);
            }
        }

        //         RG_DEBUG << "RG21Loader::parseChordItem() : insert note pitch " << pitch
        //                              << " at time " << m_currentSegmentTime << endl;

        setGroupProperties(noteEvent);

        m_currentSegment->insert(noteEvent);
    }

    m_currentSegmentTime += duration;
    if (m_tieStatus == 2)
        m_tieStatus = 0;
    else if (m_tieStatus == 1)
        m_tieStatus = 2;

    return true;
}

bool RG21Loader::parseRest()
{
    if (m_tokens.count() < 2)
        return false;

    QStringList::Iterator i = m_tokens.begin();
    timeT duration = convertRG21Duration(i);

    Event *restEvent = new Event(Note::EventRestType,
                                 m_currentSegmentTime, duration,
                                 Note::EventRestSubOrdering);

    setGroupProperties(restEvent);

    m_currentSegment->insert(restEvent);
    m_currentSegmentTime += duration;

    return true;
}

bool RG21Loader::parseText()
{
    if (!m_currentSegment)
        return false;

    std::string s;
    for (int i = 1; i < m_tokens.count(); ++i) {
        if (i > 1)
            s += " ";
        s += qstrtostr(m_tokens[i]);
    }

    if (!readNextLine() ||
            m_tokens.count() != 2 || m_tokens[0].toLower() != "position") {
        return false;
    }

    int rg21posn = m_tokens[1].toInt();
    std::string type = Text::UnspecifiedType;

    switch (rg21posn) {

    case TextAboveStave:
        type = Text::LocalTempo;
        break;

    case TextAboveStaveLarge:
        type = Text::Tempo;
        break;

    case TextAboveBarLine:
        type = Text::Direction;
        break;

    case TextBelowStave:
        type = Text::Lyric; // perhaps
        break;

    case TextBelowStaveItalic:
        type = Text::LocalDirection;
        break;

    case TextChordName:
        type = Text::ChordName;
        break;

    case TextDynamic:
        type = Text::Dynamic;
        break;
    }

    Text text(s, type);
    Event *textEvent = text.getAsEvent(m_currentSegmentTime);
    m_currentSegment->insert(textEvent);

    return true;
}

void RG21Loader::setGroupProperties(Event *e)
{
    if (m_inGroup) {

        e->set
        <Int>(BEAMED_GROUP_ID, m_groupId);
        e->set
        <String>(BEAMED_GROUP_TYPE, m_groupType);

        m_groupUntupledLength += e->getDuration();
    }
}

bool RG21Loader::parseGroupStart()
{
    m_groupType = qstrtostr(m_tokens[0].toLower());
    m_inGroup = true;
    m_groupId = m_currentSegment->getNextId();
    m_groupStartTime = m_currentSegmentTime;

    if (m_groupType == GROUP_TYPE_BEAMED) {

        // no more to do

    } else if (m_groupType == GROUP_TYPE_TUPLED) {

        // RG2.1 records two figures A and B, of which A is a time
        // value indicating the total duration of the group _after_
        // tupling (which we would call the tupled length), and B is
        // the count that appears above the group (which we call the
        // untupled count).  We need to know C, the total duration of
        // the group _before_ tupling; then we can calculate the
        // tuplet base (C / B) and tupled count (A * B / C).

        m_groupTupledLength = m_tokens[1].toUInt() *
                              Note(Note::Hemidemisemiquaver).getDuration();

        m_groupUntupledCount = m_tokens[2].toUInt();
        m_groupUntupledLength = 0;

    } else {

        RG_DEBUG
        << "RG21Loader::parseGroupStart: WARNING: Unknown group type "
        << m_groupType << ", ignoring" << endl;
        m_inGroup = false;
    }

    return true;
}

bool RG21Loader::parseIndicationStart()
{
    if (m_tokens.count() < 4)
        return false;

    unsigned int indicationId = m_tokens[2].toUInt();
    std::string indicationType = qstrtostr(m_tokens[3].toLower());

    //    RG_DEBUG << "Indication start: type is \"" << indicationType << "\"";

    if (indicationType == "tie") {

        if (m_tieStatus != 0) {
            RG_DEBUG
            << "RG21Loader:: parseIndicationStart: WARNING: Found tie within "
            << "tie, ignoring" << endl;
            return true;
        }
        // m_tieStatus = 1;

        Segment::iterator i = m_currentSegment->end();
        if (i != m_currentSegment->begin()) {
            --i;
            timeT t = (*i)->getAbsoluteTime();
            while ((*i)->getAbsoluteTime() == t) {
                (*i)->set
                <Bool>(TIED_FORWARD, true);
                if (i == m_currentSegment->begin())
                    break;
                --i;
            }
        }
        m_tieStatus = 2;

        RG_DEBUG << "rg21io: Indication start: it's a tie";

    } else {

        // Jeez.  Whose great idea was it to place marks _after_ the
        // events they're marking in the RG2.1 file format?

        timeT indicationTime = m_currentSegmentTime;
        Segment::iterator i = m_currentSegment->end();
        if (i != m_currentSegment->begin()) {
            --i;
            indicationTime = (*i)->getAbsoluteTime();
        }

        Indication indication(indicationType, 0);
        Event *e = indication.getAsEvent(indicationTime);
        e->setMaybe<Int>("indicationId", indicationId);
        setGroupProperties(e);
        m_indicationsExtant[indicationId] = e;

        // place the indication in the segment now; don't wait for the
        // close-indication, because some things may need to know about it
        // before then (e.g. close-group)

        m_currentSegment->insert(e);

        RG_DEBUG << "rg21io: Indication start: it's a real indication; id is " << indicationId << ", event is:";
        e->dump(std::cerr);

    }

    // other indications not handled yet
    return true;
}

void RG21Loader::closeIndication()
{
    if (m_tokens.count() < 3)
        return ;

    unsigned int indicationId = m_tokens[2].toUInt();
    EventIdMap::iterator i = m_indicationsExtant.find(indicationId);

    RG_DEBUG << "rg21io: Indication close: indication id is " << indicationId;

    // this is normal (for ties):
    if (i == m_indicationsExtant.end())
        return ;

    Event *indicationEvent = i->second;
    m_indicationsExtant.erase(i);

    indicationEvent->set
    <Int>
    //!!!	(Indication::IndicationDurationPropertyName,
    ("indicationduration",
     m_currentSegmentTime - indicationEvent->getAbsoluteTime());
}

void RG21Loader::closeGroup()
{
    if (m_groupType == GROUP_TYPE_TUPLED) {

        Segment::iterator i = m_currentSegment->end();
        vector<Event *> toInsert;
        vector<Segment::iterator> toErase;

        if (i != m_currentSegment->begin()) {

            --i;
            long groupId;
            timeT prev = m_groupStartTime + m_groupTupledLength;

            while ((*i)->get
                    <Int>(BEAMED_GROUP_ID, groupId) &&
                    groupId == m_groupId) {

                timeT absoluteTime = (*i)->getAbsoluteTime();
                timeT offset = absoluteTime - m_groupStartTime;
                timeT intended =
                    (offset * m_groupTupledLength) / m_groupUntupledLength;

                RG_DEBUG
                << "RG21Loader::closeGroup:"
                << " m_groupStartTime = " << m_groupStartTime
                << ", m_groupTupledLength = " << m_groupTupledLength
                << ", m_groupUntupledCount = " << m_groupUntupledCount
                << ", m_groupUntupledLength = " << m_groupUntupledLength
                << ", absoluteTime = " << (*i)->getAbsoluteTime()
                << ", offset = " << offset
                << ", intended = " << intended
                << ", new absolute time = "
                << (absoluteTime + intended - offset)
                << ", new duration = "
                << (prev - absoluteTime)
                << endl;

                absoluteTime = absoluteTime + intended - offset;
                Event *e(new Event(**i, absoluteTime, prev - absoluteTime));
                prev = absoluteTime;

                // See comment in parseGroupStart
                e->set
                <Int>(BEAMED_GROUP_TUPLET_BASE,
                      m_groupUntupledLength / m_groupUntupledCount);
                e->set
                <Int>(BEAMED_GROUP_TUPLED_COUNT,
                      m_groupTupledLength * m_groupUntupledCount /
                      m_groupUntupledLength);
                e->set
                <Int>(BEAMED_GROUP_UNTUPLED_COUNT, m_groupUntupledCount);

                // To change the time of an event, we need to erase &
                // re-insert it.  But erasure will delete the event, and
                // if it's an indication event that will invalidate our
                // indicationsExtant entry.  Hence this unpleasantness:

                if ((*i)->isa(Indication::EventType)) {
                    long indicationId = 0;
                    if ((*i)->get
                            <Int>("indicationId", indicationId)) {
                        EventIdMap::iterator ei =
                            m_indicationsExtant.find(indicationId);
                        if (ei != m_indicationsExtant.end()) {
                            m_indicationsExtant.erase(ei);
                            m_indicationsExtant[indicationId] = e;
                        }
                    }
                }

                toInsert.push_back(e);
                toErase.push_back(i);

                if (i == m_currentSegment->begin())
                    break;
                --i;
            }
        }

        for (size_t i = 0; i < toInsert.size(); ++i) {
            m_currentSegment->insert(toInsert[i]);
        }
        for (size_t i = 0; i < toErase.size(); ++i) {
            m_currentSegment->erase(toErase[i]);
        }

        m_currentSegmentTime = m_groupStartTime + m_groupTupledLength;
    }

    m_inGroup = false;
}

bool RG21Loader::parseBarType()
{
    if (m_tokens.count() < 5)
        return false;
    if (!m_composition)
        return false;

    int staffNo = m_tokens[1].toInt();
    if (staffNo > 0) {
        RG_DEBUG
        << "RG21Loader::parseBarType: We don't support different time\n"
        << "signatures on different staffs; disregarding time signature for staff " << staffNo << endl;
        return true;
    }

    // barNo is a hex integer
    int barNo = m_tokens[2].toInt(0, 16);

    int numerator = m_tokens[4].toInt();
    int denominator = m_tokens[5].toInt();

    timeT sigTime = m_composition->getBarRange(barNo).first;
    TimeSignature timeSig(numerator, denominator);
    m_composition->addTimeSignature(sigTime, timeSig);

    return true;
}

bool RG21Loader::parseStaveType()
{
    //!!! tags & connected are not yet implemented

    if (m_tokens.count() < 9)
        return false;
    if (!m_composition)
        return false;

    bool isNumeric = false;

    int staffNo = m_tokens[1].toInt(&isNumeric);
    if (!isNumeric)
        return false;

    int programNo = m_tokens[8].toInt();

    if (staffNo >= (int)m_composition->getMinTrackId() &&
            staffNo <= (int)m_composition->getMaxTrackId()) {

        Track *track = m_composition->getTrackById(staffNo);

        if (track) {
            Instrument *instr =
                m_studio->assignMidiProgramToInstrument(programNo, false);
            if (instr)
                track->setInstrument(instr->getId());
        }
    }

    return true;
}

timeT RG21Loader::convertRG21Duration(QStringList::Iterator& i)
{
    QString durationString = (*i).toLower();
    ++i;

    if (durationString == "dotted") {
        durationString += ' ';
        durationString += (*i).toLower();
        ++i;
    }

    try {

        Note n(NotationStrings::getNoteForName(durationString));
        return n.getDuration();

    } catch (NotationStrings::MalformedNoteName m) {

        RG_DEBUG << "RG21Loader::convertRG21Duration: Bad duration: "
        << durationString << endl;
        return 0;
    }

}

void RG21Loader::closeSegment()
{
    if (m_currentSegment) {

        TrackId trackId = m_currentSegmentNb - 1;

        m_currentSegment->setTrack(trackId);

        Track *track = new Track
                       (trackId, m_currentInstrumentId, trackId,
                        qstrtostr(m_currentStaffName), false);
        m_currentInstrumentId = (m_currentInstrumentId + 1) % 16;

        m_composition->addTrack(track);

        std::vector<TrackId> trackIds;
        trackIds.push_back(track->getId());
        m_composition->notifyTracksAdded(trackIds);

        m_composition->addSegment(m_currentSegment);
        m_currentSegment = 0;
        m_currentSegmentTime = 0;
        m_currentClef = Clef(Clef::Treble);

    } else {
        // ??
    }
}

long RG21Loader::convertRG21Pitch(long pitch, int noteModifier)
{
    Accidental accidental =
        (noteModifier & ModSharp) ? Sharp :
        (noteModifier & ModFlat) ? Flat :
        (noteModifier & ModNatural) ? Natural : NoAccidental;

    long rtn = Pitch::getPerformancePitchFromRG21Pitch
               (pitch, accidental, m_currentClef, m_currentKey);

    return rtn;
}

bool RG21Loader::readNextLine()
{
    bool inComment = false;

    do {
        inComment = false;

        m_currentLine = m_stream->readLine();

        if (m_stream->atEnd())
            return false;

        m_currentLine = m_currentLine.simplified();

        if (m_currentLine[0] == '#' ||
                m_currentLine.length() == 0) {
            inComment = true;
            continue; // skip comments
        }

        m_tokens = m_currentLine.split(' ', QString::SkipEmptyParts);

    } while (inComment);

    return true;
}

bool RG21Loader::load(const QString &fileName, Composition &comp)
{
    m_composition = &comp;
    comp.clear();

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        m_stream = new QTextStream(&file);
    } else {
        return false;
    }

    m_studio->unassignAllInstruments();

    while (!m_stream->atEnd()) {

        if (!readNextLine())
            break;

        QString firstToken = m_tokens.first();

        if (firstToken == "Staves" || firstToken == "Staffs") { // nb staves

            m_nbStaves = m_tokens[1].toUInt();

        } else if (firstToken == "Name") { // Staff name

            m_currentStaffName = m_tokens[1]; // we don't do anything with it yet
            m_currentSegment = new Segment;
            ++m_currentSegmentNb;

        } else if (firstToken == "Clef") {

            parseClef();

        } else if (firstToken == "Key") {

            parseKey();

        } else if (firstToken == "Metronome") {

            if (!readNextLine())
                break;
            parseMetronome();

        } else if (firstToken == ":") { // chord

            m_tokens.removeFirst(); // get rid of 1st token ':'
            parseChordItem();

        } else if (firstToken == "Rest") { // rest

            if (!readNextLine())
                break;

            parseRest();

        } else if (firstToken == "Text") {

            if (!readNextLine())
                break;

            parseText();

        } else if (firstToken == "Group") {

            if (!readNextLine())
                break;

            parseGroupStart();

        } else if (firstToken == "Mark") {

            if (m_tokens[1] == "start")
                parseIndicationStart();
            else if (m_tokens[1] == "end")
                closeIndication();

        } else if (firstToken == "Bar") {

            parseBarType();

        } else if (firstToken == "Stave") {

            parseStaveType();

        } else if (firstToken == "End") {

            if (m_inGroup)
                closeGroup();
            else
                closeSegment();

        } else {

            RG_DEBUG << "RG21Loader::parse: Unsupported element type \"" << firstToken << "\", ignoring";
        }
    }

    delete m_stream;
    m_stream = 0;

    return true;
}

vector<string> RG21Loader::convertRG21ChordMods(int chordMods)
{
    vector<string> marks;

    // bit laborious!
    if (chordMods & ModDot)    marks.push_back(Staccato);
    if (chordMods & ModLegato) marks.push_back(Tenuto);
    if (chordMods & ModAccent) marks.push_back(Accent);
    if (chordMods & ModSfz)    marks.push_back(Sforzando);
    if (chordMods & ModRfz)    marks.push_back(Rinforzando);
    if (chordMods & ModTrill)  marks.push_back(Trill);
    if (chordMods & ModTurn)   marks.push_back(Turn);
    if (chordMods & ModPause)  marks.push_back(Pause);

    return marks;
}

}
