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


namespace Rosegarden
{

class AudioFile : public SoundFile
{
public:

    typedef enum
    {
        AUDIO_NOT_LOADED,
        AUDIO_NOT_RECOGNISED,
        AUDIO_WAV
    } AudioFileType;

    AudioFile(const int &id, const string &name, const string &fileName);
    ~AudioFile();

    std::string getName() const { return m_name; }
    int getID() const { return m_id; }
    int getBits() const { return m_bits; }
    int getResolution() const { return m_resolution; }
    bool getStereo() const { return m_stereo; }
    
    AudioFileType getType() { return m_type; }

    // We must define our two abstract methods in this class
    //
    virtual bool open();
    virtual bool write();

private:

    void parseHeader(const std::string &headerString);
    void parseBody();

    int m_id;
    string m_name;
    int m_bits;
    int m_resolution;
    bool m_stereo;
    AudioFileType m_type;

    int m_fileSize;

};

}


#endif // _AUDIOFILE_H_
