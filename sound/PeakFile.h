// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*- /*
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

#include <vector>

#include <qdatetime.h>

#include "SoundFile.h"
#include "RealTime.h"

#ifndef _PEAKFILE_H_
#define _PEAKFILE_H_

// A PeakFile is generated to the BWF Supplement 3 Peak Envelope Chunk
// format as defined here:
//
// http://www.ebu.ch/pmc_bwf.html
//
// To comply with BWF format files this chunk can be embedded into
// the sample file itself (writeToHandle()) or used to generate an
// external peak file (write()).  At the moment the only type of file
// with an embedded peak chunk is the BWF file itself.
//
//



namespace Rosegarden
{

class AudioFile;
class Progress;


typedef std::pair<RealTime, RealTime> SplitPointPair;

class PeakFile : public SoundFile
{
public:
    PeakFile(AudioFile *audioFile);
    virtual ~PeakFile();

    // Standard methods
    //
    virtual bool open();
    virtual void close();

    // Write to standard peak file
    //
    virtual bool write();

    // Write the file with a progress dialog
    //
    virtual bool write(Progress *progress, unsigned short updatePercentage);

    // Write peak chunk to file handle (BWF)
    //
    bool writeToHandle(std::ofstream *file, unsigned short updatePercentage);

    // Is the peak file valid and up to date?
    //
    bool isValid();

    // Vital file stats
    //
    void printStats();

    // Get a preview of a section of the audio file where that section
    // is "width" pixels.
    //
    std::vector<float> getPreview(const RealTime &startTime,
                                  const RealTime &endTime,
                                  int width,
                                  bool showMinima);

    AudioFile* getAudioFile() { return m_audioFile; }

    // Scan to a peak and scan forward a number of peaks
    //
    bool scanToPeak(int peak);
    bool scanForward(int numberOfPeaks);

    // Find threshold crossing points
    //
    std::vector<SplitPointPair> getSplitPoints(const RealTime &startIndex,
                                               const RealTime &endIndex,
                                               int threshold);

protected:
    // Write the peak header and the peaks themselves
    //
    void writeHeader(std::ofstream *file);
    void writePeaks(Progress *progress,
                    unsigned short updatePercentage,
                    std::ofstream *file);

    // Get the position of a peak for a given time
    //
    double getPeak(const RealTime &time);

    // Parse the header
    //
    void parseHeader();

    AudioFile *m_audioFile;

    // Some Peak Envelope Chunk parameters
    //
    int m_version;
    int m_format;  // bytes in peak value (1 or 2)
    int m_pointsPerValue;
    int m_blockSize;
    int m_channels;
    int m_numberOfPeaks;
    int m_positionPeakOfPeaks;
    int m_offsetToPeaks;
    int m_bodyBytes;

    // Peak timestamp
    //
    QDateTime m_modificationTime;

    std::streampos m_chunkStartPosition;
    
};

};


#endif // _PEAKFILE_H_


