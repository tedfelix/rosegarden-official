
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
#include "Composition.h"
#include "NotationTypes.h"

#include "rg21io.h"
#include "rosedebug.h"

using Rosegarden::Event;
using Rosegarden::Track;

RG21Loader::RG21Loader(const QString& fileName)
    : m_file(fileName),
      m_stream(0),
      m_composition(0),
      m_currentTrack(0),
      m_currentTrackTime(0),
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

    Event *clefEvent = new Event(Rosegarden::Clef::EventType);
    clefEvent->setAbsoluteTime(0);
    clefEvent->set<Rosegarden::String>(Rosegarden::Clef::ClefPropertyName,
                                       clefName);
    
    m_currentTrack->insert(clefEvent);

    return true;
}

bool RG21Loader::parseChordItem()
{
    using Rosegarden::Note;

    if (m_tokens.count() < 4) return false;
    
    QString durationString = m_tokens[0].lower();
    Rosegarden::timeT duration = 0;

    try {
        Note n(durationString.latin1());
        duration = n.getDuration();
    } catch (Note::BadType b) {
        kdDebug(KDEBUG_AREA) << "RG21Loader::parseChordItem: Bad duration: "
                             << durationString << endl;
    }

}

bool RG21Loader::parse()
{
    while (!m_stream->eof()) {
        m_currentLine = m_stream->readLine();
        m_currentLine = m_currentLine.simplifyWhiteSpace();

        if (m_currentLine[0] == '#' ||
            m_currentLine.length() == 0) continue; // skip comments

        m_tokens = QStringList::split(' ', m_currentLine);

        QString firstToken = m_tokens.first();
        
        if (firstToken == "Staves" || firstToken == "Staffs") { // nb staves
            m_nbStaves = m_tokens[1].toUInt();
            
        } else if (firstToken == "Name") { // Staff name
            m_currentStaffName = m_tokens[1]; // we don't do anything with it yet
            m_currentTrack = new Rosegarden::Track;

        } else if (firstToken == "Clef") {
            parseClef();
        } else if (firstToken == ":") {
            m_tokens.remove(m_tokens.begin()); // get rid of 1st token ':'
            parseChordItem();
        }

    }
    
    return true;
}
