/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[WAVAudioFile]"

#include "WAVAudioFile.h"
#include "base/RealTime.h"

#include <sstream>

#include "misc/Debug.h"

//#define DEBUG_DECODE 1

namespace Rosegarden
{

WAVAudioFile::WAVAudioFile(const unsigned int &id,
                           const std::string &name,
                           const QString &absoluteFilePath):
        RIFFAudioFile(id, name, absoluteFilePath)
{
    m_type = WAV;
}

WAVAudioFile::WAVAudioFile(const QString &absoluteFilePath,
                           unsigned int channels = 1,
                           unsigned int sampleRate = 48000,
                           unsigned int bytesPerSecond = 6000,
                           unsigned int bytesPerFrame = 2,
                           unsigned int bitsPerSample = 16):
        RIFFAudioFile(absoluteFilePath, channels, sampleRate, bytesPerSecond, bytesPerFrame, bitsPerSample)
{
    m_type = WAV;
}

WAVAudioFile::~WAVAudioFile()
{}

bool
WAVAudioFile::open()
{
    // if already open
    if (m_inFile && (*m_inFile))
        return true;

    m_inFile = new std::ifstream(m_absoluteFilePath.toLocal8Bit(),
                                 std::ios::in | std::ios::binary);

    if (!(*m_inFile)) {
        m_type = UNKNOWN;
        return false;
    }

    // Get the file size and store it for comparison later
    m_fileSize = m_fileInfo->size();

    try {
        parseHeader();
    } catch (const BadSoundFileException &e) {
        RG_WARNING << "ERROR: WAVAudioFile::open(): parseHeader: " << e.getMessage();
        return false;
    }

    return true;
}

// Open the file for writing, write out the header and move
// to the data chunk to accept samples.  We fill in all the
// totals when we close().
//
bool
WAVAudioFile::write()
{
    // close if we're open
    if (m_outFile) {
        m_outFile->close();
        delete m_outFile;
    }

    // open for writing
    m_outFile = new std::ofstream(m_absoluteFilePath.toLocal8Bit(),
                                  std::ios::out | std::ios::binary);

    if (!(*m_outFile))
        return false;

    // write out format header chunk and prepare for sample writing
    //
    writeFormatChunk();

    return true;
}

void
WAVAudioFile::close()
{
    if (m_outFile == nullptr)
        return ;

    m_outFile->seekp(0, std::ios::end);
    unsigned int totalSize = m_outFile->tellp();

    // seek to first length position
    m_outFile->seekp(4, std::ios::beg);

    // write complete file size minus 8 bytes to here
    putBytes(m_outFile, getLittleEndianFromInteger(totalSize - 8, 4));

    // reseek from start forward 40
    m_outFile->seekp(40, std::ios::beg);

    // write the data chunk size to end
    putBytes(m_outFile, getLittleEndianFromInteger(totalSize - 44, 4));

    m_outFile->close();

    delete m_outFile;
    m_outFile = nullptr;
}

// Set the AudioFile meta data according to WAV file format specification.
//
void
WAVAudioFile::parseHeader()
{
    // Read the format chunk and populate the file data.  A plain WAV
    // file only has this chunk.  Exceptions tumble through.
    //
    readFormatChunk();

}

std::streampos
WAVAudioFile::getDataOffset()
{
    return 0;
}

bool
WAVAudioFile::decode(const unsigned char *sourceData,
                     size_t sourceBytes,
                     size_t targetSampleRate,
                     size_t targetChannels,
                     size_t targetFrames,
                     std::vector<float *> &targetData,
                     bool addToResultBuffers)
{
    size_t sourceChannels = getChannels();
    size_t sourceSampleRate = getSampleRate();
    size_t fileFrames = sourceBytes / getBytesPerFrame();

    int bitsPerSample = getBitsPerSample();
    if (bitsPerSample != 8 &&
            bitsPerSample != 16 &&
            bitsPerSample != 24 &&
            bitsPerSample != 32) { // 32-bit is IEEE-float (enforced in RIFFAudioFile)
        RG_WARNING << "WAVAudioFile::decode: unsupported " << bitsPerSample << "-bit sample size";
        return false;
    }

#ifdef DEBUG_DECODE
    RG_DEBUG << "WAVAudioFile::decode: " << sourceBytes << " bytes -> " << targetFrames << " frames, SSR " << getSampleRate() << ", TSR " << targetSampleRate << ", sch " << getChannels() << ", tch " << targetChannels;
#endif

    // If we're reading a stereo file onto a mono target, we mix the
    // two channels.  If we're reading mono to stereo, we duplicate
    // the mono channel.  Otherwise if the numbers of channels differ,
    // we just copy across the ones that do match and zero the rest.

    bool reduceToMono = (targetChannels == 1 && sourceChannels == 2);

    for (size_t ch = 0; ch < sourceChannels; ++ch) {

        if (!reduceToMono || ch == 0) {
            if (ch >= targetChannels)
                break;
            if (!addToResultBuffers)
                memset(targetData[ch], 0, targetFrames * sizeof(float));
        }

        int tch = ch; // target channel for this data
        if (reduceToMono && ch == 1) {
            tch = 0;
        }

        float ratio = 1.0;
        if (sourceSampleRate != targetSampleRate) {
            ratio = float(sourceSampleRate) / float(targetSampleRate);
        }

        for (size_t i = 0; i < targetFrames; ++i) {

            size_t j = i;
            if (sourceSampleRate != targetSampleRate) {
                j = size_t(i * ratio);
            }
            if (j >= fileFrames)
                j = fileFrames - 1;

	    float sample = convertBytesToSample
		(&sourceData[(bitsPerSample / 8) * (ch + j * sourceChannels)]);

            targetData[tch][i] += sample;
        }
    }

    // Now deal with any excess target channels

    for (size_t ch = sourceChannels; ch < targetChannels; ++ch) {
        if (ch == 1 && targetChannels == 2) {
            // copy mono to stereo
            if (!addToResultBuffers) {
                memcpy(targetData[ch],
                       targetData[ch - 1],
                       targetFrames * sizeof(float));
            } else {
                for (size_t i = 0; i < targetFrames; ++i) {
                    targetData[ch][i] += targetData[ch - 1][i];
                }
            }
        } else {
            if (!addToResultBuffers) {
                memset(targetData[ch], 0, targetFrames * sizeof(float));
            }
        }
    }

    return true;
}


}
