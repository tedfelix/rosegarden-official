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
#include "Track.h"
#include "TrackNotationHelper.h"
#include "Composition.h"
#include "NotationTypes.h"

#include "rg21io.h"
#include "rosedebug.h"

using Rosegarden::Event;
using Rosegarden::Track;
using Rosegarden::TrackNotationHelper;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Clef;
using Rosegarden::Accidental;
using Rosegarden::Sharp;
using Rosegarden::Flat;
using Rosegarden::Natural;
using Rosegarden::NoAccidental;

RG21Loader::RG21Loader(const QString& fileName)
    : m_file(fileName),
      m_stream(0),
      m_composition(0),
      m_currentTrack(0),
      m_currentTrackTime(0),
      m_currentTrackNb(0),
      m_currentClef(Clef::Treble),
      m_inGroup(false),
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
    if (m_tokens.count() != 3 || !m_currentTrack) return false;
    
    std::string clefName = m_tokens[2].lower().data();

    m_currentClef = Clef(clefName);
    Event *clefEvent = m_currentClef.getAsEvent(m_currentTrackTime);
    m_currentTrack->insert(clefEvent);

    return true;
}

bool RG21Loader::parseKey()
{
    if (m_tokens.count() < 3 || !m_currentTrack) return false;
    
    QString keyName = QString("%1 %2or")
        .arg(m_tokens[2].upper())
        .arg(m_tokens[3].lower());

    m_currentKey = Rosegarden::Key(std::string(keyName.data()));
    Event *keyEvent = m_currentKey.getAsEvent(m_currentTrackTime);
    m_currentTrack->insert(keyEvent);

    return true;
}

bool RG21Loader::parseChordItem()
{
    if (m_tokens.count() < 4) return false;
   
    QStringList::Iterator i = m_tokens.begin();
    Rosegarden::timeT duration = convertRG21Duration(i);

    // get chord mod flags and nb of notes
    int chordMods = (*i).toInt(); ++i;
    int nbNotes   = (*i).toInt(); ++i;

    // now get notes
    for(;i != m_tokens.end(); ++i) {

        long pitch = (*i).toInt();
        ++i;
        int noteMods = (*i).toInt();
        pitch = convertRG21Pitch(pitch, noteMods);

        Event *noteEvent = new Event(Rosegarden::Note::EventType);
        noteEvent->setDuration(duration);
        noteEvent->setAbsoluteTime(m_currentTrackTime);
        noteEvent->set<Int>("pitch", pitch);

//         kdDebug(KDEBUG_AREA) << "RG21Loader::parseChordItem() : insert note pitch " << pitch
//                              << " at time " << m_currentTrackTime << endl;

        if (m_inGroup) {

            noteEvent->setMaybe<Int>
                (TrackNotationHelper::BeamedGroupIdPropertyName, m_groupId);
            noteEvent->setMaybe<String>
                (TrackNotationHelper::BeamedGroupTypePropertyName, m_groupType);
	    if (m_groupType == "tupled") { //!!! Should be converting to a property value, but there is no property value for this yet (see notationsets.cpp and TrackNotationHelper.C)
		noteEvent->set/*!!! Maybe */<Int> // should be Maybe ultimately
		    (TrackNotationHelper::BeamedGroupTupledLengthPropertyName,
		     m_groupTupledLength);
		noteEvent->set/*!!! Maybe */<Int>
		    (TrackNotationHelper::BeamedGroupTupledCountPropertyName,
		     m_groupTupledCount);
	    }
        }
        

        m_currentTrack->insert(noteEvent);
    }

    m_currentTrackTime += duration;

    return true;
}

bool RG21Loader::parseRest()
{
    if (m_tokens.count() < 2) return false;
   
    QStringList::Iterator i = m_tokens.begin();
    Rosegarden::timeT duration = convertRG21Duration(i);
    
    Event *restEvent = new Event(Rosegarden::Note::EventRestType);
    restEvent->setDuration(duration);
    restEvent->setAbsoluteTime(m_currentTrackTime);
    m_currentTrack->insert(restEvent);
    m_currentTrackTime += duration;

    return true;
}

bool RG21Loader::parseGroupStart()
{
    m_groupType = m_tokens[0].lower();
    m_inGroup = true;
    m_groupId = m_currentTrack->getNextId();

    if (m_groupType == "beamed") {

	// no more to do
        
    } else if (m_groupType == "tupled") {

	m_groupTupledLength = m_tokens[1].toUInt();
	m_groupTupledCount = m_tokens[2].toUInt();

    } else {

	kdDebug(KDEBUG_AREA)
	    << "RG21Loader::parseGroupStart: WARNING: Unknown group type "
	    << m_groupType << ", ignoring" << endl;
	m_inGroup = false;
    }

    return true;
}

bool RG21Loader::parseMarkStart()
{
    // not handled yet
    return true;
}

void RG21Loader::closeMark()
{
}

void RG21Loader::closeGroup()
{
    m_inGroup = false;
}


Rosegarden::timeT RG21Loader::convertRG21Duration(QStringList::Iterator& i)
{
    QString durationString = (*i).lower();
    ++i;
    
    if (durationString == "dotted") {
        durationString += ' ';
        durationString += (*i).lower();
        ++i;
    }

    try {

	kdDebug(KDEBUG_AREA) << "durationString is \"" << durationString << "\"" << endl;
        Rosegarden::Note n(durationString.latin1());
	kdDebug(KDEBUG_AREA) << "Note duration is " << n.getDuration() << endl;
        return n.getDuration();

    } catch (Rosegarden::Note::BadType b) {

        kdDebug(KDEBUG_AREA) << "RG21Loader::convertRG21Duration: Bad duration: "
                             << durationString << endl;
        return 0;
    }

}


void RG21Loader::closeTrackOrComposition()
{
    if (m_currentTrack) {
        m_currentTrack->setInstrument(m_currentTrackNb - 1);
        m_composition->addTrack(m_currentTrack);
        m_currentTrack = 0;
        m_currentTrackTime = 0;
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
            m_currentTrack = new Rosegarden::Track;
            ++m_currentTrackNb;
            
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
                parseMarkStart();
            else if (m_tokens[1] == "end") 
                closeMark();

        } else if (firstToken == "End") {

            if (m_inGroup)
                closeGroup();
            else
                closeTrackOrComposition();
            
        }
        
    }
    
    return true;
}
