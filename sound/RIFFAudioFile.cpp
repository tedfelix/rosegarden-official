// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include "RIFFAudioFile.h"
#include "RealTime.h"
#include "Sound.h"

using std::cout;
using std::cerr;
using std::endl;


namespace Rosegarden
{

RIFFAudioFile::RIFFAudioFile(unsigned int id,
                             const std::string &name,
                             const std::string &fileName):
    AudioFile(id, name, fileName)
{
}

RIFFAudioFile::RIFFAudioFile(const std::string &fileName,
                             unsigned int channels = 1,
                             unsigned int sampleRate = 48000,
                             unsigned int bytesPerSecond = 6000,
                             unsigned int bytesPerSample = 2,
                             unsigned int bitsPerSample = 16):
    AudioFile(0, "", fileName)
{
    m_bitsPerSample = bitsPerSample;
    m_sampleRate = sampleRate;
    m_bytesPerSecond = bytesPerSecond;
    m_bytesPerSample = bytesPerSample;
    m_channels = channels;
}

RIFFAudioFile::~RIFFAudioFile()
{
}


// Show some stats on this file
//
void
RIFFAudioFile::printStats()
{
    cout << "filename         : " << m_fileName  << endl
         << "channels         : " << m_channels << endl
         << "sample rate      : " << m_sampleRate << endl
         << "bytes per second : " << m_bytesPerSecond << endl
         << "bits per sample  : " << m_bitsPerSample << endl
         << "bytes per sample : " << m_bytesPerSample << endl
         << "file length      : " << m_fileSize << " bytes" << endl
         << endl;
}

bool
RIFFAudioFile::appendSamples(const std::string &buffer)
{
    /*
    if (m_outFile == 0 || m_type != WAV)
        return false;
        */

    // write out
    putBytes(m_outFile, buffer);

    return true;
}

// scan on from a descriptor position
bool
RIFFAudioFile::scanForward(std::ifstream *file, const RealTime &time)
{
    // sanity
    if (file == 0) return false;

    unsigned int totalSamples = m_sampleRate * time.sec +
                        ( ( m_sampleRate * time.usec ) / 1000000 );
    unsigned int totalBytes = totalSamples * m_bytesPerSample;

    m_loseBuffer = true;

    // do the seek
    file->seekg(totalBytes, std::ios::cur);

    if (file->get() == EOF)
        return false;

    return true;
}

bool
RIFFAudioFile::scanForward(const RealTime &time)
{
    if (*m_inFile)
        return scanForward(m_inFile, time);
    else
        return false;
}

bool
RIFFAudioFile::scanTo(const RealTime &time)
{
    if (*m_inFile)
        return scanTo(m_inFile, time);
    else
        return false;

}

bool
RIFFAudioFile::scanTo(std::ifstream *file, const RealTime &time)
{
    // sanity
    if (file == 0) return false;

    // whatever we do here we invalidate the read buffer
    //
    m_loseBuffer = true;

    // seek past header - don't hardcode this - use the file format
    // spec to get header length and then scoot to that.
    //
    file->seekg(16, std::ios::beg);
    unsigned int lengthOfFormat = getIntegerFromLittleEndian(getBytes(file, 4));
    file->seekg(lengthOfFormat, std::ios::cur);

    try
    {
        // check we've got data chunk start
        if (getBytes(file, 4) != "data")
        {
#ifdef DEBUG_RIFF
            std::cerr << "RIFFAudioFile::scanTo() - can't find data chunk where "
                      << "it was expected" << std::endl;
#endif
            return false;
        }

        // get the length of the data chunk
#ifdef DEBUG_RIFF
        std::cout << "RIFFAudioFile::scanTo() - data chunk size = "
                  << getIntegerFromLittleEndian(getBytes(file, 4)) << std::endl;
#endif

    }
    catch(std::string s)
    {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::scanTo - EXCEPTION - \""
                  << s << "\"" << std::endl;
#endif
        return false;
    }
        
    // Ok, we're past all the header information in the data chunk.
    // Now, how much do we scan forward?
    //
    unsigned int totalSamples = m_sampleRate * time.sec +
       ((unsigned int)((double(m_sampleRate) * double(time.usec)) / 1000000.0));

    unsigned int totalBytes = totalSamples * m_channels * m_bytesPerSample;

    // When using seekg we have to keep an eye on the boundaries ourselves
    //
    if (totalBytes > m_fileSize - (lengthOfFormat + 16 + 8))
    {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::scanTo() - attempting to move past end of "
                  << "data block" << std::endl;
#endif
        return false;
    }

    file->seekg(totalBytes,  std::ios::cur);

#ifdef DEBUG_RIFF
    std::cout << "RIFFAudioFile::scanTo - seeking to " << time
              << " (" << totalBytes << " bytes)" << std::endl;
#endif

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
RIFFAudioFile::getSampleFrames(std::ifstream *file, unsigned int frames)
{
    // sanity
    if (file == 0) return std::string("");

    // Bytes per sample already takes into account the number
    // of channels we're using
    //
    long totalBytes = frames * m_bytesPerSample;
    return getBytes(file, totalBytes);
}

std::string
RIFFAudioFile::getSampleFrames(unsigned int frames)
{
    if (*m_inFile) {
        return getSampleFrames(m_inFile, frames);
    } else {
        return std::string("");
    }
}

// Return a slice of frames over a time period
//
std::string
RIFFAudioFile::getSampleFrameSlice(std::ifstream *file, const RealTime &time)
{
    // sanity
    if (file == 0) return std::string("");

    long totalSamples = m_sampleRate * time.sec +
                        ( ( m_sampleRate * time.usec ) / 1000000 );

    long totalBytes = totalSamples * m_channels * m_bytesPerSample;
    return getBytes(file, totalBytes);
}

std::string
RIFFAudioFile::getSampleFrameSlice(const RealTime &time)
{
    if (*m_inFile) {
        return getSampleFrameSlice(m_inFile, time);
    } else {
        return std::string("");
    }
}    

RealTime
RIFFAudioFile::getLength()
{
    // Fixed header size = 44 but prove by getting it from the file too
    //
    unsigned int headerLength = 44;

    if (m_inFile)
    {
        m_inFile->seekg(16, std::ios::beg);
        headerLength = getIntegerFromLittleEndian(getBytes(m_inFile, 4));
        m_inFile->seekg(headerLength, std::ios::cur);
        headerLength += (16 + 8);
    }

    // bytesPerSample allows for number of channels
    //
    double frames = (m_fileSize - headerLength)/m_bytesPerSample;
    double seconds = frames / ((double)m_sampleRate);

    int secs = int(seconds);
    int usecs = int((seconds - secs) * 1000000.0);

    return RealTime(secs, usecs);
}


// The RIFF file format chunk defines our internal meta data.
//
// Courtesy of:
//   http://www.technology.niagarac.on.ca/courses/comp630/WavFileFormat.html
//
// 'The WAV file itself consists of three "chunks" of information:
//  The RIFF chunk which identifies the file as a WAV file, The FORMAT
//  chunk which identifies parameters such as sample rate and the DATA
//  chunk which contains the actual data (samples).'
//
//
void
RIFFAudioFile::readFormatChunk()
{
    if (m_inFile == 0)
        return;

    m_loseBuffer = true;

    // seek to beginning
    m_inFile->seekg(0, std::ios::beg);

    // get the header string
    //
    std::string hS = getBytes(36);

    // Look for the RIFF identifier and bomb out if we don't find it
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_RIFF_ID, 0, 4) != 0)
#else
    if (hS.compare(0, 4, Rosegarden::AUDIO_RIFF_ID) != 0)
#endif
    {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::readFormatChunk - "
                  << "can't find RIFF identifier\n";
#endif
        throw((std::string("RIFFAudioFile::readFormatChunk - can't find RIFF identifier")));
    }

    // Look for the WAV identifier
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_WAVE_ID, 8, 4) != 0)
#else
    if (hS.compare(8, 4, Rosegarden::AUDIO_WAVE_ID) != 0)
#endif
    {
#ifdef DEBUG_RIFF
        std::cerr << "Can't find WAV identifier\n";
#endif
        throw((std::string("Can't find WAV identifier")));
    }

    // Look for the FORMAT identifier - note that this doesn't actually
    // have to be in the first chunk we come across, but for the moment
    // this is the only place we check for it because I'm lazy.
    //
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_FORMAT_ID, 12, 4) != 0)
#else
    if (hS.compare(12, 4, Rosegarden::AUDIO_FORMAT_ID) != 0)
#endif
    {
#ifdef DEBUG_RIFF
        std::cerr << "Can't find FORMAT identifier\n";
#endif
        throw((std::string("Can't find FORMAT identifier")));
    }

    // Little endian conversion of length bytes into file length
    // (add on eight for RIFF id and length field and compare to 
    // real file size).
    //
    unsigned int length = getIntegerFromLittleEndian(hS.substr(4,4)) + 8;

    if (length != m_fileSize)
    {
        char value[80];
        sprintf(value, "(read %u when size is %u)", length, m_fileSize);
        throw(std::string("\"" + m_fileName +
              "\" corrupted (wrong length) - " +
              std::string(value)));
    }

    // Check the format length
    //
    unsigned int lengthOfFormat = getIntegerFromLittleEndian(hS.substr(16, 4));

    // Make sure we step to the end of the format chunk ignoring the
    // tail if it exists
    //
    if (lengthOfFormat > 0x10)
    {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::readFormatChunk - "
                  << "extended Format Chunk (" << lengthOfFormat << ")"
                  << std::endl;
#endif

        // ignore any overlapping bytes 
        m_inFile->seekg(lengthOfFormat - 0x10, std::ios::cur);
    }
    else if (lengthOfFormat < 0x10)
    {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::readFormatChunk - "
                  << "truncated Format Chunk (" << lengthOfFormat << ")"
                  << std::endl;
#endif

        m_inFile->seekg(lengthOfFormat - 0x10, std::ios::cur);
        //throw(std::string("Format chunk too short"));
    }


    // Check our sub format is PCM encoded - currently the only encoding
    // we support.
    //
    unsigned int alwaysOne = getIntegerFromLittleEndian(hS.substr(20, 2));

    if (alwaysOne != 0x01)
        throw(std::string("Rosegarden currently only supports PCM encoded RIFF files"));


    // We seem to have a good looking .WAV file - extract the
    // sample information and populate this locally
    //
    unsigned int channelNumbers =  getIntegerFromLittleEndian(hS.substr(22,2));
    
    switch(channelNumbers)
    {
        case 0x01:
        case 0x02:
            m_channels = channelNumbers;
            break;

        default:
            {
                throw(std::string("Unsupported number of channels"));
            }
            break;
    }

    // Now the rest of the information
    //
    m_sampleRate = getIntegerFromLittleEndian(hS.substr(24,4));
    m_bytesPerSecond = getIntegerFromLittleEndian(hS.substr(28,4));
    m_bytesPerSample = getIntegerFromLittleEndian(hS.substr(32,2));
    m_bitsPerSample = getIntegerFromLittleEndian(hS.substr(34,2));

    //printStats();

}

// Write out the format chunk from our internal data
//
void
RIFFAudioFile::writeFormatChunk()
{
    if (m_outFile == 0 || m_type != WAV)
        return;

    std::string outString;

    // RIFF type is all we support for the moment
    outString += AUDIO_RIFF_ID;

    // Now write the total length of the file minus these first 8 bytes.
    // We won't know this until we've finished recording the file.
    //
    outString += "0000";

    // WAV file is all we support
    //
    outString += AUDIO_WAVE_ID;

    // Begin the format chunk
    outString += AUDIO_FORMAT_ID;

    // length
    //cout << "LENGTH = " << getLittleEndianFromInteger(0x10, 4) << endl;
    outString += getLittleEndianFromInteger(0x10, 4);

    // "always one"
    outString += getLittleEndianFromInteger(0x01, 2);

    // channel
    outString += getLittleEndianFromInteger(m_channels, 2);

    // sample rate
    outString += getLittleEndianFromInteger(m_sampleRate, 4);

    // bytes per second
    outString += getLittleEndianFromInteger(m_bytesPerSecond, 4);

    // bytes per sample
    outString += getLittleEndianFromInteger(m_bytesPerSample, 2);

    // bits per sample
    outString += getLittleEndianFromInteger(m_bitsPerSample, 2);

    // Now mark the beginning of the "data" chunk and leave the file
    // open for writing.
    outString += "data";

    // length of data to follow - again needs to be written after
    // we've completed the file.
    //
    outString += "0000";

    // write out
    //
    putBytes(m_outFile, outString);
}


AudioFileType
RIFFAudioFile::identifySubType(const std::string &filename)
{
    std::ifstream *testFile =
        new std::ifstream(filename.c_str(), std::ios::in | std::ios::binary);

    if (!(*testFile))
        return UNKNOWN;

    std::string hS;
    unsigned int numberOfBytes = 36;
    char *bytes = new char[numberOfBytes];

    testFile->read(bytes, numberOfBytes);
    for (unsigned int i = 0; i < numberOfBytes; i++)
        hS += (unsigned char)bytes[i];

    AudioFileType type = UNKNOWN;

    // Test for BWF first because it's an extension of a plain WAV
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_RIFF_ID, 0, 4) == 0 &&
        hS.compare(Rosegarden::AUDIO_WAVE_ID, 8, 4) == 0 &&
        hS.compare(Rosegarden::AUDIO_BWF_ID, 12, 4) == 0)
#else
    if (hS.compare(0, 4, Rosegarden::AUDIO_RIFF_ID) == 0 &&
        hS.compare(8, 4, Rosegarden::AUDIO_WAVE_ID) == 0 &&
        hS.compare(12, 4, Rosegarden::AUDIO_BWF_ID) == 0)
#endif
    {
        type = BWF;
    }
    // Now for a WAV
#if (__GNUC__ < 3)
    else if (hS.compare(Rosegarden::AUDIO_RIFF_ID, 0, 4) == 0 &&
             hS.compare(Rosegarden::AUDIO_WAVE_ID, 8, 4) == 0)
#else
    else if (hS.compare(0, 4, Rosegarden::AUDIO_RIFF_ID) == 0 &&
             hS.compare(8, 4, Rosegarden::AUDIO_WAVE_ID) == 0)
#endif
    {
        type = WAV;
    }
    else
        type = UNKNOWN;

    testFile->close();
    delete [] bytes;

    return type;
}



}

