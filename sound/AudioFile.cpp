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
    m_bitsPerSample(0), m_sampleRate(0), m_bytesPerSecond(0),
    m_bytesPerSample(0), m_stereo(false), m_type(AUDIO_NOT_LOADED),
    m_fileSize(0)
{
}

AudioFile::~AudioFile()
{
}


void
AudioFile::parseHeader(const std::string &hS)
{
    // Courtesy of:
    //   http://www.technology.niagarac.on.ca/courses/comp630/WavFileFormat.html
    //
    // 'The WAV file itself consists of three "chunks" of information:
    //  The RIFF chunk which identifies the file as a WAV file, The FORMAT
    //  chunk which identifies parameters such as sample rate and the DATA
    //  chunk which contains the actual data (samples).'
    //
    //

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

    // Look for the FORMAT identifier
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_FORMAT_ID, 12, 4) != 0)
#else
    if (hS.compare(4, 4, Rosegarden::AUDIO_FORMAT_ID) != 0)
#endif
    {
        throw((string("AudioFile::parseHeader - can't find FORMAT identifier")));
    }

    // Little endian conversion of length bytes into file length
    // (add on eight for RIFF id and length field and compare to 
    // real file size).
    //
    int length = getLittleEndian(hS.substr(4,4)) + 8;

    if (length != m_fileSize)
        throw(string("AudioFile::parseHeader - file " + m_fileName +
                     " corrupted (wrong length)"));

    // Check the format length (always 0x10)
    //
    int lengthOfFormat = getLittleEndian(hS.substr(16, 4));

    if (lengthOfFormat != 0x10)
        throw(string("AudioFile::parseHeader - format length incorrect"));


    // Check this field is one
    //
    int alwaysOne = getLittleEndian(hS.substr(20, 2));
    if (alwaysOne != 0x01)
        throw(string("AudioFile::parseHeader - always one byte isn't"));


    // We seem to have a good looking .WAV file - extract the
    // sample information and populate this locally
    //
    int channelNumbers =  getLittleEndian(hS.substr(22,2));
    
    switch(channelNumbers)
    {
        case 0x01:
            m_stereo = false;
            break;

        case 0x02:
            m_stereo = true;
            break;

        default:
            {
                throw(string("AudioFile::parseHeader - unrecognised number of channels"));
            }
            break;
    }

    // Now the rest of the information
    //
    m_sampleRate = getLittleEndian(hS.substr(24,4));
    m_bytesPerSecond = getLittleEndian(hS.substr(28,4));
    m_bytesPerSample = getLittleEndian(hS.substr(32,2));
    m_bitsPerSample = getLittleEndian(hS.substr(34,2));
   
}

bool
AudioFile::open()
{
    ifstream *file = new ifstream(m_fileName.c_str(), ios::in | ios::binary);

    // get the actual file size
    //
    file->seekg(0, ios::end);
    m_fileSize = file->tellg();
    file->seekg(0, ios::beg);

    // now parse the file
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

// Show some stats on this file
//
void
AudioFile::printStats()
{
    cout << "filename        : " << m_fileName  << endl
         << "number of bits  : " << m_bitsPerSample << endl
         << "sample rate     : " << m_sampleRate << endl
         << "file length     : " << m_fileSize << " bytes" << endl
         << "stereo          : " << ( m_stereo ? "Yes" : "No" ) << endl
         << endl;
}

bool
AudioFile::write()
{
    return true;
}

}


