// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*- /*
/*
  Rosegarden-4
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

#include <cmath>

#include <qdatetime.h>
#include <qstringlist.h>
#include <qpalette.h>
#include <kapp.h>

#include "PeakFile.h"
#include "AudioFile.h"
#include "Audio.h"
#include "Sound.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

PeakFile::PeakFile(AudioFile *audioFile):
    SoundFile(audioFile->getPeakFilename()),
    m_audioFile(audioFile),
    m_version(-1),                         // -1 defines new file - start at 0
    m_format(1),                           // default is 8-bit peak format
    m_pointsPerValue(0),
    m_blockSize(256),                      // default block size is 256 samples
    m_channels(0),
    m_numberOfPeaks(0),
    m_positionPeakOfPeaks(0),
    m_offsetToPeaks(0),
    m_modificationTime(QDate(1970, 1, 1), QTime(0, 0, 0)),
    m_chunkStartPosition(0)
{
}

PeakFile::~PeakFile()
{
}

PeakFile::PeakFile(const PeakFile &peak):
    QObject(peak.parent(), peak.name()),
    SoundFile(peak.getFilename()),
    m_audioFile(peak.getAudioFile()),
    m_version(peak.getVersion()),
    m_format(peak.getFormat()),
    m_pointsPerValue(peak.getPointsPerValue()),
    m_blockSize(peak.getBlockSize()),
    m_channels(peak.getChannels()),
    m_numberOfPeaks(peak.getNumberOfPeaks()),
    m_positionPeakOfPeaks(peak.getPositionPeakOfPeaks()),
    m_offsetToPeaks(peak.getOffsetToPeaks()),
    m_modificationTime(peak.getModificationTime()),
    m_chunkStartPosition(peak.getChunkStartPosition())
{
}



bool
PeakFile::open()
{
    // If we're already open then don't open again
    //
    if (m_inFile && m_inFile->is_open())
        return true;

    // Open
    //
    m_inFile = new std::ifstream(m_fileName.c_str(),
                                 std::ios::in | std::ios::binary);
    // Check we're open
    //
    if (!(*m_inFile))
        return false;

    try
    {
        parseHeader();
    }
    catch(std::string s)
    {
        cerr << "PeakFile::open - EXCEPTION \"" << s << "\""
             << endl;
        return false;
    }
    
    return true;
}

void
PeakFile::parseHeader()
{
    if (!(*m_inFile)) return;

    m_inFile->seekg(0, std::ios::beg);

    // get full header length
    //
    std::string header = getBytes(128);

#if (__GNUC__ < 3)
    if (header.compare(Rosegarden::AUDIO_BWF_PEAK_ID, 0, 4) != 0)
#else
    if (header.compare(0, 4, Rosegarden::AUDIO_BWF_PEAK_ID) != 0)
#endif
    {
        throw(std::string("PeakFile::parseHeader - can't find LEVL identifier"));
    }

    int length = getIntegerFromLittleEndian(header.substr(4, 4));

    // Get the length of the header minus the first 8 bytes
    //
    if (length == 0)
        throw(std::string("PeakFile::parseHeader - can't get header length"));

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

    QStringList dateTime = QStringList::split(":", dateString);

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
    cout << endl;
    cout << "STATS for PeakFile \"" << m_fileName << "\"" << endl
         << "-----" << endl << endl;

    cout << "  VERSION = " << m_version << endl
         << "  FORMAT  = " << m_format << endl
         << "  BYTES/VALUE = " << m_pointsPerValue << endl
         << "  BLOCKSIZE   = " << m_blockSize << endl
         << "  CHANNELS    = " << m_channels << endl
         << "  PEAK FRAMES = " << m_numberOfPeaks << endl
         << "  PEAK OF PKS = " << m_positionPeakOfPeaks << endl
         << endl;

    cout << "DATE" << endl
         << "----" << endl << endl
         << "  YEAR    = " << m_modificationTime.date().year() << endl
         << "  MONTH   = " << m_modificationTime.date().month()<< endl
         << "  DAY     = " << m_modificationTime.date().day() << endl
         << "  HOUR    = " << m_modificationTime.time().hour() << endl
         << "  MINUTE  = " << m_modificationTime.time().minute()
         << endl
         << "  SECOND  = " << m_modificationTime.time().second()
         << endl
         << "  MSEC    = " << m_modificationTime.time().msec()
         << endl << endl;
}

bool
PeakFile::write()
{
    return write(5); // default update every 5%
}

bool
PeakFile::write(unsigned short updatePercentage)
{
    if (m_outFile)
    {
        m_outFile->close();
        delete m_outFile; 
    }

    // Attempt to open AudioFile so that we can extract sample data
    // for preview file generation
    //
    try
    {
        if (!m_audioFile->open())
            return false;
    }
    catch(std::string e)
    {
        std::cerr << "PeakFile::write - \"" << e << "\"" << std::endl;
        return false;
    }

    // create and test that we've made it
    m_outFile = new std::ofstream(m_fileName.c_str(),
                                  std::ios::out | std::ios::binary);
    if (!(*m_outFile))
        return false;

    // write out the header
    writeHeader(m_outFile);

    // and now the peak values
    writePeaks(updatePercentage, m_outFile);

    return true;
}

// Close the peak file and tidy up
//
void
PeakFile::close()
{
    // Close any input file handle
    //
    if (m_inFile && m_inFile->is_open())
        {
            m_inFile->close();
            delete m_inFile;
            m_inFile = 0;
        }

    if (m_outFile == 0)
        return;

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

    std::string dateString(fDate.data());

    // Pad with spaces to make up to 28 bytes long and output
    //
    dateString += "     ";
    putBytes(m_outFile, dateString);

    // Ok, now close and tidy up
    //
    m_outFile->close();
    delete m_outFile;
    m_outFile = 0;
}

// If the audio file is more recently modified that the modification time
// on this peak file then we're invalid.  The action to rectify this is
// usually to regenerate the peak data.
//
bool
PeakFile::isValid()
{
    if (m_audioFile->getModificationDateTime() > m_modificationTime)
        return false;

    return true;
}

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

// Build up a header string and then pump it out to the file handle
//
void
PeakFile::writeHeader(std::ofstream *file)
{
    if(!file || !(*file)) return;

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

    //cout << "HEADER LENGTH = " << header.length() << endl;

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
    m_inFile->seekg(m_chunkStartPosition + std::streampos(128), std::ios::beg); 
    m_inFile->seekg(peak * m_format * m_channels * m_pointsPerValue,
                    std::ios::cur);

    // Ensure we re-read the input buffer if we're 
    // doing buffered reads as it's now meaningless
    //
    m_loseBuffer = true;

    if (m_inFile->eof())
    {
        m_inFile->clear();
        return false;
    }

    return true;
}

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

    if (m_inFile->eof())
    {
        m_inFile->clear();
        return false;
    }

    return true;
}


void
PeakFile::writePeaks(unsigned short /*updatePercentage*/,
                     std::ofstream *file)
{
    if(!file || !(*file)) return;

    cout << "PeakFile::writePeaks - calculating peaks" << endl;

    // Scan to beginning of audio data
    m_audioFile->scanTo(RealTime(0, 0));

    // Store our samples
    //
    std::vector<std::pair<int, int> > channelPeaks;
    std::string samples;
    char *samplePtr;

    int sampleValue;
    int sampleMax = 0 ;
    int sampleFrameCount = 0;

    int channels = m_audioFile->getChannels();
    int bytes = m_audioFile->getBitsPerSample() / 8;

    // for the progress dialog
    unsigned int apprxTotalBytes = m_audioFile->getSize();
    unsigned int byteCount = 0;

    for (int i = 0; i < channels; i++)
        channelPeaks.push_back(std::pair<int, int>());

    // clear down info
    m_numberOfPeaks = 0;
    m_bodyBytes = 0;
    m_positionPeakOfPeaks = 0;

    while(true)
    {
        try
        {
            samples = m_audioFile->
                getBytes(m_blockSize * channels * bytes);
        }
        catch (std::string e)
        {
            // do nothing
        }

        // If no bytes or less than the total number of bytes are returned
        // then break out
        //
        if (samples.length() == 0 ||
            samples.length() < (m_blockSize * m_audioFile->getChannels()
                                * bytes))
            break;

        byteCount += samples.length();

        emit setProgress((int)(double(byteCount)/
                               double(apprxTotalBytes) * 100.0));
        kapp->processEvents();

        samplePtr = (char *)samples.c_str();

        for (int i = 0; i < m_blockSize; i++)
        {
            for (unsigned int j = 0; j < m_audioFile->getChannels(); j++)
            {

                // Single byte format values range from 0-255 and then
                // shifted down about the x-axis.  Double byte and above
                // are already centred about x-axis.
                //
                if(bytes == 1)
                {
                    // get value
                    sampleValue = (unsigned char)(*samplePtr)
                                      - (int(SAMPLE_MAX_8BIT) / 2);
                    samplePtr++;

                }
                else if (bytes == 2)
                {
                    sampleValue = (int)(*((short *)samplePtr));
                    samplePtr += 2;
                }
                else
                {
                    throw(std::string("PeakFile::writePeaks - unsupported bit depth"));
                }

                // First time for each channel
                //
                if (i == 0)
                {
                    channelPeaks[j].first = sampleValue;
                    channelPeaks[j].second = sampleValue;
                }
                else
                {
                    // Compare and store
                    //
                    if (sampleValue > channelPeaks[j].first)
                        channelPeaks[j].first = sampleValue;

                    if (sampleValue < channelPeaks[j].second)
                        channelPeaks[j].second = sampleValue;
                }

                // Store peak of peaks if it fits
                //
                if (abs(sampleValue) > sampleMax)
                {
                    sampleMax = abs(sampleValue);
                    m_positionPeakOfPeaks = sampleFrameCount;
                }
            }

            // for peak of peaks as well as frame count
            sampleFrameCount++;
        }

        // Write absolute peak data in channel order
        //
        for (unsigned int i = 0; i < m_audioFile->getChannels(); i++)
        {
            putBytes(file, getLittleEndianFromInteger(channelPeaks[i].first,
                                                      bytes));
            putBytes(file, getLittleEndianFromInteger(channelPeaks[i].second,
                                                      bytes));
            m_bodyBytes += bytes * 2;
        }

        // increment number of peak frames
        m_numberOfPeaks++;
    }

    // set format number
    m_format = bytes;

    cout << "PeakFile::writePeaks - "
              << "completed peaks" << endl;

}

// Get a normalised vector for the preview at a given horizontal resolution.
// We return a value for each channel and if returnLow is set we also return
// an interleaved low value for each channel.
//
//
std::vector<float>
PeakFile::getPreview(const RealTime &startTime,
                     const RealTime &endTime,
                     int width,
                     bool showMinima)
{
    std::vector<float> ret;

    int startPeak = getPeak(startTime);
    int endPeak = getPeak(endTime);

    // Sanity check
    if (startPeak > endPeak)
        return ret;

    // Actual possible sample length in RealTime
    //
    double step = double(endPeak - startPeak) / double(width);
    std::string peakData;
    int peakNumber;
    float hiValue = 0.0f;
    float loValue = 0.0f;

    cout << "PeakFile::getPreview - getting preview for \""
              << m_audioFile->getFilename() << "\"" << endl;

    // Get a divisor
    //
    float divisor = 0.0f;
    switch(m_format)
    {
        case 1:
            divisor = SAMPLE_MAX_8BIT;
            break;

        case 2:
            divisor = SAMPLE_MAX_16BIT;
            break;

        default:
            cerr << "PeakFile::getPreview - "
                      << "unsupported peak length format (" << m_format << ")"
                      << endl;
            return ret;
    }

    for (int i = 0; i < width; i++)
    {
        peakNumber = startPeak + int(double(i) * step);

        // Seek to value
        //
        if (scanToPeak(peakNumber) == false)
            ret.push_back(0.0f);

        // Get peak value over channels
        //
        for (int j = 0; j < m_channels; j++)
        {

            hiValue = 0.0f;
            loValue = 0.0f;

            try
            {
                peakData = getBytes(m_inFile, m_format * m_pointsPerValue);
            }
            catch (std::string e)
            {
                // Problem with the get - probably an EOF
                // return the results so far.
                //
                /*
                cout << "PeakFile::getPreview - \"" << e << "\"\n"
                          << endl;
                */
                return ret;
            }

            if (peakData.length() == (unsigned int)(m_format *
                                                    m_pointsPerValue))
            {
                int intDivisor = int(divisor);
                int inValue =
                    getIntegerFromLittleEndian(peakData.substr(0, m_format));

		while (inValue > intDivisor) {
		    inValue -= (1 << (m_format * 8));
		}

                hiValue += float(inValue);

                if (m_pointsPerValue == 2)
                {
                    inValue = 
                        getIntegerFromLittleEndian(
                                peakData.substr(m_format, m_format));

		    while (inValue > intDivisor) {
			inValue -= (1 << (m_format * 8));
		    }

                    loValue += float(inValue);
                }
            }
            else
            {
                // We didn't get the whole peak block - return what
                // we've got so far
                //
                cerr << "PeakFile::getPreview - "
                          << "failed to get complete peak block"
                          << endl;
                return ret;
            }

            //cout << "VALUE = " << hiValue / divisor << endl;

            // Always push back high value
            ret.push_back(hiValue / divisor);

            if (showMinima)
                ret.push_back(loValue / divisor);
        }
    }

    return ret;
}

int
PeakFile::getPeak(const RealTime &time)
{
    double frames = ((time.sec * 1000000.0) + time.usec) *
                        m_audioFile->getSampleRate() / 1000000.0;
    return int(frames / double(m_blockSize));
}

RealTime
PeakFile::getTime(int peak)
{
    int usecs = int((double)peak * (double)m_blockSize *
        double(1000000.0) / double(m_audioFile->getSampleRate()));
    return RealTime(usecs/1000000, usecs % 1000000);
}

// Get pairs of split points for areas that exceed a percentage
// threshold
//
std::vector<SplitPointPair> 
PeakFile::getSplitPoints(const RealTime &startTime,
                         const RealTime &endTime,
                         int threshold)
{
    std::vector<SplitPointPair> points;
    std::string peakData;

    int startPeak = getPeak(startTime);
    int endPeak = getPeak(endTime);

    if (endPeak < startPeak) return std::vector<SplitPointPair>();

    scanToPeak(startPeak);

    float divisor = 0.0f;
    switch(m_format)
    {
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
    float fThreshold = float(threshold)/100.0;
    bool belowThreshold = true;
    RealTime startSplit;

    for (int i = startPeak; i < endPeak; i++)
    {
        value = 0.0;

        for (int j = 0; j < m_channels; j++)
        {
            try
            {
                peakData = getBytes(m_inFile, m_format * m_pointsPerValue);
            }
            catch (std::string e)
            {
                break;
            }

            if (peakData.length() == (unsigned int)(m_format *
                                                    m_pointsPerValue))
            {
                int peakValue =
                    getIntegerFromLittleEndian(peakData.substr(0, m_format));

                value += fabs(float(peakValue) / divisor);
            }
        }

        value /= float(m_channels);

        if (belowThreshold)
        {
            if (value > fThreshold)
            {
                startSplit = getTime(i);
                belowThreshold = false;
            }
        }
        else
        {
            if (value < fThreshold)
            {
                // insert values
                points.push_back(SplitPointPair(startSplit, getTime(i)));
                belowThreshold = true;
            }
        }
    }

    // if we've got a split point open the close it
    if (belowThreshold == false)
    {
        points.push_back(SplitPointPair(startSplit,
                                        getTime(endPeak)));
    }

    return points;
}


}


