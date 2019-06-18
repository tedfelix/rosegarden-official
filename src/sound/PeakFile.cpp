/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2018 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PeakFile]"

#include <algorithm>  // std::max()
#include <cmath>  // std::fabs()
#include <unistd.h>  // usleep()
#include <iostream>
#include <string>
#include <utility>  // std::pair
#include <vector>

#include <QDateTime>
#include <QProgressDialog>
#include <QStringList>

#include "PeakFile.h"
#include "AudioFile.h"
#include "base/Profiler.h"
#include "misc/Debug.h"
#include "misc/Strings.h"

//#define DEBUG_PEAKFILE 1
//#define DEBUG_PEAKFILE_BRIEF 1
//#define DEBUG_PEAKFILE_CACHE 1

#ifdef DEBUG_PEAKFILE
#define DEBUG_PEAKFILE_BRIEF 1
#endif

// Slow down the process so the progress dialog can be seen even with
// very small files.
// !!! DO NOT SHIP WITH THIS SET TO 1 !!!
#define TEST_PROGRESS_DIALOG 0
// !!! DO NOT SHIP WITH THIS SET TO 1 !!!

static const float SAMPLE_MAX_8BIT  = (float)(0xff);
static const float SAMPLE_MAX_16BIT = (float)(0xffff/2);
static const float SAMPLE_MAX_24BIT = (float)(0xffffff/2);
static const char AUDIO_BWF_PEAK_ID[] = "levl";  // BWF peak chunk id

namespace Rosegarden
{

PeakFile::PeakFile(AudioFile *audioFile) :
        SoundFile(audioFile->getPeakFilename()),
        m_audioFile(audioFile),
        m_version( -1),                          // -1 defines new file - start at 0
        m_format(1),                            // default is 8-bit peak format
        m_pointsPerValue(0),
        m_blockSize(256),                       // default block size is 256 samples
        m_channels(0),
        m_numberOfPeaks(0),
        m_positionPeakOfPeaks(0),
        m_offsetToPeaks(0),
        m_bodyBytes(0),
        m_modificationTime(QDate(1970, 1, 1), QTime(0, 0, 0)),
        m_chunkStartPosition(0),
        m_lastPreviewStartTime(0, 0),
        m_lastPreviewEndTime(0, 0),
        m_lastPreviewWidth( -1),
        m_lastPreviewShowMinima(false)
{
}

PeakFile::~PeakFile()
{
}

bool
PeakFile::open()
{
    // Set the file size
    //
    QFileInfo info(m_fileName);
    m_fileSize = (size_t)info.size(); // cast from qint64

    // If we're already open then don't open again
    //
    if (m_inFile && m_inFile->is_open())
        return true;

    // Open
    //
    m_inFile = new std::ifstream(m_fileName.toLocal8Bit(),
                                 std::ios::in | std::ios::binary);
    // Check we're open
    //
    if (!(*m_inFile))
        return false;

    try {
        parseHeader();
    } catch (const BadSoundFileException &s) {

#ifdef DEBUG_PEAKFILE
        RG_WARNING << "open() - EXCEPTION \"" << s.getMessage() << "\"";
#endif

        return false;
    }

    return true;
}

void
PeakFile::parseHeader()
{
    if (!(*m_inFile))
        return ;

    m_inFile->seekg(0, std::ios::beg);

    // get full header length
    //
    std::string header = getBytes(128);

    if (header.compare(0, 4, AUDIO_BWF_PEAK_ID) != 0) {
        throw(BadSoundFileException(m_fileName, "PeakFile::parseHeader - can't find LEVL identifier"));
    }

    int length = getIntegerFromLittleEndian(header.substr(4, 4));

    // Get the length of the header minus the first 8 bytes
    //
    if (length == 0)
        throw(BadSoundFileException(m_fileName, "PeakFile::parseHeader - can't get header length"));

    // Get the file information
    //
    m_version = getIntegerFromLittleEndian(header.substr(8, 4));
    m_format = getIntegerFromLittleEndian(header.substr(12, 4));
    m_pointsPerValue = getIntegerFromLittleEndian(header.substr(16, 4));
    m_blockSize = getIntegerFromLittleEndian(header.substr(20, 4));
    m_channels = getIntegerFromLittleEndian(header.substr(24, 4));
    m_numberOfPeaks = getIntegerFromLittleEndian(header.substr(28, 4));
    m_positionPeakOfPeaks = getIntegerFromLittleEndian(header.substr(32, 4));

    // Read in date string and convert it up to QDateTime
    //
    QString dateString = QString(header.substr(40, 28).c_str());

    QStringList dateTime = dateString.split(":", QString::SkipEmptyParts);

    m_modificationTime.setDate(QDate(dateTime[0].toInt(),
                                     dateTime[1].toInt(),
                                     dateTime[2].toInt()));

    m_modificationTime.setTime(QTime(dateTime[3].toInt(),
                                     dateTime[4].toInt(),
                                     dateTime[5].toInt(),
                                     dateTime[6].toInt()));

    //printStats();
}

void
PeakFile::printStats()
{
    RG_DEBUG << "printStats()";

    RG_DEBUG << "  STATS for PeakFile" << m_fileName;
    RG_DEBUG << "  ----------------------------";
    RG_DEBUG << "    VERSION =" << m_version;
    RG_DEBUG << "    FORMAT  =" << m_format;
    RG_DEBUG << "    BYTES/VALUE =" << m_pointsPerValue;
    RG_DEBUG << "    BLOCKSIZE   =" << m_blockSize;
    RG_DEBUG << "    CHANNELS    =" << m_channels;
    RG_DEBUG << "    PEAK FRAMES =" << m_numberOfPeaks;
    RG_DEBUG << "    PEAK OF PKS =" << m_positionPeakOfPeaks;
    RG_DEBUG << "";

    RG_DEBUG << "  DATE";
    RG_DEBUG << "  ----------------";
    RG_DEBUG << "    YEAR   =" << m_modificationTime.date().year();
    RG_DEBUG << "    MONTH  =" << m_modificationTime.date().month();
    RG_DEBUG << "    DAY    =" << m_modificationTime.date().day();
    RG_DEBUG << "    HOUR   =" << m_modificationTime.time().hour();
    RG_DEBUG << "    MINUTE =" << m_modificationTime.time().minute();
    RG_DEBUG << "    SECOND =" << m_modificationTime.time().second();
    RG_DEBUG << "    MSEC   =" << m_modificationTime.time().msec();
    RG_DEBUG << "";
}

bool
PeakFile::write()
{
    if (m_outFile) {
        m_outFile->close();
        delete m_outFile;
    }

    // Attempt to open AudioFile so that we can extract sample data
    // for preview file generation
    //
    try {
        if (!m_audioFile->open())
            return false;
    } catch (const BadSoundFileException &e) {
#ifdef DEBUG_PEAKFILE
        RG_WARNING << "write() - \"" << e.getMessage() << "\"";
#endif

        return false;
    }

    // create and test that we've made it
    m_outFile = new std::ofstream(m_fileName.toLocal8Bit(),
                                  std::ios::out | std::ios::binary);
    if (!(*m_outFile))
        return false;

    // write out the header
    writeHeader(m_outFile);

    // and now the peak values
    writePeaks(m_outFile);

    return true;
}

void
PeakFile::close()
{
    // Close any input file handle
    //
    if (m_inFile && m_inFile->is_open()) {
        m_inFile->close();
        delete m_inFile;
        m_inFile = nullptr;
    }

    if (m_outFile == nullptr)
        return ;

    // Seek to start of chunk
    //
    m_outFile->seekp(m_chunkStartPosition, std::ios::beg);

    // Seek to size field at set it
    //
    m_outFile->seekp(4, std::ios::cur);
    putBytes(m_outFile, getLittleEndianFromInteger(m_bodyBytes + 120, 4));

    // Seek to format and set it (m_format is only set at the
    // end of writePeaks()
    //
    m_outFile->seekp(4, std::ios::cur);
    putBytes(m_outFile, getLittleEndianFromInteger(m_format, 4));

    // Seek to number of peak frames and write value
    //
    m_outFile->seekp(12, std::ios::cur);
    putBytes(m_outFile,
             getLittleEndianFromInteger(m_numberOfPeaks, 4));

    // Peak of peaks
    //
    putBytes(m_outFile,
             getLittleEndianFromInteger(m_positionPeakOfPeaks, 4));

    // Seek to date field
    //
    m_outFile->seekp(4, std::ios::cur);

    // Set modification time to now
    //
    m_modificationTime = m_modificationTime.currentDateTime();

    QString fDate;
    fDate.sprintf("%04d:%02d:%02d:%02d:%02d:%02d:%03d",
                  m_modificationTime.date().year(),
                  m_modificationTime.date().month(),
                  m_modificationTime.date().day(),
                  m_modificationTime.time().hour(),
                  m_modificationTime.time().minute(),
                  m_modificationTime.time().second(),
                  m_modificationTime.time().msec());

    std::string dateString( qStrToStrLocal8( fDate )  );

    // Pad with spaces to make up to 28 bytes long and output
    //
    dateString += "     ";
    putBytes(m_outFile, dateString);

    // Ok, now close and tidy up
    //
    m_outFile->close();
    delete m_outFile;
    m_outFile = nullptr;
}

bool
PeakFile::isValid()
{
    if (m_audioFile->getModificationDateTime() > m_modificationTime)
        return false;

    return true;
}

#if 0
bool
PeakFile::writeToHandle(std::ofstream *file,
                        unsigned short /*updatePercentage*/)
{
    // Remember the position where we pass in the ofstream pointer
    // so we can return there to write close() information.
    //
    m_chunkStartPosition = file->tellp();

    return false;
}
#endif

void
PeakFile::writeHeader(std::ofstream *file)
{
    if (!file || !(*file))
        return ;

    std::string header;

    // The "levl" identifer for this chunk
    //
    header += AUDIO_BWF_PEAK_ID;

    // Add a four byte version of the size of the header chunk (120
    // bytes from this point onwards)
    //
    header += getLittleEndianFromInteger(120, 4);

    // A four byte version number (incremented every time)
    //
    header += getLittleEndianFromInteger(++m_version, 4);

    // Format of the peak points - 1 = unsigned char
    //                             2 = unsigned short
    //
    header += getLittleEndianFromInteger(m_format, 4);

    // Points per value          - 1 = 1 peak and has vertical about x-axis
    //                             2 = 2 peaks so differs above and below x-axis
    //
    // .. hardcode to 2 for the mo
    m_pointsPerValue = 2;
    header += getLittleEndianFromInteger(m_pointsPerValue, 4);

    // Block size - default and recommended is 256
    //
    header += getLittleEndianFromInteger(m_blockSize, 4);

    // Set channels up if they're currently empty
    //
    if (m_channels == 0 && m_audioFile)
        m_channels = m_audioFile->getChannels();

    // Peak channels - same as AudioFile channels
    //
    header += getLittleEndianFromInteger(m_channels, 4);

    // Number of peak frames - we write this at close() and so
    // for the moment put spacing 0's in.
    header += getLittleEndianFromInteger(0, 4);

    // Position of peak of peaks - written at close()
    //
    header += getLittleEndianFromInteger(0, 4);

    // Offset to start of peaks - usually the total size of this header
    //
    header += getLittleEndianFromInteger(128, 4);

    // Creation timestamp - fill in on close() so just use spacing
    // of 28 bytes for the moment.
    //
    header += getLittleEndianFromInteger(0, 28);

    // reserved space - 60 bytes
    header += getLittleEndianFromInteger(0, 60);

    //RG_DEBUG << "writeHeader(): HEADER LENGTH =" << header.length();

    // write out the header
    //
    putBytes(file, header);
}

bool
PeakFile::scanToPeak(int peak)
{
    if (!m_inFile)
        return false;

    if (!m_inFile->is_open())
        return false;

    // Scan to start of chunk and then seek to peak number
    //
    ssize_t pos = (ssize_t)m_chunkStartPosition + 128 +
                  peak * m_format * m_channels * m_pointsPerValue;

    ssize_t off = pos - m_inFile->tellg();

    if (off == 0) {
        return true;
    } else if (off < 0) {
        //RG_WARNING << "scanToPeak(): warning: seeking backwards for peak " << peak << " (" << m_inFile->tellg() << " -> " << pos << ")";
        m_inFile->seekg(pos);
    } else {
        m_inFile->seekg(off, std::ios::cur);
    }

    // Ensure we re-read the input buffer if we're
    // doing buffered reads as it's now meaningless
    //
    m_loseBuffer = true;

    if (m_inFile->eof()) {
        m_inFile->clear();
        return false;
    }

    return true;
}

#if 0
bool
PeakFile::scanForward(int numberOfPeaks)
{
    if (!m_inFile)
        return false;

    if (!m_inFile->is_open())
        return false;

    // Seek forward and number of peaks
    //
    m_inFile->seekg(numberOfPeaks * m_format * m_channels * m_pointsPerValue,
                    std::ios::cur);

    // Ensure we re-read the input buffer
    m_loseBuffer = true;

    if (m_inFile->eof()) {
        m_inFile->clear();
        return false;
    }

    return true;
}
#endif

void
PeakFile::writePeaks(std::ofstream *file)
{
    if (!file || !(*file))
        return ;

#ifdef DEBUG_PEAKFILE
    RG_DEBUG << "writePeaks() - calculating peaks";
#endif

    // Scan to beginning of audio data
    m_audioFile->scanTo(RealTime(0, 0));

    // Store our samples
    //
    std::vector<std::pair<int, int> > channelPeaks;
    std::string samples;
    unsigned char *samplePtr;

    int sampleValue;
    int sampleMax = 0 ;
    int sampleFrameCount = 0;

    int channels = m_audioFile->getChannels();
    int bytes = m_audioFile->getBitsPerSample() / 8;

    m_format = bytes;
    if (bytes == 3 || bytes == 4) // 24-bit PCM or 32-bit float
        m_format = 2; // write 16-bit PCM instead

    // for the progress dialog
    size_t apprxTotalBytes = m_audioFile->getSize();
    size_t byteCount = 0;

    for (int i = 0; i < channels; i++)
        channelPeaks.push_back(std::pair<int, int>());

    // clear down info
    m_numberOfPeaks = 0;
    m_bodyBytes = 0;
    m_positionPeakOfPeaks = 0;

    // ??? Block count?  How does this differ from m_numberOfPeaks?
    int ct = 0;

    // ??? for each block...?
    while (true) {
        try {
            // Read a block
            samples = m_audioFile->
                      getBytes(m_blockSize * channels * bytes);
        } catch (const BadSoundFileException &e) {
            RG_WARNING << "writePeaks():" << e.getMessage();
            break;
        }

        // If no bytes or less than the total number of bytes are returned
        // then break out
        //
        if (samples.length() == 0 ||
            samples.length() < (m_blockSize * m_audioFile->getChannels()
                                * bytes))
            break;

        byteCount += samples.length();

#if !TEST_PROGRESS_DIALOG
        // ??? Every 2000 blocks?  That's around 2Mbytes?
        if (ct % 2000 == 0) {
#else
// Testing the progress dialogs.
        // Slow things down so we can test the progress dialog.
        usleep(10000);

        // ??? Every 10 blocks?  That's around 10kbytes!
        if (ct % 10 == 0) {
#endif
            int progress = static_cast<int>(double(byteCount) /
                    double(apprxTotalBytes) * 100.0);

            //RG_DEBUG << "writePeaks(): progress" << progress;

            if (m_progressDialog) {
                if (m_progressDialog->wasCanceled())
                    break;

                m_progressDialog->setValue(progress);
            }

            qApp->processEvents(QEventLoop::AllEvents);
        }
        ++ct;

        samplePtr = (unsigned char *)samples.c_str();

        for (int i = 0; i < m_blockSize; i++) {
            for (unsigned int ch = 0; ch < m_audioFile->getChannels(); ch++) {
                // Single byte format values range from 0-255 and then
                // shifted down about the x-axis.  Double byte and above
                // are already centred about x-axis.
                //
                if (bytes == 1) {
                    // get value
                    sampleValue = int(*samplePtr) - 128;
                    samplePtr++;
                } else if (bytes == 2) {
                    unsigned char b2 = samplePtr[0];
                    unsigned char b1 = samplePtr[1];
                    unsigned int bits = (b1 << 8) + b2;
                    sampleValue = (short)bits;
                    samplePtr += 2;
                } else if (bytes == 3) {
                    unsigned char b3 = samplePtr[0];
                    unsigned char b2 = samplePtr[1];
                    unsigned char b1 = samplePtr[2];
                    unsigned int bits = (b1 << 24) + (b2 << 16) + (b3 << 8);

                    // write out as 16-bit (m_format == 2)
                    sampleValue = int(bits) / 65536;

                    samplePtr += 3;
                } else if (bytes == 4)  // IEEE float (enforced by RIFFAudioFile)
                {
                    // write out as 16-bit (m_format == 2)
                    float val = *(float *)samplePtr;
                    sampleValue = (int)(32767.0 * val);
                    samplePtr += 4;
                } else {
                    throw(BadSoundFileException(m_fileName, "PeakFile::writePeaks - unsupported bit depth"));
                }

                // First time for each channel
                //
                if (i == 0) {
                    channelPeaks[ch].first = sampleValue;
                    channelPeaks[ch].second = sampleValue;
                } else {
                    // Compare and store
                    //
                    if (sampleValue > channelPeaks[ch].first)
                        channelPeaks[ch].first = sampleValue;

                    if (sampleValue < channelPeaks[ch].second)
                        channelPeaks[ch].second = sampleValue;
                }

                // Store peak of peaks if it fits
                //
                if (std::abs(sampleValue) > sampleMax) {
                    sampleMax = std::abs(sampleValue);
                    m_positionPeakOfPeaks = sampleFrameCount;
                }
            }

            // for peak of peaks as well as frame count
            sampleFrameCount++;
        }

        // Write absolute peak data in channel order
        //
        for (unsigned int i = 0; i < m_audioFile->getChannels(); i++) {
            putBytes(file, getLittleEndianFromInteger(channelPeaks[i].first,
                     m_format));
            putBytes(file, getLittleEndianFromInteger(channelPeaks[i].second,
                     m_format));
            m_bodyBytes += m_format * 2;
        }

        // increment number of peak frames
        m_numberOfPeaks++;
    }

#ifdef DEBUG_PEAKFILE
    RG_DEBUG << "writePeaks() - completed peaks";
#endif

}

std::vector<float>
PeakFile::getPreview(const RealTime &startTime,
                     const RealTime &endTime,
                     int width,
                     bool showMinima)
{
#ifdef DEBUG_PEAKFILE_BRIEF
    RG_DEBUG << "getPreview() - startTime = " << startTime
             << ", endTime = " << endTime
             << ", width = " << width
             << ", showMinima = " << showMinima;
#endif

    if (getSize() == 0) {
        RG_DEBUG << "getPreview() - PeakFile size == 0";
        return std::vector<float>();
    }

    // Regenerate cache on these conditions
    //
    if (!m_peakCache.length()) {
#ifdef DEBUG_PEAKFILE_CACHE
        RG_DEBUG << "getPreview() - no peak cache";
#endif

        if (getSize() < (256 *1024)) // if less than 256K PeakFile
        {
            // Scan to start of peak data
            scanToPeak(0);
            try
            {
                m_peakCache = getBytes(m_inFile, getSize() - 128);
            } catch (const BadSoundFileException &e)
            {
                RG_WARNING << "PeakFile::getPreview: " << e.getMessage();
            }

#ifdef DEBUG_PEAKFILE_CACHE
            RG_DEBUG << "getPreview() - generated peak cache - size = " << m_peakCache.length();
#endif

        } else {
#ifdef DEBUG_PEAKFILE_CACHE
            RG_DEBUG << "getPreview() - file size = " << getSize() << ", not generating cache";
#endif

        }
    }

    // Check to see if we hit the "lastPreview" cache by comparing the last
    // query parameters we used.
    //
    if (startTime == m_lastPreviewStartTime && endTime == m_lastPreviewEndTime
            && width == m_lastPreviewWidth && showMinima == m_lastPreviewShowMinima) {
#ifdef DEBUG_PEAKFILE_CACHE
        RG_DEBUG << "getPreview() - hit last preview cache";
#endif

        return m_lastPreviewCache;
    } else {
#ifdef DEBUG_PEAKFILE_CACHE
        RG_DEBUG << "getPreview() - last preview " << m_lastPreviewStartTime << " -> " << m_lastPreviewEndTime << ", w " << m_lastPreviewWidth << "; this " << startTime << " -> " << endTime << ", w " << width;
#endif

    }

    // Clear the cache - we need to regenerate it
    //
    m_lastPreviewCache.clear();

    int startPeak = getPeak(startTime);
    int endPeak = getPeak(endTime);

    // Sanity check
    if (startPeak > endPeak)
        return m_lastPreviewCache;

    // Actual possible sample length in RealTime
    //
    double step = double(endPeak - startPeak) / double(width);
    std::string peakData;
    int peakNumber;

#ifdef DEBUG_PEAKFILE_BRIEF
    RG_DEBUG << "getPreview() - getting preview for \"" << m_audioFile->getFilename() << "\"";
#endif

    // Get a divisor
    //
    float divisor = 0.0f;
    switch (m_format) {
    case 1:
        divisor = SAMPLE_MAX_8BIT;
        break;

    case 2:
        divisor = SAMPLE_MAX_16BIT;
        break;

    default:
#ifdef DEBUG_PEAKFILE_BRIEF
        RG_DEBUG << "getPreview() - unsupported peak length format (" << m_format << ")";
#endif

        return m_lastPreviewCache;
    }

    float *hiValues = new float[m_channels];
    float *loValues = new float[m_channels];

    for (int i = 0; i < width; i++) {

        peakNumber = startPeak + int(double(i) * step);
        int nextPeakNumber = startPeak + int(double(i + 1) * step);

        // Seek to value
        //
        if (!m_peakCache.length()) {

            if (scanToPeak(peakNumber) == false) {
#ifdef DEBUG_PEAKFILE
                RG_DEBUG << "getPreview(): scanToPeak(" << peakNumber << ") failed";
#endif

                m_lastPreviewCache.push_back(0.0f);
            }
        }
#ifdef DEBUG_PEAKFILE
        RG_DEBUG << "getPreview(): step is " << step << ", format * pointsPerValue * chans is " << (m_format * m_pointsPerValue * m_channels);
        RG_DEBUG << "              i = " << i << ", peakNumber = " << peakNumber << ", nextPeakNumber = " << nextPeakNumber;
#endif

        for (int ch = 0; ch < m_channels; ch++) {
            hiValues[ch] = 0.0f;
            loValues[ch] = 0.0f;
        }

        // Get peak value over channels
        //
        for (int k = 0; peakNumber < nextPeakNumber; ++k) {

            for (int ch = 0; ch < m_channels; ch++) {

                if (!m_peakCache.length()) {

                    try {
                        peakData = getBytes(m_inFile, m_format * m_pointsPerValue);
                    } catch (const BadSoundFileException &e) {
                        // Problem with the get - probably an EOF
                        // return the results so far.
                        //
#ifdef DEBUG_PEAKFILE
                        RG_DEBUG << "getPreview() - \"" << e.getMessage() << "\"";
#endif

                        goto done;
                    }
#ifdef DEBUG_PEAKFILE
                    RG_DEBUG << "getPreview() - read from file";
#endif

                } else {

                    int valueNum = peakNumber * m_channels + ch;
                    int charNum = valueNum * m_format * m_pointsPerValue;
                    int charLength = m_format * m_pointsPerValue;

                    // Get peak value from the cached string if
                    // the value is valid.
                    //
                    if (charNum + charLength <= (int)m_peakCache.length()) {
                        peakData = m_peakCache.substr(charNum, charLength);
#ifdef DEBUG_PEAKFILE

                        RG_DEBUG << "getPreview() - hit peakCache";
#endif

                    }
                }


                if (peakData.length() != (unsigned int)(m_format *
                                                        m_pointsPerValue)) {
                    // We didn't get the whole peak block - return what
                    // we've got so far
                    //
#ifdef DEBUG_PEAKFILE
                    RG_DEBUG << "getPreview() - failed to get complete peak block";
#endif

                    goto done;
                }

                int intDivisor = int(divisor);
                int inValue =
                    getIntegerFromLittleEndian(peakData.substr(0, m_format));

                while (inValue > intDivisor) {
                    inValue -= (1 << (m_format * 8));
                }

#ifdef DEBUG_PEAKFILE
                RG_DEBUG << "getPreview() - found potential hivalue " << inValue;
#endif

                if (k == 0 || inValue > hiValues[ch]) {
                    hiValues[ch] = float(inValue);
                }

                if (m_pointsPerValue == 2) {

                    inValue =
                        getIntegerFromLittleEndian(
                            peakData.substr(m_format, m_format));

                    while (inValue > intDivisor) {
                        inValue -= (1 << (m_format * 8));
                    }

                    if (k == 0 || inValue < loValues[ch]) {
                        loValues[ch] = inValue;
                    }
                }
            }

            ++peakNumber;
        }

        for (int ch = 0; ch < m_channels; ++ch) {

            float value = hiValues[ch] / divisor;

#ifdef DEBUG_PEAKFILE_BRIEF
            RG_DEBUG << "getPreview() - VALUE = " << hiValues[ch] / divisor;
#endif

            if (showMinima) {
                m_lastPreviewCache.push_back(loValues[ch] / divisor);
            } else {
                value = std::fabs(value);
                if (m_pointsPerValue == 2) {
                    value = std::max(value, std::fabs(loValues[ch] / divisor));
                }
                m_lastPreviewCache.push_back(value);
            }
        }
    }

done:
    resetStream();
    delete[] hiValues;
    delete[] loValues;

    // We have a good preview in the cache so store our parameters
    //
    m_lastPreviewStartTime = startTime;
    m_lastPreviewEndTime = endTime;
    m_lastPreviewWidth = width;
    m_lastPreviewShowMinima = showMinima;

#ifdef DEBUG_PEAKFILE_BRIEF
    RG_DEBUG << "getPreview() - Returning " << m_lastPreviewCache.size() << " items";
#endif

    return m_lastPreviewCache;
}

int
PeakFile::getPeak(const RealTime &time)
{
    double frames = ((time.sec * 1000000.0) + time.usec()) *
                    m_audioFile->getSampleRate() / 1000000.0;
    return int(frames / double(m_blockSize));
}

RealTime
PeakFile::getTime(int peak)
{
    int usecs = int((double)peak * (double)m_blockSize *
                    double(1000000.0) / double(m_audioFile->getSampleRate()));
    return RealTime(usecs / 1000000, (usecs % 1000000) * 1000);
}

std::vector<SplitPointPair>
PeakFile::getSplitPoints(const RealTime &startTime,
                         const RealTime &endTime,
                         int threshold,
                         const RealTime &minLength)
{
    std::vector<SplitPointPair> points;
    std::string peakData;

    int startPeak = getPeak(startTime);
    int endPeak = getPeak(endTime);

    if (endPeak < startPeak)
        return std::vector<SplitPointPair>();

    scanToPeak(startPeak);

    float divisor = 0.0f;
    switch (m_format) {
    case 1:
        divisor = SAMPLE_MAX_8BIT;
        break;

    case 2:
        divisor = SAMPLE_MAX_16BIT;
        break;

    default:
        return points;
    }

    float value;
    float fThreshold = float(threshold) / 100.0;
    bool belowThreshold = true;
    RealTime startSplit = RealTime::zeroTime;
    bool inSplit = false;

    for (int i = startPeak; i < endPeak; i++) {
        value = 0.0;

        for (int ch = 0; ch < m_channels; ch++) {
            try {
                peakData = getBytes(m_inFile, m_format * m_pointsPerValue);
            } catch (const BadSoundFileException &e) {
                RG_WARNING << "getSplitPoints(): " << e.getMessage();
                break;
            }

            if (peakData.length() == (unsigned int)(m_format *
                                                    m_pointsPerValue)) {
                int peakValue =
                    getIntegerFromLittleEndian(peakData.substr(0, m_format));

                value += std::fabs(float(peakValue) / divisor);
            }
        }

        value /= float(m_channels);

        if (belowThreshold) {
            if (value > fThreshold) {
                startSplit = getTime(i);
                inSplit = true;
                belowThreshold = false;
            }
        } else {
            if (value < fThreshold && getTime(i) - startSplit > minLength) {
                // insert values
                if (inSplit) {
                    points.push_back(SplitPointPair(startSplit, getTime(i)));
                }
                inSplit = false;
                belowThreshold = true;
            }
        }
    }

    // if we've got a split point open the close it
    if (inSplit) {
        points.push_back(SplitPointPair(startSplit,
                                        getTime(endPeak)));
    }

    return points;
}


}


