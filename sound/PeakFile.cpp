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

#include <math.h>

#include <qdatetime.h>
#include <qstringlist.h>
#include <qpainter.h>
#include <qpalette.h>
#include <kapp.h>

#include "PeakFile.h"
#include "AudioFile.h"
#include "Audio.h"
#include "Sound.h"
#include "Progress.h"

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
    return write(0, 5); // default update every 5%
}

bool
PeakFile::write(Progress *progress, unsigned short updatePercentage)
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
    writePeaks(progress, updatePercentage, m_outFile);

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

    /*
    if (m_inFile->eof())
        return false;
        */

    return true;
}


void
PeakFile::writePeaks(Progress *progress,
                     unsigned short /*updatePercentage*/,
                     std::ofstream *file)
{
    if(!file || !(*file)) return;

    cout << "PeakFile::writePeaks - calculating peaks" << endl;

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

    // Get approx file size for progress dialog
    //
    unsigned int apprxTotalBytes = m_audioFile->getSize();
    unsigned int byteCount = 0;

    // Store peaks of peaks position
    //
    int sampleValue;
    int sampleMax = 0 ;
    int sampleFrameCount = 0;

    int showCount = 0;

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
            cerr << "PeakFile::writePeaks - \"" << e << "\"" 
                      << endl
                      << "PeakFile::writePeaks - leaving last block" << endl;
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

        // Set the progress dialog
        //
        if (progress)
        {
            progress->setCompleted((int)(double(byteCount)/
                                   double(apprxTotalBytes) * 100.0));
        }

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

                // first time through set to both
                if (i == 0 && j == 0)
                {
                    channelPeaks[j].first = sampleValue;
                    channelPeaks[j].second = sampleValue;
                }
                else
                {
                    /*
                    if (showCount++ < 10)
                        cout << "SAMPLES = " << sampleValue << endl;
                    */

                    // Store maximum for this block
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
        /*
        if (showCount++ == 10)
          cout << endl;
          */

        // Write absolute peak data in channel order
        //
        for (unsigned int i = 0; i < m_audioFile->getChannels(); i++)
        {
            //cout << "PeakFile::writePeaks - "
                      //<< "writing peak data out" << endl;

            //cout << "WRITING HI VALUE = " << channelPeaks[i].first << endl;

            putBytes(file, getLittleEndianFromInteger(channelPeaks[i].first,
                                                      bytes));

            //cout << "WRITING LO VALUE = " << channelPeaks[i].second << endl;

            putBytes(file, getLittleEndianFromInteger(channelPeaks[i].second,
                                                      bytes));
            // count up total body bytes
            //
            m_bodyBytes += bytes * 2;
        }

        // increment number of peak frames
        m_numberOfPeaks++;

        // Keep the gui updated
        //
        if (progress)
            progress->processEvents();
    }

    // set format number
    m_format = bytes;

    cout << "PeakFile::writePeaks - "
              << "completed peaks" << endl;

}

// Get a normalised vector for the preview at a given horizontal
// resolution
//
std::vector<float>
PeakFile::getPreview(const RealTime &startTime,
                     const RealTime &endTime,
                     int width)
{
    std::vector<float> ret;

    double startPeak = getPeak(startTime);
    double endPeak = getPeak(endTime);
    double sampleEndPeak = getPeak(m_audioFile->getLength() - startTime);

    // Sanity check
    if (startPeak > endPeak)
        return ret;

    // Actual possible sample length in RealTime
    //
    double step = double(sampleEndPeak - startPeak) / double(width);
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
        peakNumber = int(startPeak + (i * step));

        // Seek to value
        //
        if (scanToPeak(peakNumber) == false)
            ret.push_back(0.0f);

        hiValue = 0.0f;
        loValue = 0.0f;

        // Get peak value over channels
        //
        for (int j = 0; j < m_channels; j++)
        {
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

                if (inValue > intDivisor)
                    inValue = intDivisor - inValue;

                hiValue += float(inValue);

                if (m_pointsPerValue == 2)
                {
                    inValue = 
                        getIntegerFromLittleEndian(
                                peakData.substr(m_format, m_format));

                    if (inValue > intDivisor)
                        inValue = intDivisor - inValue;

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
        }

        /*
        cout << "READING HI VALUE = " << hiValue/float(m_channels) << endl;
        cout << "READING LO VALUE = " << loValue/float(m_channels) << endl;
        */

        hiValue /= (divisor * (float)m_channels);
        loValue /= (divisor * (float)m_channels);


        /*
        float db = 10 * log10(fabs(hiValue));
        float val = (db + 45.0) / 45.0;

        if (val < 0.0) val = 0.0;
        if (val > 1.0) val = 1.0;

        ret.push_back(val * 0.5 + 0.5);
        */
        ret.push_back(hiValue);
    }

    return ret;
}

void
PeakFile::drawPixmap(const RealTime &startTime,
                     const RealTime &endTime,
                     QPixmap *pixmap)
{
    std::vector<float> ret = getPreview(startTime,
                                        endTime, 
                                        pixmap->width());
    // Setup pixmap and painter
    //
    QPainter painter(pixmap);
    pixmap->fill(kapp->palette().color(QPalette::Active,
                                       QColorGroup::Base));
    painter.setPen(Qt::black);

    float yStep = pixmap->height() / 2;

    for (int i = 0; i < pixmap->width(); i++)
    {
        // Draw the line
        //
        painter.drawLine(i, yStep + ret[i] * yStep,
                         i, yStep - ret[i] * yStep);

    }

    return;

}

double
PeakFile::getPeak(const RealTime &time)
{
    double frames = ((time.sec * 1000000.0) + time.usec) *
                        m_audioFile->getSampleRate() / 1000000.0;
    return (frames / double(m_blockSize));
}


}
