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

#include <qdatetime.h>
#include <qstringlist.h>

#include "PeakFile.h"
#include "AudioFile.h"
#include "Audio.h"
#include "Sound.h"

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
        std::cerr << "PeakFile::open - EXCEPTION \"" << s << "\""
                  << std::endl;
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

    printStats();

}

void
PeakFile::printStats()
{
    std::cout << std::endl;
    std::cout << "STATS for PeakFile \"" << m_fileName << "\"" << std::endl
              << "-----" << std::endl << std::endl;

    std::cout << "  VERSION = " << m_version << std::endl
              << "  FORMAT  = " << m_format << std::endl
              << "  BYTES/VALUE = " << m_pointsPerValue << std::endl
              << "  BLOCKSIZE   = " << m_blockSize << std::endl
              << "  CHANNELS    = " << m_channels << std::endl
              << "  PEAK FRAMES = " << m_numberOfPeaks << std::endl
              << "  PEAK OF PKS = " << m_positionPeakOfPeaks << std::endl
              << std::endl;

    std::cout << "DATE" << std::endl
              << "----" << std::endl << std::endl
              << "  YEAR    = " << m_modificationTime.date().year() << std::endl
              << "  MONTH   = " << m_modificationTime.date().month()<< std::endl
              << "  DAY     = " << m_modificationTime.date().day() << std::endl
              << "  HOUR    = " << m_modificationTime.time().hour() << std::endl
              << "  MINUTE  = " << m_modificationTime.time().minute()
              << std::endl
              << "  SECOND  = " << m_modificationTime.time().second()
              << std::endl
              << "  MSEC    = " << m_modificationTime.time().msec()
              << std::endl << std::endl;
}

bool
PeakFile::write()
{
    return write(5); // default update every 5%
}

bool
PeakFile::write(unsigned short undatePercentage)
{
    if (m_outFile)
    {
        m_outFile->close();
        delete m_outFile; 
    }

    // Attempt to open AudioFile so that we can extract sample data
    // for preview file generation
    //
    if (!m_audioFile->open())
      return false;

    // create and test that we've made it
    m_outFile = new std::ofstream(m_fileName.c_str(),
                                  std::ios::out | std::ios::binary);
    if (!(*m_outFile))
        return false;

    // write out the header
    writeHeader(m_outFile);

    // and now the peak values
    writePeaks(m_outFile);

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

    // Seek to number of peak frames and write value
    //
    m_outFile->seekp(20, std::ios::cur);
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
                        unsigned short updatePercentage)
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

    // Ensure we re-read the input buffer
    m_loseBuffer = true;

    /*
    if (m_inFile->eof())
        return false;
        */

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

    /*
    if (m_inFile->eof())
        return false;
        */

    return true;
}


void
PeakFile::writePeaks(std::ofstream *file)
{
    if(!file || !(*file)) return;

    std::cout << "PeakFile::writePeaks - calculating peaks" << std::endl;


    // Scan to beginning of data
    //
    m_audioFile->scanTo(RealTime(0, 0));


    // Store our samples
    //
    std::string samples;
    char *samplePtr;

    // Store our maxima and minima in this vector 
    // 
    std::vector<std::pair<int, int> > channelPeaks;

    for (unsigned int i = 0; i < m_audioFile->getChannels(); i++)
        channelPeaks.push_back(std::pair<int, int>());

    // Set the format level now we've written the peak data
    //
    int bytes = m_audioFile->getBitsPerSample() / 8;

    // Store peaks of peaks position
    //
    int sampleValue;
    int sampleMax = 0 ;
    int sampleFrameCount = 0;

    // Clear some totals
    //
    m_numberOfPeaks = 0;
    m_bodyBytes = 0;
    m_positionPeakOfPeaks = 0;

    // Always loop until we hit EOF
    //
    while(true)
    {
        // clear peak data
        samples.erase(samples.begin(), samples.end());

        // Get a block of bytes - blocksize * channels * bytes per sample
        //
        // We fetch from the AudioFile handle and place into the given
        // output file handle.
        //
        try
        {
            samples = m_audioFile->
                getBytes(m_blockSize * m_audioFile->getChannels() * bytes);
        }
        catch (std::string e)
        {
            std::cerr << "PeakFile::writePeaks - \"" << e << "\"" 
                      << std::endl
                      << "PeakFile::writePeaks - leaving last block" << std::endl;
            break;
        }

        // If no bytes or less than the total number of bytes are returned
        // then break out
        //
        if (samples.length() == 0 ||
            samples.length() < (m_blockSize * m_audioFile->getChannels()
                                * bytes))
            break;

        // The sample data pointer
        //
        samplePtr = (char *)samples.c_str();

        // Set the peaks (positive and negative) to zero for
        // each channel.
        // 
        for (unsigned int i = 0; i < m_audioFile->getChannels(); i++)
        {
            channelPeaks[i].first = 0;
            channelPeaks[i].second = 0;
        }

        // Scan over entire block
        //
        for (int i = 0; i < m_blockSize; i++)
        {
            for (unsigned int j = 0; j < m_audioFile->getChannels(); j++)
            {

                if(bytes == 1)
                {
                    // get value
                    sampleValue = *((unsigned char *)samplePtr);
                    samplePtr++;

                }
                else if (bytes == 2)
                {
                    sampleValue = *((short *)samplePtr);
                    samplePtr += 2;
                }
                else
                {
                    throw(std::string("PeakFile::writePeaks - unsupported bit depth"));
                }

                // Store maximum for this block
                //
                if (sampleValue > channelPeaks[i].first)
                    channelPeaks[j].first = sampleValue;
                else if (sampleValue < channelPeaks[j].second)
                    channelPeaks[j].second = sampleValue;

                // Make positive
                //
                if (sampleValue < 0)
                    sampleValue = -sampleValue;

                // Store peak of peaks if it fits
                //
                if (sampleValue > sampleMax)
                {
                    sampleMax = sampleValue;
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
            //std::cout << "PeakFile::writePeaks - "
                      //<< "writing peak data out" << std::endl;

            putBytes(file, getLittleEndianFromInteger(channelPeaks[i].first,
                                                      bytes));
            if (channelPeaks[i].second < 0)
                channelPeaks[i].second = -channelPeaks[i].second;

            putBytes(file, getLittleEndianFromInteger(channelPeaks[i].second,
                                                      bytes));
            // count up total body bytes
            //
            m_bodyBytes += bytes * 2;
        }

        // increment number of peak frames
        m_numberOfPeaks++;
    }

    // set format number
    m_format = bytes;

    std::cout << "PeakFile::writePeaks - "
              << "completed peaks" << std::endl;

}

// Get a normalised vector for the preview at a given horizontal
// resolution
//
std::vector<float>
PeakFile::getPreview(const RealTime &startIndex,
                     const RealTime &endIndex,
                     int resolution)
{
    std::vector<float> ret;
    int startPeak = ((startIndex.sec * 1000000.0 + startIndex.usec) *
                      m_audioFile->getSampleRate()) / (m_blockSize * 1000000);

    int endPeak = ((endIndex.sec * 1000000.0 + endIndex.usec) *
                      m_audioFile->getSampleRate()) / (m_blockSize * 1000000);

    if (startPeak > endPeak)
        return ret;

    float hiValue = 0.0f;
    float loValue = 0.0f;

    std::string peakData;

    std::cout << "PeakFile::getPreview - getting preview for \""
              << m_audioFile->getFilename() << "\"" << std::endl;

    // Walk through the peak values 
    //
    std::cout << "START = " << startPeak << std::endl;
    std::cout << "END   = " << endPeak << std::endl;

    float divisor = 0.0f;
    switch(m_format)
    {
        case 1:
            divisor = SAMPLE_MAX_8BIT;
            break;

        case 2:
            divisor = SAMPLE_MAX_16BIT;

        default:
            std::cerr << "PeakFile::getPreview - unsupported peak length format"
                      << std::endl;
            return ret;
    }

    for (int i = startPeak; i < endPeak; i++)
    {
        // Seek to first peak value
        //
        if (i == startPeak)
        {
            if (scanToPeak(startPeak) == false)
                return ret;
        }

        hiValue = 0.0f;
        loValue = 0.0f;

        // Get peak value
        for (int j = 0; j < m_channels; j++)
        {
            try
            {
                peakData = getBytes(m_format * m_pointsPerValue);
            }
            catch (std::string e)
            {
                // Problem with the get - probably an EOF
                // return the results so far.
                //
                std::cout << "PeakFile::getPreview - \"" << e << "\"\n"
                          << std::endl;
                return ret;
            }

            if (peakData.length() == m_format * m_pointsPerValue)
            {
                hiValue += getIntegerFromLittleEndian(
                               peakData.substr(0, m_format));

                if (m_pointsPerValue == 2)
                {
                    loValue += getIntegerFromLittleEndian(
                               peakData.substr(m_format, m_format));
                }
            }
            else
            {
                // We didn't get the whole peak block - return what
                // we've got so far
                //
                std::cerr << "PeakFile::getPreview - "
                          << "failed to get complete peak block"
                          << std::endl;
                return ret;
            }
        }

        hiValue /= (float)m_channels;
        loValue /= (float)m_channels;

        hiValue /= divisor;
        loValue /= divisor;

        /*
        cout << "HI = " << hiValue << endl;
        cout << "LO = " << loValue << endl;
        */

        ret.push_back(hiValue);
    }

    return ret;
}


};



