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


#include "MidiRecord.h"
#include "MidiArts.h"

namespace Rosegarden
{

using std::vector;

RosegardenMidiRecord_impl::RosegardenMidiRecord_impl(): m_record(false)
{
    m_midiEventQueue = new vector<Arts::MidiEvent>();
}

RosegardenMidiRecord_impl::~RosegardenMidiRecord_impl()
{
    delete m_midiEventQueue;
}


void
RosegardenMidiRecord_impl::processCommand(const Arts::MidiCommand &midiCommand)
{
    m_midiThru.processCommand(midiCommand);
    Arts::TimeStamp ts = m_midiThru.time();
    addToList(Arts::MidiEvent(ts, midiCommand));
}

void
RosegardenMidiRecord_impl::processEvent(const Arts::MidiEvent &midiEvent)
{
  
    m_midiThru.processEvent(midiEvent);
    addToList(midiEvent);
}

void
RosegardenMidiRecord_impl::addToList(const Arts::MidiEvent &midiEvent)
{
    if (m_record == true)
    {
	m_midiEventQueue->push_back(midiEvent);
    }
}

vector<Arts::MidiEvent> *
RosegardenMidiRecord_impl::getQueue()
{
    vector<Arts::MidiEvent> *returnQueue = m_midiEventQueue;
    m_midiEventQueue = new vector<Arts::MidiEvent>();
    return returnQueue;
}

REGISTER_IMPLEMENTATION(RosegardenMidiRecord_impl);

}

