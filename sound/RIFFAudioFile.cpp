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


/*
bool
RIFFAudioFile::open()
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
*/

// Show some stats on this file
//
void
RIFFAudioFile::printStats()
{
    cout << "filename        : " << m_fileName  << endl
         << "number of bits  : " << m_bitsPerSample << endl
         << "sample rate     : " << m_sampleRate << endl
         << "file length     : " << m_fileSize << " bytes" << endl
         << "channels        : " << m_channels << endl
         << endl;
}

// For an RIFFAudioFile we write a header (if we can) and leave
// the file descriptor open for subsequent appends.
//
/*
bool
RIFFAudioFile::write()
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
*/

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

/*
void
RIFFAudioFile::writeHeader()
{
}
*/

// scan on from a descriptor position
bool
RIFFAudioFile::scanForward(std::ifstream *file, const RealTime &time)
{
    // sanity
    if (file == 0) return false;

    unsigned int totalSamples = m_sampleRate * time.sec +
                        ( ( m_sampleRate * time.usec ) / 1000000 );
    unsigned int totalBytes = totalSamples * m_bytesPerSample;

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

    // seek past header - don't hardcode this - use the file format
    // spec to get header length
    //
    file->seekg(36, std::ios::beg);

    try
    {
        // check we've got data chunk start
        if (getBytes(file, 4) != "data")
        {
            std::cerr << "RIFFAudioFile::scanTo() - can't find data chunk where "
                      << "it was expected" << std::endl;
            return false;
        }

        // get the length of the data chunk
        std::cout << "RIFFAudioFile::scanTo() - data chunk size = "
                  << getIntegerFromLittleEndian(getBytes(file, 4)) << std::endl;

    }
    catch(std::string s)
    {
        std::cerr << "RIFFAudioFile::scanTo - EXCEPTION - \""
                  << s << "\"" << std::endl;
        return false;
    }
        
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
        std::cerr << "RIFFAudioFile::scanTo() - attempting to move past end of "
                  << "data block" << std::endl;
        return false;
    }

    file->seekg(totalBytes,  std::ios::cur);

    std::cout << "RIFFAudioFile::scanTo - seeking to " << time
              << " (" << totalBytes << " bytes)" << std::endl;

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

// Close the file and calculate the sizes
//
/*
void
RIFFAudioFile::close()
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
    m_outFile = 0;

}
*/

// Get a normalised (-1.0 to +1.0 float) preview of the audio file
// at a certain resolution - don't use a small resolution on a large
// file or you could be waiting for a while - until this has been
// optimised.  Don't use to high a resolution or your data will
// be meaningless of course.
//
/*
std::vector<float>
RIFFAudioFile::getPreview(const RealTime &resolution)
{
    std::vector<float> preview;

    if (m_inFile == 0)
        return preview;
    //std::ifstream *previewFile = new std::ifstream(m_fileName.c_str(),
                                                   //std::ios::in |
                                                   //std::ios::binary);
    try
    {

    // move past header to beginning of the data
    scanTo(m_inFile, RealTime(0, 0));

    unsigned int totalSample, totalBytes;
    std::string samples;
    char *samplePtr;
    float meanValue;

    // Read sample data at given resolution and push the results
    // onto the result vector.
  
    // We need sinc interpolation:
    //
    //   sinc(x)
    //      returns sin(pi*x)/(pi*x) at all points of array x.

    do
    {
        meanValue = 0.0f; // reset

        samples = getBytes(m_bytesPerSample); // buffered read
        samplePtr = (char *)samples.c_str();

        for (unsigned int i = 0; i < m_channels; i++)
        {

            // get the whole frame
            switch(m_bitsPerSample)
            {
                case 8: // 8 bit
                    meanValue += (*((unsigned char *)samplePtr))
                                 / SAMPLE_MAX_8BIT;
                    samplePtr++;
                    break;

                case 16: // 16 bit
                    meanValue += (*((short*)samplePtr)) / SAMPLE_MAX_16BIT;
                    samplePtr += 2;
                    break;

                case 24: // 24 bit
                default:
                    std::cerr << "RIFFAudioFile::getPreview - "
                              << "unsupported bit depth of "
                              << m_bitsPerSample
                              << std::endl;
                    break;
            }
        }

        meanValue /= ((float)m_channels);

        // store - only one value per whole sample frame
        //preview.push_back(sinc(meanValue));
        preview.push_back(meanValue);
    }
    while(scanForward(m_inFile, resolution));

    // clear up
    //previewFile->close();
    //delete previewFile;
    
    }
    catch(std::string s)
    {
        std::cerr << "RIFFAudioFile::getPreview - EXCEPTION - \"" 
                  << s << "\"" << std::endl;
    }

    return preview;
}
*/

RealTime
RIFFAudioFile::getLength()
{
    // The total length of data chunk = m_fileSize - 44 (fixed header size)
    // and so it's easy to work out the length of the file:

    // bytesPerSample allows for number of channels
    //
    double frames = ( m_fileSize - 44 ) / m_bytesPerSample;
    double seconds = frames / ((double)m_sampleRate);

    int secs = seconds;
    int usecs = ( seconds - secs ) * 1000000;

    return RealTime(secs, usecs);
}

/*
// Nothing going on in here for the moment
//
void
RIFFAudioFile::parseBody()
{
}
*/


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
        throw((std::string("RIFFAudioFile::parseHeader - can't find RIFF identifier")));
    }

    // Look for the WAV identifier
    //
#if (__GNUC__ < 3)
    if (hS.compare(Rosegarden::AUDIO_WAVE_ID, 8, 4) != 0)
#else
    if (hS.compare(4, 4, Rosegarden::AUDIO_WAVE_ID) != 0)
#endif
    {
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
    if (hS.compare(4, 4, Rosegarden::AUDIO_FORMAT_ID) != 0)
#endif
    {
        throw((std::string("Can't find FORMAT identifier")));
    }

    // Little endian conversion of length bytes into file length
    // (add on eight for RIFF id and length field and compare to 
    // real file size).
    //
    unsigned int length = getIntegerFromLittleEndian(hS.substr(4,4)) + 8;

    if (length != m_fileSize)
    {
        throw(std::string("\"" + m_fileName +
                     "\" corrupted (wrong length)"));
    }

    // Check the format length
    //
    unsigned int lengthOfFormat = getIntegerFromLittleEndian(hS.substr(16, 4));

    // Make sure we step to the end of the format chunk ignoring the
    // tail if it exists
    //
    if (lengthOfFormat > 0x10)
    {
        std::cerr << "RIFFAudioFile::readFormatChunk - "
                  << "extended Format Chunk (" << lengthOfFormat << ")"
                  << std::endl;

        // ignore any overlapping bytes 
        m_inFile->seekg(lengthOfFormat - 16);
    }
    else
    {
        throw("Format chunk too short");
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





}

