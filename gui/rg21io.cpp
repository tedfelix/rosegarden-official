// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <string>

#include "Event.h"
#include "Segment.h"
#include "SegmentNotationHelper.h"
#include "Composition.h"
#include "NotationTypes.h"
#include "BaseProperties.h"

#include "rg21io.h"
#include "rosedebug.h"

using std::vector;
using std::string;

using Rosegarden::Event;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::TimeSignature;
using Rosegarden::Accidental;
using Rosegarden::Sharp;
using Rosegarden::Flat;
using Rosegarden::Natural;
using Rosegarden::NoAccidental;
using Rosegarden::Mark;
using Rosegarden::Note;
using Rosegarden::Indication;
using Rosegarden::timeT;

using namespace Rosegarden::BaseProperties;

RG21Loader::RG21Loader(const QString& fileName)
    : m_file(fileName),
      m_stream(0),
      m_composition(0),
      m_currentSegment(0),
      m_currentSegmentTime(0),
      m_currentSegmentNb(0),
      m_currentClef(Clef::Treble),
      m_inGroup(false),
      m_tieStatus(0),
      m_nbStaves(0)
{

    if (m_file.open(IO_ReadOnly)) {

        m_stream = new QTextStream(&m_file);

        parse();
    }

}

RG21Loader::~RG21Loader()
{
    delete m_stream;
}

bool RG21Loader::parseClef()
{
    if (m_tokens.count() != 3 || !m_currentSegment) return false;
    
    std::string clefName = m_tokens[2].lower().data();

    m_currentClef = Clef(clefName);
    Event *clefEvent = m_currentClef.getAsEvent(m_currentSegmentTime);
    m_currentSegment->insert(clefEvent);

    return true;
}

bool RG21Loader::parseKey()
{
    if (m_tokens.count() < 3 || !m_currentSegment) return false;
    
    QString keyName = QString("%1 %2or")
        .arg(m_tokens[2].upper())
        .arg(m_tokens[3].lower());

    m_currentKey = Rosegarden::Key(std::string(keyName.data()));
    Event *keyEvent = m_currentKey.getAsEvent(m_currentSegmentTime);
    m_currentSegment->insert(keyEvent);

    return true;
}

bool RG21Loader::parseChordItem()
{
    if (m_tokens.count() < 4) return false;
   
    QStringList::Iterator i = m_tokens.begin();
    timeT duration = convertRG21Duration(i);

    // get chord mod flags and nb of notes
    int chordMods = (*i).toInt(); ++i;
    int nbNotes   = (*i).toInt(); ++i;

    vector<string> marks = convertRG21ChordMods(chordMods);

    // now get notes
    for(;i != m_tokens.end(); ++i) {

        long pitch = (*i).toInt();
        ++i;
        int noteMods = (*i).toInt();
        pitch = convertRG21Pitch(pitch, noteMods);

        Event *noteEvent = new Event(Rosegarden::Note::EventType);
        noteEvent->setDuration(duration);
        noteEvent->setAbsoluteTime(m_currentSegmentTime);
        noteEvent->set<Int>(PITCH, pitch);

	if (m_tieStatus == 1) {
	    noteEvent->set<Bool>(TIED_FORWARD, true);
	} else if (m_tieStatus == 2) {
	    noteEvent->set<Bool>(TIED_BACKWARD, true);
	}	    

	if (marks.size() > 0) {
	    noteEvent->set<Int>(MARK_COUNT, marks.size());
	    for (int j = 0; j < marks.size(); ++j) {
		noteEvent->set<String>(getMarkPropertyName(j), marks[j]);
	    }
	}

//         kdDebug(KDEBUG_AREA) << "RG21Loader::parseChordItem() : insert note pitch " << pitch
//                              << " at time " << m_currentSegmentTime << endl;

	setGroupProperties(noteEvent);
        
        m_currentSegment->insert(noteEvent);
    }

    m_currentSegmentTime += duration;
    if (m_tieStatus == 2) m_tieStatus = 0;
    else if (m_tieStatus == 1) m_tieStatus = 2;
    
    return true;
}

bool RG21Loader::parseRest()
{
    if (m_tokens.count() < 2) return false;
   
    QStringList::Iterator i = m_tokens.begin();
    timeT duration = convertRG21Duration(i);
    
    Event *restEvent = new Event(Rosegarden::Note::EventRestType);
    restEvent->setDuration(duration);
    restEvent->setAbsoluteTime(m_currentSegmentTime);

    setGroupProperties(restEvent);

    m_currentSegment->insert(restEvent);
    m_currentSegmentTime += duration;

    return true;
}

void RG21Loader::setGroupProperties(Event *e)
{
    if (m_inGroup) {

	e->setMaybe<Int>(BEAMED_GROUP_ID, m_groupId);
	e->setMaybe<String>(BEAMED_GROUP_TYPE, m_groupType);

	if (m_groupType == GROUP_TYPE_TUPLED) {
	    e->setMaybe<Int>(BEAMED_GROUP_TUPLED_LENGTH, m_groupTupledLength);
	    e->setMaybe<Int>(BEAMED_GROUP_TUPLED_COUNT, m_groupTupledCount);
	}

	m_groupUntupledLength += e->getDuration();
    }
}
           

bool RG21Loader::parseGroupStart()
{
    m_groupType = m_tokens[0].lower();
    m_inGroup = true;
    m_groupId = m_currentSegment->getNextId();
    m_groupStartTime = m_currentSegmentTime;

    if (m_groupType == GROUP_TYPE_BEAMED) {

	// no more to do
        
    } else if (m_groupType == GROUP_TYPE_TUPLED) {

	m_groupTupledLength = m_tokens[1].toUInt() *
	    Note(Note::Hemidemisemiquaver).getDuration();
	m_groupTupledCount = m_tokens[2].toUInt();
	m_groupUntupledLength = 0;

    } else {

	kdDebug(KDEBUG_AREA)
	    << "RG21Loader::parseGroupStart: WARNING: Unknown group type "
	    << m_groupType << ", ignoring" << endl;
	m_inGroup = false;
    }

    return true;
}

bool RG21Loader::parseIndicationStart()
{
    if (m_tokens.count() < 4) return false;
   
    unsigned int indicationId = m_tokens[2].toUInt();
    std::string indicationType = m_tokens[3].lower().latin1();

//    kdDebug(KDEBUG_AREA) << "Indication start: type is \"" << indicationType << "\"" << endl;

    if (indicationType == "tie") {

	if (m_tieStatus != 0) {
	    kdDebug(KDEBUG_AREA)
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
		(*i)->set<Bool>(TIED_FORWARD, true);
		if (i == m_currentSegment->begin()) break;
		--i;
	    }
	}
	m_tieStatus = 2;

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
	setGroupProperties(e);
	m_indicationsExtant[indicationId] = e;

	// place the indication in the segment now; don't wait for the
	// close-indication, because some things may need to know about it
	// before then (e.g. close-group)

	m_currentSegment->insert(e);
    }

    // other indications not handled yet
    return true;
}

void RG21Loader::closeIndication()
{
    if (m_tokens.count() < 3) return;
   
    unsigned int indicationId = m_tokens[2].toUInt();
    EventIdMap::iterator i = m_indicationsExtant.find(indicationId);

    // this is normal (for ties):
    if (i == m_indicationsExtant.end()) return;

    Event *indicationEvent = i->second;
    m_indicationsExtant.erase(i);

    indicationEvent->set<Int>(Indication::IndicationDurationPropertyName,
			m_currentSegmentTime - indicationEvent->getAbsoluteTime());
//!!! see comment in previous method    m_currentSegment->insert(indicationEvent);
}

void RG21Loader::closeGroup()
{
    if (m_groupType == GROUP_TYPE_TUPLED) {

	Segment::iterator i = m_currentSegment->end();

	if (i != m_currentSegment->begin()) {

	    --i;
	    long groupId;
	    timeT prev = m_groupStartTime + m_groupTupledLength;

	    while ((*i)->get<Int>(BEAMED_GROUP_ID, groupId) &&
		   groupId == m_groupId) {

		(*i)->setMaybe<Int>
		    (BEAMED_GROUP_UNTUPLED_LENGTH, m_groupUntupledLength);

		timeT duration = (*i)->getDuration();
		timeT offset = (*i)->getAbsoluteTime() - m_groupStartTime;
		timeT intended =
		    (offset * m_groupTupledLength) / m_groupUntupledLength;
/*
		kdDebug(KDEBUG_AREA)
		    << "RG21Loader::closeGroup:"
		    << " m_groupStartTime = " << m_groupStartTime
		    << ", m_groupTupledLength = " << m_groupTupledLength
		    << ", m_groupUntupledLength = " << m_groupUntupledLength
		    << ", absoluteTime = " << (*i)->getAbsoluteTime()
		    << ", offset = " << offset
		    << ", intended = " << intended
		    << ", new absolute time = " <<
		    ((*i)->getAbsoluteTime() + intended - offset) << endl;
*/
		(*i)->addAbsoluteTime(intended - offset);
		(*i)->setDuration(prev - (*i)->getAbsoluteTime());
		prev = (*i)->getAbsoluteTime();

		(*i)->set<Int>(TUPLET_NOMINAL_DURATION, duration);

		if (i == m_currentSegment->begin()) break;
		--i;
	    }
	}

	m_currentSegmentTime = m_groupStartTime + m_groupTupledLength;
    }

    m_inGroup = false;
}


bool RG21Loader::parseBarType()
{
    if (m_tokens.count() < 5) return false;
    if (!m_composition) return false;

    int staffNo = m_tokens[1].toInt();
    if (staffNo > 0) {
	kdDebug(KDEBUG_AREA)
	    << "RG21Loader::parseBarType: We don't support different time\n"
	    << "signatures on different staffs; disregarding time signature for staff " << staffNo << endl;
	return true;
    }

    int barNo = m_tokens[2].toInt();

    int numerator   = m_tokens[4].toInt();
    int denominator = m_tokens[5].toInt();

    timeT sigTime = m_composition->getBarRange(barNo, true).first;
    TimeSignature timeSig(numerator, denominator);
    m_composition->getReferenceSegment()->insert(timeSig.getAsEvent(sigTime));

    return true;
}


bool RG21Loader::parseStaveType()
{
    // not implemented yet
    return true;
}


timeT RG21Loader::convertRG21Duration(QStringList::Iterator& i)
{
    QString durationString = (*i).lower();
    ++i;
    
    if (durationString == "dotted") {
        durationString += ' ';
        durationString += (*i).lower();
        ++i;
    }

    try {

        Rosegarden::Note n(durationString.latin1());
        return n.getDuration();

    } catch (Rosegarden::Note::BadType b) {

        kdDebug(KDEBUG_AREA) << "RG21Loader::convertRG21Duration: Bad duration: "
                             << durationString << endl;
        return 0;
    }

}


void RG21Loader::closeSegmentOrComposition()
{
    if (m_currentSegment) {
        m_currentSegment->setInstrument(m_currentSegmentNb - 1);
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
        (noteModifier & ModSharp)   ? Sharp :
        (noteModifier & ModFlat)    ? Flat  :
        (noteModifier & ModNatural) ? Natural : NoAccidental;
 
    Rosegarden::NotationDisplayPitch displayPitch(pitch, accidental);

    long rtn = displayPitch.getPerformancePitchFromRG21Pitch(m_currentClef,
							     m_currentKey);

    return rtn;
}

vector<string> RG21Loader::convertRG21ChordMods(int chordMods)
{
    vector<string> marks;

    // bit laborious!
    if (chordMods & ModDot) 
	marks.push_back(Note::getMarkName(Rosegarden::Staccato));

    if (chordMods & ModLegato)
	marks.push_back(Note::getMarkName(Rosegarden::Tenuto));

    if (chordMods & ModAccent)
	marks.push_back(Note::getMarkName(Rosegarden::Accent));

    if (chordMods & ModSfz)
	marks.push_back(Note::getMarkName(Rosegarden::Sforzando));

    if (chordMods & ModRfz) {

	// I'm quite confused by this.  Some rg21 files appear to have
	// chords with mod of 80 (rinforzando plus turn), which is
	// obviously almost always nonsense but appears to be saved in
	// place of a pause (and reloaded "correctly" by rg2.1).  What
	// am I missing here?

	//!!! Aargh!  It's a hex number.  Fix this

	if (chordMods & ModTurn) {
	    chordMods = chordMods ^= ModTurn;
	    chordMods = chordMods |= ModPause;
	} else {
	    marks.push_back(Note::getMarkName(Rosegarden::Rinforzando));
	}
    }

    if (chordMods & ModTrill)
	marks.push_back(Note::getMarkName(Rosegarden::Trill));

    if (chordMods & ModTurn)
	marks.push_back(Note::getMarkName(Rosegarden::Turn));

    if (chordMods & ModPause)
	marks.push_back(Note::getMarkName(Rosegarden::Pause));

    return marks;
}

bool RG21Loader::readNextLine()
{
    bool inComment = false;
    
    do {
        inComment = false;

        m_currentLine = m_stream->readLine();

        if (m_stream->eof()) return false;
        
        m_currentLine = m_currentLine.simplifyWhiteSpace();

        if (m_currentLine[0] == '#' ||
            m_currentLine.length() == 0) {
            inComment = true;
            continue; // skip comments
        }

        m_tokens = QStringList::split(' ', m_currentLine);
        
    } while (inComment);

    return true;
}


bool RG21Loader::parse()
{
    while (!m_stream->eof()) {

        if (!readNextLine()) break;
        
        QString firstToken = m_tokens.first();
        
        if (firstToken == "Staves" || firstToken == "Staffs") { // nb staves

            m_nbStaves = m_tokens[1].toUInt();
            m_composition = new Rosegarden::Composition;

        } else if (firstToken == "Name") { // Staff name

            m_currentStaffName = m_tokens[1]; // we don't do anything with it yet
            m_currentSegment = new Rosegarden::Segment;
            ++m_currentSegmentNb;
            
        } else if (firstToken == "Clef") {

            parseClef();

        } else if (firstToken == "Key") {

            parseKey();

        } else if (firstToken == ":") { // chord

            m_tokens.remove(m_tokens.begin()); // get rid of 1st token ':'
            parseChordItem();

        } else if (firstToken == "Rest") { // rest

            if (!readNextLine()) break;

            parseRest();

        } else if (firstToken == "Group") {

            if (!readNextLine()) break;

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
                closeSegmentOrComposition();
            
        }
        
    }
    
    return true;
}
