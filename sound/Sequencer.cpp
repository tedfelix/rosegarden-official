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


#include <iostream>

#include "Sequencer.h"

//#define _WITH_ALSA_

#ifdef _WITH_ALSA_
#include "AlsaDriver.h"
#else
#include "ArtsDriver.h"
#endif

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

// Create a driver depending on what we have enabled.
// Initialisation of the driver is performed at construction
//
//
Sequencer::Sequencer()
{
#ifdef _WITH_ALSA_
    m_soundDriver = new AlsaDriver();
#else
    m_soundDriver = new ArtsDriver();
#endif

    m_soundDriver->initialiseMidi();
    m_soundDriver->initialiseAudio();
}


Sequencer::~Sequencer()
{
    delete m_soundDriver;
}


bool
Sequencer::addAudioFile(const std::string &fileName, const unsigned int &id)
{
    AudioFile *ins = new AudioFile(id, fileName, fileName);
    try
    {
        ins->open();
    }
    catch(std::string s)
    {
        return false;
    }

    m_audioFiles.push_back(ins);

    cout << "Sequencer::addAudioFile() = \"" << fileName << "\"" << endl;

    return true;
}

bool
Sequencer::removeAudioFile(const unsigned int &id)
{
    std::vector<AudioFile*>::iterator it = getAudioFile(id);

    if(it == m_audioFiles.end())
        return false;

    m_audioFiles.erase(it);

    return true;
}

std::vector<AudioFile*>::iterator
Sequencer::getAudioFile(const unsigned int &id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getID() == id)
            return it;
    }

    return m_audioFiles.end();
}


void
Sequencer::clearAudioFiles()
{
    cout << "Sequencer::clearAudioFiles() - clearing down audio files" << endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
        delete(*it);

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());
}


bool
Sequencer::queueAudio(const unsigned int &id,
                      const RealTime &absoluteTime,
                      const RealTime &audioStartMarker,
                      const RealTime &duration,
                      const RealTime &playLatency)
{
    std::vector<AudioFile*>::iterator it = getAudioFile(id);

    if (it == m_audioFiles.end())
        return false;

    std::cout << "queueAudio() - queuing Audio event at time "
              << absoluteTime + playLatency << std::endl;

    // register the AudioFile in the playback queue
    //
    PlayableAudioFile *newAF =
                         new PlayableAudioFile(*it,
                                               absoluteTime + playLatency,
                                               audioStartMarker - absoluteTime,
                                               duration);
                                               //m_soundServer);

    m_soundDriver->queueAudio(newAF);

    return true;
}



}


