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

AudioFilePlayer::AudioFilePlayer()
{
}


AudioFilePlayer::~AudioFilePlayer()
{
}


bool
AudioFilePlayer::addAudioFile(const AudioFileType &audioFileType,
                              const string &fileName,
                              const int &id)
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
AudioFilePlayer::deleteAudioFile(const int &id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getID() == id)
        {
            m_audioFiles.erase(it);
            return true;
        }
    }

    return false;
}

void
AudioFilePlayer::clear()
{
    cout << "AudioFilePlayer::clear() - clearing down audio files" << endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
        m_audioFiles.erase(it);

}


bool
AudioFilePlayer::playAudio(const int &id, const RealTime startIndex,
                           const RealTime duration)
{
    return true;
}



}

