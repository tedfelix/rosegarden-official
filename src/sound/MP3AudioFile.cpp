// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "MP3AudioFile.h"

#ifdef HAVE_LIBMAD

#include <mad.h>

namespace Rosegarden
{


/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

struct player
{
    unsigned char const *start;
    unsigned long length;
    //int default_driver;
    //ao_device *device;
    //ao_sample_format format;
    //class SoundTouch *touch;
};



MP3AudioFile::MP3AudioFile(const unsigned int &id,
                           const std::string &name,
                           const std::string &fileName):
        AudioFile(id, name, fileName)
{
    m_type = MP3;
}


MP3AudioFile::MP3AudioFile(const std::string &fileName,
                           unsigned int /*channels*/,
                           unsigned int /*sampleRate*/,
                           unsigned int /*bytesPerSecond*/,
                           unsigned int /*bytesPerSample*/,
                           unsigned int /*bitsPerSample*/):
        AudioFile(0, std::string(""), fileName)
{
    m_type = MP3;
}


MP3AudioFile::~MP3AudioFile()
{}

bool
MP3AudioFile::open()
{
    // if already open
    if (m_inFile && (*m_inFile))
        return true;

    m_inFile = new std::ifstream(m_fileName.c_str(),
                                 std::ios::in | std::ios::binary);

    if (!(*m_inFile)) {
        m_type = UNKNOWN;
        return false;
    }

    // Get the file size and store it for comparison later
    m_fileSize = m_fileInfo->size();

    try {
        parseHeader();
    } catch (BadSoundFileException s) {
        throw(s);
    }

    return true;
}

bool
MP3AudioFile::write()
{
    return false;
}

void
MP3AudioFile::close()
{}

void
MP3AudioFile::parseHeader()
{
    const std::string MP3_TAG("TAG");
    if (m_inFile == 0)
        return ;

    // store size conveniently
    m_fileSize = m_fileInfo->size();

    if (m_fileSize == 0) {
        std::string mess = std::string("\"") + m_fileName +
                           std::string("\" is empty - invalid MP3 file");
        throw(mess);
    }

    // seek to beginning
    m_inFile->seekg(0, std::ios::beg);

    // get some header information
    //
    const int bufferLength = 3096;
    std::string hS = getBytes(bufferLength);
    bool foundMP3 = false;

    for (unsigned int i = 0; i < hS.length() - 1; ++i) {
        if ((hS[i] & 0xff) == 0xff && (hS[i + 1] & 0xe0) == 0xe0) {
            foundMP3 = true;
            break;
        }
    }

    if (foundMP3 == false || (int)hS.length() < bufferLength) {
        std::string mess = std::string("\"") + m_fileName +
                           std::string("\" doesn't appear to be a valid MP3 file");
        throw(mess);
    }

    // guess most likely values - these are reset during decoding
    m_channels = 2;
    m_sampleRate = 44100;

    mad_synth synth;
    mad_frame frame;
    mad_stream stream;

    mad_synth_init(&synth);
    mad_stream_init(&stream);
    mad_frame_init(&frame);

    /*
    mad_stream_buffer(&stream, hS.data(), hS.length());

    if (mad_header_decode(&frame.header, &stream) == -1)
    {
        throw("Can't decode header");
    }

    mad_frame_decode(&frame, &stream);

    m_sampleRate = frame.header.samplerate;

    mad_synth_frame(&synth, &frame);
    struct mad_pcm *pcm = &synth.pcm;

    m_channels = pcm->channels;
    */

    /*
        struct player player;
        struct mad_decoder decoder;
        struct stat stat;
        void *fdm;
        int result;
     
        if (fstat(fd, &stat) == -1 ||
            stat.st_size == 0)
          return 0;
     
        fdm = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (fdm == MAP_FAILED) {
          fprintf(stderr, "mmap failed, aborting...\n");
          return 0;
        }
     
        player.start = (unsigned char *)fdm;
        player.length = stat.st_size;
        player.default_driver = ao_default_driver_id();
        player.device = NULL;
        player.touch = new SoundTouch;
        player.touch->setTempo(tempo);
        player.touch->setPitch(pitch);
        mad_decoder_init(&decoder, &player,
                         input, 0 , 0 , process_output,
                         decode_error, 0);
     
        result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
        mad_decoder_finish(&decoder);
        delete player.touch;
        ao_close(player.device);
        if (munmap((void *)player.start, stat.st_size) == -1)
            return 4;
     
        return result;
    */
}

std::streampos
MP3AudioFile::getDataOffset()
{
    return 0;
}

bool
MP3AudioFile::scanTo(const RealTime & /*time*/)
{
    return false;
}

bool
MP3AudioFile::scanTo(std::ifstream * /*file*/, const RealTime & /*time*/)
{
    return false;
}


// Scan forward in a file by a certain amount of time
//
bool
MP3AudioFile::scanForward(const RealTime & /*time*/)
{
    return false;
}

bool
MP3AudioFile::scanForward(std::ifstream * /*file*/, const RealTime & /*time*/)
{
    return false;
}


// Return a number of samples - caller will have to
// de-interleave n-channel samples themselves.
//
std::string
MP3AudioFile::getSampleFrames(std::ifstream * /*file*/,
                              unsigned int /*frames*/)
{
    return "";
}

unsigned int
MP3AudioFile::getSampleFrames(std::ifstream * /*file*/,
                              char * /* buf */,
                              unsigned int /*frames*/)
{
    return 0;
}

std::string
MP3AudioFile::getSampleFrames(unsigned int /*frames*/)
{
    return "";
}


// Return a number of (possibly) interleaved samples
// over a time slice from current file pointer position.
//
std::string
MP3AudioFile::getSampleFrameSlice(std::ifstream * /*file*/,
                                  const RealTime & /*time*/)
{
    return "";
}

std::string
MP3AudioFile::getSampleFrameSlice(const RealTime & /*time*/)
{
    return "";
}


// Append a string of samples to an already open (for writing)
// audio file.
//
bool
MP3AudioFile::appendSamples(const std::string & /*buffer*/)
{
    return false;
}

bool
MP3AudioFile::appendSamples(const char * /*buffer*/, unsigned int)
{
    return false;
}


// Get the length of the sample in Seconds/Microseconds
//
RealTime
MP3AudioFile::getLength()
{
    return RealTime(0, 0);
}

void
MP3AudioFile::printStats()
{}





}

#endif // HAVE_LIBMAD
