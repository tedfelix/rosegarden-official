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


#ifndef _AUDIOFILE_H_
#define _AUDIOFILE_H_

#include <string>
#include "SoundFile.h"
#include "RealTime.h"

// An AudioFile extracts and maintains information pertaining
// to an audio (sample) file.  For the moment the only format
// we handle is the .wav
//
// Our "open" call in this case only examines the header and
// populates useful information/checks for errors.  We don't
// yet attempt to scan the body of the file as it could be
// potentially very big.
//
// When it comes to writing this file out we may need to get
// chunks of data from the file itself depending on what driver
// we're using.  For JACK/ALSA we do have to fill the output
// buffer ourselves in this manner.
//

namespace Rosegarden
{

typedef enum
{
    AUDIO_NOT_LOADED,
    AUDIO_NOT_RECOGNISED,
    AUDIO_WAV
} AudioFileType;

class AudioFile : public SoundFile
{
public:


    AudioFile(const unsigned int &id,
              const std::string &name,
              const std::string &fileName);
    ~AudioFile();

    std::string getName() const { return m_name; }
    unsigned int getID() const { return m_id; }
    unsigned int getBitsPerSample() const { return m_bitsPerSample; }
    unsigned int getSampleRate() const { return m_sampleRate; }
    bool getChannels() const { return m_channels; }
    
    AudioFileType getType() { return m_type; }

    // We must define our two abstract methods in this class
    //
    virtual bool open();
    virtual bool write();

    // Show the information we have on this file
    //
    void printStats();

    // Move file pointer to relative time in data chunk -
    // shouldn't be less than zero.  Returns true if the
    // scan time was valid and successful.
    // 
    bool scanTo(const RealTime &time);

    // Return a number of samples - caller will have to
    // de-interleave n-channel samples themselves.
    //
    std::string getSamples(unsigned int samples);

    // Return a number of (possibly) interleaved samples
    // over a time slice from current position.
    //
    std::string getSampleSlice(const RealTime &time);

private:

    void parseHeader(const std::string &header);
    void parseBody();

    unsigned int   m_id;
    std::string    m_name;
    unsigned int   m_bitsPerSample;
    unsigned int   m_sampleRate;
    unsigned int   m_bytesPerSecond;
    unsigned int   m_bytesPerSample;
    unsigned int   m_channels;
    AudioFileType  m_type;
    unsigned int   m_fileSize;

    std::ifstream *m_file;
};

}


#endif // _AUDIOFILE_H_
