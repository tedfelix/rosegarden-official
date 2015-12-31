/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <vector>

#include <QObject>
#include <QDateTime>

#include "SoundFile.h"
#include "base/RealTime.h"

#ifndef RG_PEAKFILE_H
#define RG_PEAKFILE_H

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


typedef std::pair<RealTime, RealTime> SplitPointPair;

class PeakFile : public QObject, public SoundFile
{
    Q_OBJECT

public:
    PeakFile(AudioFile *audioFile);
    virtual ~PeakFile();

    // Copy constructor
    //
    PeakFile(const PeakFile &);

    // Standard file methods
    //
    virtual bool open();
    virtual void close();

    // Write to standard peak file
    //
    virtual bool write();

    // Write the file, emit value() signal and process app events
    //
    virtual bool write(unsigned short updatePercentage);

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
    const AudioFile* getAudioFile() const { return m_audioFile; }

    // Scan to a peak and scan forward a number of peaks
    //
    bool scanToPeak(int peak);
    bool scanForward(int numberOfPeaks);

    // Find threshold crossing points
    //
    std::vector<SplitPointPair> getSplitPoints(const RealTime &startTime,
                                               const RealTime &endTime,
                                               int threshold,
                                               const RealTime &minLength);
    // Accessors
    //
    int getVersion() const { return m_version; }
    int getFormat() const { return m_format; }
    int getPointsPerValue() const { return m_pointsPerValue; }
    int getBlockSize() const { return m_blockSize; }
    int getChannels() const { return m_channels; }
    int getNumberOfPeaks() const { return m_numberOfPeaks; }
    int getPositionPeakOfPeaks() const { return m_positionPeakOfPeaks; }
    int getOffsetToPeaks() const { return m_offsetToPeaks; }
    int getBodyBytes() const { return m_bodyBytes; }
    QDateTime getModificationTime() const { return m_modificationTime; }
    std::streampos getChunkStartPosition() const
        { return m_chunkStartPosition; }

    bool isProcessingPeaks() const { return m_keepProcessing; }
    void setProcessingPeaks(bool value) { m_keepProcessing = value; }

signals:
    void setValue(int);
    
protected:
    // Write the peak header and the peaks themselves
    //
    void writeHeader(std::ofstream *file);
    void writePeaks(unsigned short updatePercentage,
                    std::ofstream *file);

    // Get the position of a peak for a given time
    //
    int getPeak(const RealTime &time);

    // And the time of a peak
    //
    RealTime getTime(int peak);

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

    // For cacheing of peak information in memory we use the last query 
    // parameters as our key to the cached data.
    //
    RealTime           m_lastPreviewStartTime;
    RealTime           m_lastPreviewEndTime;
    int                m_lastPreviewWidth;
    bool               m_lastPreviewShowMinima;
    std::vector<float> m_lastPreviewCache;

    // Do we actually want to keep processing this peakfile?
    // In case we get a cancel.
    //
    bool               m_keepProcessing;

    std::string        m_peakCache;
    
};

}


#endif // RG_PEAKFILE_H


