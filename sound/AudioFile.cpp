// -*- c-basic-offset: 4 -*-

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

#include "Audio.h"
#include "AudioFile.h"

using std::cout;
using std::cerr;
using std::endl;


namespace Rosegarden
{

AudioFile::AudioFile(const int &id, const string &name, const string &fileName):
    SoundFile(fileName), m_id(id), m_name(name),
    m_bits(0), m_resolution(0), m_stereo(false),
    m_type(AUDIO_NOT_LOADED)
{
}

AudioFile::~AudioFile()
{
}


void
AudioFile::parseHeader(const std::string &hS)
{
    // The WAV file itself consists of three "chunks" of information:
    // The RIFF chunk which identifies the file as a WAV file, The FORMAT
    // chunk which identifies parameters such as sample rate and the DATA
    // chunk which contains the actual data (samples). 


    // Look for the RIFF identifier and bomb out if we don't find it
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_RIFF_ID, 0, 4) != 0)
#else
    if (hS.compare(0, 4, Rosegarden::AUDIO_RIFF_ID) != 0)
#endif
    {
        throw((string("AudioFile::parseHeader - can't find RIFF identifier")));
    }

    // Look for the WAV identifier
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_WAVE_ID, 8, 4) != 0)
#else
    if (hS.compare(4, 4, Rosegarden::AUDIO_WAVE_ID) != 0)
#endif
    {
        throw((string("AudioFile::parseHeader - can't find WAV identifier")));
    }

    return true;
}

bool
AudioFile::open()
{
    ifstream *file = new ifstream(m_fileName.c_str(), ios::in | ios::binary);

    try
    {
        if (*file)
        {
            try
            {
                parseHeader(getBytes(file, 36));
            }
            catch(string s)
            {
                cerr << "EXCEPTION : " << s << endl;
            }
        }
        else
        {
            m_type = AUDIO_NOT_LOADED;
            return (false);
        }
        file->close();
    }
    catch(string s)
    {  
        cout << "EXCEPTION : " << s << endl;
    }

    return true;
}

bool
AudioFile::write()
{
    return true;
}

}


