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
#include "RealTime.h"

using std::cout;
using std::cerr;
using std::endl;


namespace Rosegarden
{

AudioFile::AudioFile(const unsigned int &id,
                     const std::string &name, const std::string &fileName):
    SoundFile(fileName), m_id(id), m_name(name),
    m_bitsPerSample(0), m_sampleRate(0), m_bytesPerSecond(0),
    m_bytesPerSample(0), m_channels(0), m_type(AUDIO_NOT_LOADED),
    m_fileSize(0), m_file(0)
{
}

AudioFile::AudioFile(const std::string &fileName):
    SoundFile(fileName), m_id(0), m_name(""),
    m_bitsPerSample(0), m_sampleRate(0), m_bytesPerSecond(0),
    m_bytesPerSample(0), m_channels(0), m_type(AUDIO_NOT_LOADED),
    m_fileSize(0), m_file(0)
{
}

AudioFile::~AudioFile()
{
    if (m_file)
    {
        m_file->close();
        delete m_file;
    }
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
        throw((std::string("AudioFile::parseHeader - can't find RIFF identifier")));
    }

    // Look for the WAV identifier
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_WAVE_ID, 8, 4) != 0)
#else
    if (hS.compare(4, 4, Rosegarden::AUDIO_WAVE_ID) != 0)
#endif
    {
        throw((std::string("AudioFile::parseHeader - can't find WAV identifier")));
    }

    // Look for the FORMAT identifier - note that this doesn't actually
    // have to be in the first chunk we come across, but for the moment
    // this is the only place we check for it because I'm lazy.
    //
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_FORMAT_ID, 12, 4) != 0)
#else
    if (hS.compare(4, 4, Rosegarden::AUDIO_FORMAT_ID) != 0)
#endif
    {
        throw((std::string("AudioFile::parseHeader - can't find FORMAT identifier")));
    }

    // Little endian conversion of length bytes into file length
    // (add on eight for RIFF id and length field and compare to 
    // real file size).
    //
    unsigned int length = getLittleEndian(hS.substr(4,4)) + 8;

    if (length != m_fileSize)
        throw(std::string("AudioFile::parseHeader - file " + m_fileName +
                     " corrupted (wrong length)"));

    // Check the format length (always 0x10)
    //
    unsigned int lengthOfFormat = getLittleEndian(hS.substr(16, 4));

    if (lengthOfFormat != 0x10)
        throw(std::string("AudioFile::parseHeader - format length incorrect"));


    // Check this field is one
    //
    unsigned int alwaysOne = getLittleEndian(hS.substr(20, 2));
    if (alwaysOne != 0x01)
        throw(std::string("AudioFile::parseHeader - \"always one\" byte isn't"));


    // We seem to have a good looking .WAV file - extract the
    // sample information and populate this locally
    //
    unsigned int channelNumbers =  getLittleEndian(hS.substr(22,2));
    
    switch(channelNumbers)
    {
        case 0x01:
        case 0x02:
            m_channels = channelNumbers;
            break;

        default:
            {
                throw(std::string("AudioFile::parseHeader - unsupport number of channels"));
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
    m_file = new std::ifstream(m_fileName.c_str(),
                               std::ios::in | std::ios::binary);

    // get the actual file size
    //
    m_file->seekg(0, std::ios::end);
    m_fileSize = m_file->tellg();
    m_file->seekg(0, std::ios::beg);

    // now parse the file
    try
    {
        if (*m_file)
        {
            try
            {
                parseHeader(getBytes(m_file, 36));
            }
            catch(std::string s)
            {
                cerr << "EXCEPTION : " << s << endl;
            }
        }
        else
        {
            m_type = AUDIO_NOT_LOADED;
            return (false);
        }
    }
    catch(std::string s)
    {  
        cout << "EXCEPTION : " << s << endl;
        return false;
    }

    m_type = AUDIO_WAV;

    // Reset to front of "data" block
    //
    scanTo(m_file, RealTime(0, 0));

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
         << "channels        : " << m_channels << endl
         << endl;
}

bool
AudioFile::write()
{
    return true;
}

bool
AudioFile::scanTo(std::ifstream *file, const RealTime &time)
{
    // sanity
    if (file == 0) return false;

    // seek past header
    file->seekg(36, std::ios::beg);

    // check we've got data chunk start
    if (getBytes(file, 4) != "data")
    {
        std::cerr << "AudioFile::scanTo() - can't find data chunk where "
                  << "it was expected" << std::endl;
        return false;
    }

    // How much do we scan forward?
    //
    unsigned int totalSamples = m_sampleRate * time.sec +
                        ( ( m_sampleRate * time.usec ) / 1000000 );

    unsigned int totalBytes = totalSamples * m_channels * m_bytesPerSample;


    // When using seekg we have to keep an eye on the boundaries ourselves
    //
    if (totalBytes > m_fileSize - 40)
    {
        std::cerr << "AudioFile::scanTo() - attempting to move past end of "
                  << "data block" << std::endl;
        return false;
    }

    file->seekg(totalBytes,  std::ios::cur);

    std::cout << "AudioFile::scanTo - seeking to " << time
              << "(" << totalBytes << " bytes)" << std::endl;

    return true;
}

// Get a certain number of sample frames - a frame is a set
// of samples (all channels) for a given sample quanta.
//
// For example, getting one frame of 16-bit stereo will return
// four bytes of data (two per channel).
//
//
std::string
AudioFile::getSampleFrames(std::ifstream *file, unsigned int frames)
{
    // sanity
    if (file == 0) return std::string("");

    // Bytes per sample already takes into account the number
    // of channels we're using
    //
    long totalBytes = frames * m_bytesPerSample;
    return getBytes(file, totalBytes);
}

// Return a slice of frames over a time period
//
std::string
AudioFile::getSampleFrameSlice(std::ifstream *file, const RealTime &time)
{
    // sanity
    if (file == 0) return std::string("");

    long totalSamples = m_sampleRate * time.sec +
                        ( ( m_sampleRate * time.usec ) / 1000000 );

    long totalBytes = totalSamples * m_channels * m_bytesPerSample;
    return getBytes(file, totalBytes);
}


}


