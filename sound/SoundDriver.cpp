// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.1
  A sequencer and musical notation editor.

  This program is Copyright 2000-2002
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


#include "SoundDriver.h"

namespace Rosegarden
{

SoundDriver::SoundDriver(const std::string &name):
    m_name(name),
    m_driverStatus(NO_DRIVER),
    m_playStartPosition(0, 0),
    m_startPlayback(false),
    m_playing(false),
    m_recordStatus(ASYNCHRONOUS_MIDI),
    m_midiRunningId(MidiInstrumentBase),
    m_audioRunningId(AudioInstrumentBase)

{
}


SoundDriver::~SoundDriver()
{
}

MappedInstrument*
SoundDriver::getMappedInstrument(InstrumentId id)
{
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == id)
            return (*it);
    }

    return 0;
}

void
SoundDriver::queueAudio(PlayableAudioFile *audioFile)
{
    m_audioPlayQueue.push_back(audioFile);
}

void
SoundDriver::setMappedInstrument(MappedInstrument *mI)
{
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == mI->getId())
        {
            (*it)->setChannel(mI->getChannel());
            (*it)->setType(mI->getType());
            return;
        }
    }

    // else create a new one
    m_instruments.push_back(mI);

    std::cout << "SoundDriver: setMappedInstrument() : "
              << "type = " << mI->getType() << " : "
              << "channel = " << (int)(mI->getChannel()) << " : "
              << "id = " << mI->getId() << std::endl;


}

}

