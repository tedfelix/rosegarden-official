/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RIFFAudioFile.h"
#include "base/RealTime.h"
#include "base/Profiler.h"
#include "misc/Strings.h"

using std::cout;
using std::cerr;
using std::endl;

//#define DEBUG_RIFF


namespace Rosegarden
{

RIFFAudioFile::RIFFAudioFile(unsigned int id,
                             const std::string &name,
                             const QString &fileName):
    AudioFile(id, name, fileName),
    m_subFormat(PCM),
    m_bytesPerSecond(0),
    m_bytesPerFrame(0)
{}

RIFFAudioFile::RIFFAudioFile(const QString &fileName,
                             unsigned int channels = 1,
                             unsigned int sampleRate = 48000,
                             unsigned int bytesPerSecond = 6000,
                             unsigned int bytesPerFrame = 2,
                             unsigned int bitsPerSample = 16):
        AudioFile(0, "", fileName)
{
    m_bitsPerSample = bitsPerSample;
    m_sampleRate = sampleRate;
    m_bytesPerSecond = bytesPerSecond;
    m_bytesPerFrame = bytesPerFrame;
    m_channels = channels;

    if (bitsPerSample == 16)
        m_subFormat = PCM;
    else if (bitsPerSample == 32)
        m_subFormat = FLOAT;
    else
        throw(BadSoundFileException(m_fileName, qstrtostr(QObject::tr("Rosegarden currently only supports 16 or 32-bit PCM or IEEE floating-point RIFF files for writing"))));

}

RIFFAudioFile::~RIFFAudioFile()
{}


// Show some stats on this file
//
void
RIFFAudioFile::printStats()
{
    cout << "filename         : " << m_fileName << endl
    << "channels         : " << m_channels << endl
    << "sample rate      : " << m_sampleRate << endl
    << "bytes per second : " << m_bytesPerSecond << endl
    << "bits per sample  : " << m_bitsPerSample << endl
    << "bytes per frame  : " << m_bytesPerFrame << endl
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

bool
RIFFAudioFile::appendSamples(const char *buf, unsigned int frames)
{
    putBytes(m_outFile, buf, frames * m_bytesPerFrame);
    return true;
}

// scan on from a descriptor position
bool
RIFFAudioFile::scanForward(std::ifstream *file, const RealTime &time)
{
    // sanity
    if (file == 0)
        return false;

    unsigned int totalSamples = m_sampleRate * time.sec +
                                ( ( m_sampleRate * time.usec() ) / 1000000 );
    unsigned int totalBytes = totalSamples * m_bytesPerFrame;

    m_loseBuffer = true;

    // do the seek
    file->seekg(totalBytes, std::ios::cur);

    if (file->eof())
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
    if (file == 0)
        return false;

    // whatever we do here we invalidate the read buffer
    //
    m_loseBuffer = true;

    file->clear();

    // seek past header - don't hardcode this - use the file format
    // spec to get header length and then scoot to that.
    //
    file->seekg(16, std::ios::beg);

    unsigned int lengthOfFormat = 0;

    try {
        lengthOfFormat = getIntegerFromLittleEndian(getBytes(file, 4));
        file->seekg(lengthOfFormat, std::ios::cur);

        // check we've got data chunk start
	std::string chunkName;
	int chunkLength = 0;

        while ((chunkName = getBytes(file, 4)) != "data") {
	    if (file->eof()) {
		std::cerr << "RIFFAudioFile::scanTo(): failed to find data "
			  << std::endl;
		return false;
	    }
//#ifdef DEBUG_RIFF
	    std::cerr << "RIFFAudioFile::scanTo(): skipping chunk: "
		      << chunkName << std::endl;
//#endif
	    chunkLength = getIntegerFromLittleEndian(getBytes(file, 4));
	    if (chunkLength < 0) {
		std::cerr << "RIFFAudioFile::scanTo(): negative chunk length "
			  << chunkLength << " for chunk " << chunkName << std::endl;
		return false;
	    }
	    file->seekg(chunkLength, std::ios::cur);
        }

        // get the length of the data chunk, and scan past it as a side-effect
	chunkLength = getIntegerFromLittleEndian(getBytes(file, 4));
#ifdef DEBUG_RIFF

        std::cout << "RIFFAudioFile::scanTo() - data chunk size = "
        << chunkLength << std::endl;
#endif

    } catch (BadSoundFileException s) {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::scanTo - EXCEPTION - \""
        << s.getMessage() << "\"" << std::endl;
#endif

        return false;
    }

    // Ok, we're past all the header information in the data chunk.
    // Now, how much do we scan forward?
    //
    size_t totalFrames = size_t(RealTime::realTime2Frame(time, m_sampleRate));

    unsigned int totalBytes = totalFrames * m_bytesPerFrame;

    // When using seekg we have to keep an eye on the boundaries ourselves
    //
    if (totalBytes > m_fileSize - (lengthOfFormat + 16 + 8)) {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::scanTo() - attempting to move past end of "
        << "data block" << std::endl;
#endif

        return false;
    }

#ifdef DEBUG_RIFF
    std::cout << "RIFFAudioFile::scanTo - seeking to " << time
    << " (" << totalBytes << " bytes from current " << file->tellg()
    << ")" << std::endl;
#endif

    file->seekg(totalBytes, std::ios::cur);

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
    if (file == 0)
        return std::string("");

    // Bytes per sample already takes into account the number
    // of channels we're using
    //
    long totalBytes = frames * m_bytesPerFrame;

    try {
        return getBytes(file, totalBytes);
    } catch (BadSoundFileException s) {
        return "";
    }
}

unsigned int
RIFFAudioFile::getSampleFrames(std::ifstream *file, char *buf,
                               unsigned int frames)
{
    if (file == 0)
        return 0;
    try {
        return getBytes(file, buf, frames * m_bytesPerFrame) / m_bytesPerFrame;
    } catch (BadSoundFileException s) {
        return 0;
    }
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
    if (file == 0)
        return std::string("");

    long totalFrames = RealTime::realTime2Frame(time, m_sampleRate);
    long totalBytes = totalFrames * m_bytesPerFrame;

    try {
        return getBytes(file, totalBytes);
    } catch (BadSoundFileException s) {
        return "";
    }
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

    if (m_inFile) {
        m_inFile->seekg(16, std::ios::beg);
        headerLength = getIntegerFromLittleEndian(getBytes(m_inFile, 4));
        m_inFile->seekg(headerLength, std::ios::cur);
        headerLength += (16 + 8);
    }

    if (!m_bytesPerFrame || !m_sampleRate) return RealTime::zeroTime;

    double frames = (m_fileSize - headerLength) / m_bytesPerFrame;
    double seconds = frames / ((double)m_sampleRate);

    int secs = int(seconds);
    int nsecs = int((seconds - secs) * 1000000000.0);

    return RealTime(secs, nsecs);
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
        return ;

    m_loseBuffer = true;

    // seek to beginning
    m_inFile->seekg(0, std::ios::beg);

    // get the header string
    //
    std::string hS = getBytes(36);

    // Look for the RIFF identifier and bomb out if we don't find it
    //
    if (hS.compare(0, 4, AUDIO_RIFF_ID) != 0) {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::readFormatChunk - "
        << "can't find RIFF identifier\n";
#endif

        throw(BadSoundFileException(m_fileName, qstrtostr(QObject::tr("Can't find RIFF identifier"))));
    }

    // Look for the WAV identifier
    //
    if (hS.compare(8, 4, AUDIO_WAVE_ID) != 0) {
#ifdef DEBUG_RIFF
        std::cerr << "Can't find WAV identifier\n";
#endif

        throw(BadSoundFileException(m_fileName, qstrtostr(QObject::tr("Can't find WAV identifier"))));
    }

    // Look for the FORMAT identifier - note that this doesn't actually
    // have to be in the first chunk we come across, but for the moment
    // this is the only place we check for it because I'm lazy.
    //
    //
    if (hS.compare(12, 4, AUDIO_FORMAT_ID) != 0) {
#ifdef DEBUG_RIFF
        std::cerr << "Can't find FORMAT identifier\n";
#endif

        throw(BadSoundFileException(m_fileName, qstrtostr(QObject::tr("Can't find FORMAT identifier"))));
    }

    // Little endian conversion of length bytes into file length
    // (add on eight for RIFF id and length field and compare to
    // real file size).
    //
    unsigned int length = getIntegerFromLittleEndian(hS.substr(4, 4)) + 8;

    if (length != m_fileSize) {
        std::cerr << "WARNING: RIFFAudioFile: incorrect length ("
        << length << ", file size is " << m_fileSize << "), ignoring"
        << std::endl;
        length = m_fileSize;
    }

    // Check the format length
    //
    unsigned int lengthOfFormat = getIntegerFromLittleEndian(hS.substr(16, 4));

    // Make sure we step to the end of the format chunk ignoring the
    // tail if it exists
    //
    if (lengthOfFormat > 0x10) {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::readFormatChunk - "
        << "extended Format Chunk (" << lengthOfFormat << ")"
        << std::endl;
#endif

        // ignore any overlapping bytes
        m_inFile->seekg(lengthOfFormat - 0x10, std::ios::cur);
    } else if (lengthOfFormat < 0x10) {
#ifdef DEBUG_RIFF
        std::cerr << "RIFFAudioFile::readFormatChunk - "
        << "truncated Format Chunk (" << lengthOfFormat << ")"
        << std::endl;
#endif

        m_inFile->seekg(lengthOfFormat - 0x10, std::ios::cur);
        //throw(BadSoundFileException(m_fileName, "Format chunk too short"));
    }


    // Check sub format - we support PCM or IEEE floating point.
    //
    unsigned int subFormat = getIntegerFromLittleEndian(hS.substr(20, 2));

    if (subFormat == 0x01) {
        m_subFormat = PCM;
    } else if (subFormat == 0x03) {
        m_subFormat = FLOAT;
    } else {
        throw(BadSoundFileException(m_fileName, qstrtostr(QObject::tr("Rosegarden currently only supports PCM or IEEE floating-point RIFF files"))));
    }

    // We seem to have a good looking .WAV file - extract the
    // sample information and populate this locally
    //
    unsigned int channelNumbers = getIntegerFromLittleEndian(hS.substr(22, 2));

    switch (channelNumbers) {
    case 0x01:
    case 0x02:
        m_channels = channelNumbers;
        break;

    default: {
            throw(BadSoundFileException(m_fileName, qstrtostr(QObject::tr("Unsupported number of channels"))));
        }
        break;
    }

    // Now the rest of the information
    //
    m_sampleRate = getIntegerFromLittleEndian(hS.substr(24, 4));
    m_bytesPerSecond = getIntegerFromLittleEndian(hS.substr(28, 4));
    m_bytesPerFrame = getIntegerFromLittleEndian(hS.substr(32, 2));
    m_bitsPerSample = getIntegerFromLittleEndian(hS.substr(34, 2));

    if (m_subFormat == PCM) {
        if (m_bitsPerSample != 8 && m_bitsPerSample != 16 && m_bitsPerSample != 24) {
            throw BadSoundFileException(QObject::tr("Rosegarden currently only supports 8-, 16- or 24-bit PCM in RIFF files"));
        }
    } else if (m_subFormat == FLOAT) {
        if (m_bitsPerSample != 32) {
            throw BadSoundFileException(QObject::tr("Rosegarden currently only supports 32-bit floating-point in RIFF files"));
        }
    }

   // printStats();

}

// Write out the format chunk from our internal data
//
void
RIFFAudioFile::writeFormatChunk()
{
    if (m_outFile == 0 || m_type != WAV)
        return ;

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

    // 1 for PCM, 3 for float
    if (m_subFormat == PCM) {
        outString += getLittleEndianFromInteger(0x01, 2);
    } else {
        outString += getLittleEndianFromInteger(0x03, 2);
    }

    // channel
    outString += getLittleEndianFromInteger(m_channels, 2);

    // sample rate
    outString += getLittleEndianFromInteger(m_sampleRate, 4);

    // bytes per second
    outString += getLittleEndianFromInteger(m_bytesPerSecond, 4);

    // bytes per sample
    outString += getLittleEndianFromInteger(m_bytesPerFrame, 2);

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
RIFFAudioFile::identifySubType(const QString &filename)
{
    std::ifstream *testFile =
        new std::ifstream(filename.toLocal8Bit(), std::ios::in | std::ios::binary);

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
    if (hS.compare(0, 4, AUDIO_RIFF_ID) == 0 &&
            hS.compare(8, 4, AUDIO_WAVE_ID) == 0 &&
            hS.compare(12, 4, AUDIO_BWF_ID) == 0) {
        type = BWF;
    }
    // Now for a WAV
    else if (hS.compare(0, 4, AUDIO_RIFF_ID) == 0 &&
             hS.compare(8, 4, AUDIO_WAVE_ID) == 0) {
        type = WAV;
    } else
        type = UNKNOWN;

    testFile->close();
    delete testFile;
    delete [] bytes;

    return type;
}

float
RIFFAudioFile::convertBytesToSample(const unsigned char *ubuf)
{
    switch (getBitsPerSample()) {

    case 8: {
            // WAV stores 8-bit samples unsigned, other sizes signed.
            return (float)(ubuf[0] - 128.0) / 128.0;
        }

    case 16: {
            // Two's complement little-endian 16-bit integer.
            // We convert endianness (if necessary) but assume 16-bit short.
            unsigned char b2 = ubuf[0];
            unsigned char b1 = ubuf[1];
            unsigned int bits = (b1 << 8) + b2;
            return (float)(short(bits)) / 32768.0;
        }

    case 24: {
            // Two's complement little-endian 24-bit integer.
            // Again, convert endianness but assume 32-bit int.
            unsigned char b3 = ubuf[0];
            unsigned char b2 = ubuf[1];
            unsigned char b1 = ubuf[2];
            // Rotate 8 bits too far in order to get the sign bit
            // in the right place; this gives us a 32-bit value,
            // hence the larger float divisor
            unsigned int bits = (b1 << 24) + (b2 << 16) + (b3 << 8);
            return (float)(int(bits)) / 2147483648.0;
        }

    case 32: {
            // IEEE floating point
            return *(float *)ubuf;
        }

    default:
        return 0.0f;
    }
}

}

