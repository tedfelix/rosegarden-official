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

#include "AudioFilePlayer.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

AudioFilePlayer::AudioFilePlayer(Rosegarden::Sequencer *sequencer):
    m_sequencer(sequencer)
{
    assert(m_sequencer != 0);
}


AudioFilePlayer::~AudioFilePlayer()
{
}


bool
AudioFilePlayer::addAudioFile(const string &fileName,
                              const unsigned int &id)
{
    AudioFile *ins = new AudioFile(id, fileName, fileName);
    try
    {
        ins->open();
    }
    catch(string s)
    {
        return false;
    }

    m_audioFiles.push_back(ins);

    cout << "AudioFilePlayer::addAudioFile() = \"" << fileName << "\"" << endl;

    return true;
}

bool
AudioFilePlayer::deleteAudioFile(const unsigned int &id)
{
    std::vector<AudioFile*>::iterator it = getAudioFile(id);

    if(it == 0)
        return false;

    m_audioFiles.erase(it);

    return true;
}

std::vector<AudioFile*>::iterator
AudioFilePlayer::getAudioFile(const unsigned int &id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getID() == id)
            return it;
    }

    return 0;
}


void
AudioFilePlayer::clear()
{
    cout << "AudioFilePlayer::clear() - clearing down audio files" << endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        m_audioFiles.erase(it);
    }

}


bool
AudioFilePlayer::playAudio(const unsigned int &id, const RealTime startIndex,
                           const RealTime duration)
{
    std::vector<AudioFile*>::iterator it = getAudioFile(id);

    if (it == 0)
        return false;

    // send the resultant AudioFile to the Sequencer for playback
    //
    m_sequencer->playAudioFile(*it, startIndex, duration);

    return true;
}



}

