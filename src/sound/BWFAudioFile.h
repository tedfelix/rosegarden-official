/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


// Specialisation of a RIFF file - the WAV defines a format chunk
// holding audio file meta data and a data chunk with interleaved
// sample bytes.
//

#include "RIFFAudioFile.h"


#ifndef RG_BWFAUDIOFILE_H
#define RG_BWFAUDIOFILE_H

namespace Rosegarden
{

class BWFAudioFile : public RIFFAudioFile
{
public:
    BWFAudioFile(const unsigned int &id,
                 const std::string &name,
                 const QString &fileName);

    BWFAudioFile(const QString &fileName,
                  unsigned int channels,
                  unsigned int sampleRate,
                  unsigned int bytesPerSecond,
                  unsigned int bytesPerFrame,
                  unsigned int bitsPerSample);

    ~BWFAudioFile();

    // Override these methods for the WAV
    //
    virtual bool open();
    virtual bool write();
    virtual void close();

    // Get all header information
    //
    void parseHeader();

    //
    // 
    //virtual std::vector<float> getPreview(const RealTime &resolution);

    // Offset to start of sample data
    //
    virtual std::streampos getDataOffset();

    // Peak file name
    //
    virtual QString getPeakFilename()
        { return (m_fileName + ".pk"); }


    //!!! NOT IMPLEMENTED YET
    // 
    virtual bool decode(const unsigned char */* sourceData */,
                        size_t /* sourceBytes */,
                        size_t /* targetSampleRate */,
                        size_t /* targetChannels */,
                        size_t /* targetFrames */,
                        std::vector<float *> &/* targetData */,
                        bool /* addToResultBuffers = false */) { return false; }

protected:

};

}


#endif // RG_BWFAUDIOFILE_H
