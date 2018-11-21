/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <vector>

#include <QObject>
#include <QDateTime>
#include <QPointer>

class QProgressDialog;

#include "SoundFile.h"
#include "base/RealTime.h"

#ifndef RG_PEAKFILE_H
#define RG_PEAKFILE_H


namespace Rosegarden
{

class AudioFile;


typedef std::pair<RealTime, RealTime> SplitPointPair;

/**
 * A PeakFile is generated to the Broadcast Wave Format (BWF) Peak Envelope
 * Chunk format as defined in EBU Tech 3285 Supplement 3:
 *
 *   https://tech.ebu.ch/docs/tech/tech3285s3.pdf
 *
 * To comply with BWF format files this chunk can be embedded into
 * the sample file itself (writeToHandle()) or used to generate an
 * external peak file (write()).  At the moment the only type of file
 * with an embedded peak chunk is the BWF file itself.
 */
class PeakFile : public QObject, public SoundFile
{
    Q_OBJECT

public:
    PeakFile(AudioFile *audioFile);
    ~PeakFile() override;

    bool open() override;
    void close() override;

    /// Set up a progress dialog for write().
    void setProgressDialog(QPointer<QProgressDialog> progressDialog)
            { m_progressDialog = progressDialog; }

    /// Write to standard peak file
    bool write() override;

    /// Is the peak file valid and up to date?
    /**
     * If the audio file is more recently modified than the modification time
     * on this peak file then we're invalid.  The action to rectify this is
     * usually to regenerate the peak data.
     */
    bool isValid();

    /// Get a preview of a section of the audio file.
    std::vector<float> getPreview(const RealTime &startTime,
                                  const RealTime &endTime,
                                  int width,
                                  bool showMinima);

    AudioFile *getAudioFile()  { return m_audioFile; }
    const AudioFile *getAudioFile() const  { return m_audioFile; }

    /// Find threshold crossing points
    /**
     * Get pairs of split points for areas that exceed a percentage
     * threshold.
     */
    std::vector<SplitPointPair> getSplitPoints(const RealTime &startTime,
                                               const RealTime &endTime,
                                               int threshold,
                                               const RealTime &minLength);

    /// For debugging
    void printStats();

    /// Write peak chunk to file handle (BWF)
    //bool writeToHandle(std::ofstream *file, unsigned short updatePercentage);

    //int getVersion() const { return m_version; }
    //int getFormat() const { return m_format; }
    //int getPointsPerValue() const { return m_pointsPerValue; }
    //int getBlockSize() const { return m_blockSize; }
    //int getChannels() const { return m_channels; }
    //int getNumberOfPeaks() const { return m_numberOfPeaks; }
    //int getPositionPeakOfPeaks() const { return m_positionPeakOfPeaks; }
    //int getOffsetToPeaks() const { return m_offsetToPeaks; }
    //int getBodyBytes() const { return m_bodyBytes; }
    //QDateTime getModificationTime() const { return m_modificationTime; }
    //std::streampos getChunkStartPosition() const
    //    { return m_chunkStartPosition; }

protected:
    /// Build up a header string and then pump it out to the file handle
    void writeHeader(std::ofstream *file);
    void writePeaks(std::ofstream *file);

    /// Convert time to block.
    /**
     * rename: getBlock()
     */
    int getPeak(const RealTime &time);

    /// Convert block to time.
    RealTime getTime(int block);

    void parseHeader();

    /// The AudioFile that this peak file is based on.
    AudioFile *m_audioFile;

    // Peak Envelope Chunk parameters
    int m_version;
    int m_format;  // bytes in peak value (1 or 2)
    int m_pointsPerValue;
    int m_blockSize;
    int m_channels;
    int m_numberOfPeaks;
    int m_positionPeakOfPeaks;
    int m_offsetToPeaks;
    int m_bodyBytes;

    /// Used to determine whether the peak file is out of sync with the audio file.
    QDateTime m_modificationTime;

    /// Always zero since we don't support adding a peak chunk to an audio file.
    std::streampos m_chunkStartPosition;

    // Parameters from the last call to getPreview() to determine whether
    // m_lastPreviewCache is valid for a specific request.
    RealTime           m_lastPreviewStartTime;
    RealTime           m_lastPreviewEndTime;
    int                m_lastPreviewWidth;
    bool               m_lastPreviewShowMinima;
    /// Cached preview to speed up getPreview().
    std::vector<float> m_lastPreviewCache;

    /// Optional progress dialog for write().
    QPointer<QProgressDialog> m_progressDialog;

    /// Cached in-memory copy of the peak file for getPreview().
    std::string        m_peakCache;
    
    bool scanToPeak(int peak);
    //bool scanForward(int numberOfPeaks);
};

}


#endif // RG_PEAKFILE_H


