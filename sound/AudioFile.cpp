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
    SoundFile(fileName),
    m_id(id),
    m_name(name),
    m_bitsPerSample(0),
    m_sampleRate(0),
    m_bytesPerSecond(0),
    m_bytesPerSample(0),
    m_channels(0),
    m_type(UNKNOWN),
    m_fileSize(0),
    m_inFile(0),
    m_outFile(0)
{
}

AudioFile::AudioFile(const std::string &fileName,
                     AudioFileType type,
                     unsigned int channels,
                     unsigned int sampleRate,
                     unsigned int bytesPerSecond,
                     unsigned int bytesPerSample,
                     unsigned int bitsPerSample ):
    SoundFile(fileName), m_id(0), m_name(""),
    m_bitsPerSample(bitsPerSample),
    m_sampleRate(sampleRate),
    m_bytesPerSecond(bytesPerSecond),
    m_bytesPerSample(bytesPerSample),
    m_channels(channels),
    m_type(type),
    m_fileSize(0),
    m_inFile(0),
    m_outFile(0)
{
}

AudioFile::~AudioFile()
{
    if (m_inFile)
    {
        m_inFile->close();
        delete m_inFile;
    }

    if (m_outFile)
    {
        m_outFile->close();
        delete m_outFile;
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
    unsigned int length = getIntegerFromLittleEndian(hS.substr(4,4)) + 8;

    if (length != m_fileSize)
        throw(std::string("AudioFile::parseHeader - file " + m_fileName +
                     " corrupted (wrong length)"));

    // Check the format length (always 0x10)
    //
    unsigned int lengthOfFormat = getIntegerFromLittleEndian(hS.substr(16, 4));

    if (lengthOfFormat != 0x10)
        throw(std::string("AudioFile::parseHeader - format length incorrect"));


    // Check this field is one
    //
    unsigned int alwaysOne = getIntegerFromLittleEndian(hS.substr(20, 2));
    if (alwaysOne != 0x01)
        throw(std::string("AudioFile::parseHeader - \"always one\" byte isn't"));


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
                throw(std::string("AudioFile::parseHeader - unsupport number of channels"));
            }
            break;
    }

    // Now the rest of the information
    //
    m_sampleRate = getIntegerFromLittleEndian(hS.substr(24,4));
    m_bytesPerSecond = getIntegerFromLittleEndian(hS.substr(28,4));
    m_bytesPerSample = getIntegerFromLittleEndian(hS.substr(32,2));
    m_bitsPerSample = getIntegerFromLittleEndian(hS.substr(34,2));
   
}

bool
AudioFile::open()
{
    m_inFile = new std::ifstream(m_fileName.c_str(),
                                 std::ios::in | std::ios::binary);

    // get the actual file size
    //
    m_inFile->seekg(0, std::ios::end);
    m_fileSize = m_inFile->tellg();
    m_inFile->seekg(0, std::ios::beg);

    // now parse the file
    try
    {
        if (*m_inFile)
        {
            try
            {
                parseHeader(getBytes(m_inFile, 36));
            }
            catch(std::string s)
            {
                cerr << "EXCEPTION : " << s << endl;
            }
        }
        else
        {
            m_type = UNKNOWN;
            return (false);
        }
    }
    catch(std::string s)
    {  
        cout << "EXCEPTION : " << s << endl;
        return false;
    }

    m_type = WAV;

    // Reset to front of "data" block
    //
    scanTo(m_inFile, RealTime(0, 0));

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

// For an AudioFile we write a header (if we can) and leave
// the file descriptor open for subsequent appends.
//
bool
AudioFile::write()
{
    // for the moment we only support WAVs
    //
    if (m_type != WAV)
        return false;

    // close if we're open
    if (m_outFile)
    {
        m_outFile->close();
        delete m_outFile;
    }

    // open for writing
    m_outFile = new std::ofstream(m_fileName.c_str(),
                                  std::ios::out | std::ios::binary);


    return true;
}

bool
AudioFile::appendSamples(const std::string &buffer)
{
    /*
    if (m_outFile == 0 || m_type != WAV)
        return false;
        */

    // write out
    putBytes(m_outFile, buffer);

    return true;
}

void
AudioFile::writeHeader()
{
    if (m_outFile == 0 || m_type != WAV)
        return;

    std::string outString;

    // RIFF type is all we support for the moment
    outString += "RIFF";

    // Now write the total length of the file minus these first 8 bytes.
    // We won't know this until we've finished recording the file.
    //
    outString += "0000";

    // WAV file is all we support
    //
    outString += "WAVE";

    // Begin the format chunk
    outString += "fmt ";

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

    // get the length of the data chunk
    std::cout << "AudioFile::scanTo() - data chunk size = "
              << getIntegerFromLittleEndian(getBytes(file, 4)) << std::endl;


    // Ok, we're past all the header information in the data chunk.
    // Now, how much do we scan forward?
    //
    unsigned int totalSamples = m_sampleRate * time.sec +
                        ( ( m_sampleRate * time.usec ) / 1000000 );

    unsigned int totalBytes = totalSamples * m_channels * m_bytesPerSample;


    // When using seekg we have to keep an eye on the boundaries ourselves
    //
    if (totalBytes > m_fileSize - 44)
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

// Close the file and calculate the sizes
//
void
AudioFile::close()
{
    if (m_outFile == 0)
        return;

    m_outFile->seekp(0, std::ios::end);
    unsigned int totalSize = m_outFile->tellp();

    // seek to first length position
    m_outFile->seekp(4, std::ios::beg);

    // write complete file size minus 8 bytes to here
    putBytes(m_outFile, getLittleEndianFromInteger(totalSize - 8, 4));

    // reseek from start forward 40
    m_outFile->seekp(40, std::ios::beg);

    // write the data chunk size to end
    putBytes(m_outFile, getLittleEndianFromInteger(totalSize - 44, 4));

    m_outFile->close();
    delete m_outFile;

}

}

