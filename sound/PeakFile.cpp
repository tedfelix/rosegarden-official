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

#include "PeakFile.h"
#include "AudioFile.h"
#include "Audio.h"

namespace Rosegarden
{

PeakFile::PeakFile(AudioFile *audioFile):
    SoundFile(audioFile->getPeakFilename()),
    m_audioFile(audioFile),
    m_version(-1),                         // -1 defines new file - start at 0
    m_format(1),                           // default is 8-bit peak format
    m_blockSize(256),                      // default block size is 256 samples
    m_numberOfPeaks(0),
    m_positionPeakOfPeaks(0),
    m_offsetToPeaks(0),
    m_year(0),
    m_month(0),
    m_day(0),
    m_hours(0),
    m_minutes(0),
    m_seconds(0),
    m_milliseconds(0),
    m_chunkStartPosition(0)
{
}

PeakFile::~PeakFile()
{
}

bool
PeakFile::open()
{
    m_inFile = new std::ifstream(m_fileName.c_str(),
                                 std::ios::in | std::ios::binary);
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

    std::string levelIdentifier = getBytes(4);

#if (__GNUC__ < 3)
    if (levelIdentifier.compare(Rosegarden::AUDIO_BWF_PEAK_ID, 0, 4) != 0)
#else
    if (levelIdentifier.compare(0, 4, Rosegarden::AUDIO_BWF_PEAK_ID) != 0)
#endif
    {
        throw(std::string("PeakFile::parseHeader - can't find LEVL identifier"));
    }

    // Get the length of the header minus the first 8 bytes
    //
    int length = getIntegerFromLittleEndian(getBytes(4));

    if (length == 0)
        throw(std::string("PeakFile::parseHeader - can't get header length"));

    std::string headerStr = getBytes(length);

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
    if (m_outFile == 0)
        return;

    // Seek to start of chunk
    //
    m_outFile->seekp(m_chunkStartPosition, std::ios::beg); 

    // Seek to number of peak frames and write value
    //
    m_outFile->seekp(28, std::ios::cur);
    putBytes(m_outFile,
             getLittleEndianFromInteger(m_numberOfPeaks, 4));

    // Peak of peaks 
    //
    putBytes(m_outFile,
             getLittleEndianFromInteger(m_positionPeakOfPeaks, 4));

    // Seek to date field
    //
    m_outFile->seekp(4, std::ios::cur);

    // Get the date and format it
    //
    QDate *date = new QDate();
    QTime *time = new QTime();
    QString fDate;
    fDate.sprintf("%04d:%02d:%02d:%02d:%02d:%02d:%03d",
                    date->currentDate().year(),
                    date->currentDate().month(),
                    date->currentDate().day(),
                    time->currentTime().hour(),
                    time->currentTime().minute(),
                    time->currentTime().second(),
                    time->currentTime().msec());

    std::string dateString(fDate.data());

    // Pad with spaces to make up to 28 bytes long and output
    //
    dateString += "     ";
    putBytes(m_outFile, dateString);

    delete date;
    delete time;

    // Ok, now close and tidy up
    //
    m_outFile->close();
    delete m_outFile;
    m_outFile = 0;
}

bool
PeakFile::isValid()
{
    return false;
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
    header += getLittleEndianFromInteger(2, 4); // hardcode to 2 for the mo

    // Block size - default and recommended is 256 
    //
    header += getLittleEndianFromInteger(m_blockSize, 4);

    // Peak channels - same as AudioFile channels
    //
    header += getLittleEndianFromInteger(m_audioFile->getChannels(), 4);

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

    for (int i = 0; i < m_audioFile->getChannels(); i++)
        channelPeaks.push_back(std::pair<int, int>());

    // Set the format level now we've written the peak data
    //
    int bytes = m_audioFile->getBitsPerSample() / 8;

    // Store peaks of peaks position
    //
    int sampleValue;
    int sampleMax = 0 ;
    int sampleMaxPosition = 0;
    int sampleFrameCount = 0;

    // Set total number of written peaks to zero
    //
    m_numberOfPeaks = 0;

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
                    sampleMaxPosition = sampleFrameCount;
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
        }

        // increment number of peak frames
        m_numberOfPeaks++;
    }

    // set format number
    m_format = bytes;

    std::cout << "PeakFile::writePeaks - "
              << "completed peaks" << std::endl;

}


};



