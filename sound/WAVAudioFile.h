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


// Specialisation of a RIFF file - the WAV defines a format chunk
// holding audio file meta data and a data chunk with interleaved
// sample bytes.
//

#include "RIFFAudioFile.h"


#ifndef _WAVAUDIOFILE_H_
#define _WAVAUDIOFILE_H_

namespace Rosegarden
{

class RIFFPeakManager;

class WAVAudioFile : public RIFFAudioFile
{
public:
    WAVAudioFile(const unsigned int &id,
                 const std::string &name,
                 const std::string &fileName);

    WAVAudioFile(const std::string &fileName,
                  unsigned int channels,
                  unsigned int sampleRate,
                  unsigned int bytesPerSecond,
                  unsigned int bytesPerSample,
                  unsigned int bitsPerSample);

    ~WAVAudioFile();

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
    virtual streampos getDataOffset();

    // Peak file name
    //
    virtual std::string getPeakFilename()
        { return (m_fileName + std::string(".pk")); }


protected:

    RIFFPeakManager *m_peakManager;

};

}


#endif // _WAVAUDIOFILE_H_
