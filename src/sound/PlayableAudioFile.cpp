/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PlayableAudioFile.h"

#include "RingBufferPool.h"

#include <utility>

#include <pthread.h>


namespace Rosegarden
{


//#define DEBUG_PLAYABLE 1
//#define DEBUG_PLAYABLE_READ 1

AudioCache PlayableAudioFile::m_smallFileCache;

std::vector<PlayableAudioFile::sample_t *> PlayableAudioFile::m_workBuffers;
size_t PlayableAudioFile::m_workBufferSize = 0;
QMutex PlayableAudioFile::m_workBuffersMutex;

char *PlayableAudioFile::m_rawFileBuffer;
size_t PlayableAudioFile::m_rawFileBufferSize = 0;

RingBufferPool *PlayableAudioFile::m_ringBufferPool = nullptr;

static constexpr size_t a_xfadeFrames = 30;

PlayableAudioFile::PlayableAudioFile(InstrumentId instrumentId,
                                     AudioFile *audioFile,
                                     const RealTime &startTime,
                                     const RealTime &startIndex,
                                     const RealTime &duration,
                                     size_t bufferSize,
                                     size_t smallFileSize,
                                     int targetChannels,
                                     int targetSampleRate) :
    m_startTime(startTime),
    m_startIndex(startIndex),
    m_duration(duration),
    m_file(nullptr),
    m_audioFile(audioFile),
    m_instrumentId(instrumentId),
    m_targetChannels(targetChannels),
    m_targetSampleRate(targetSampleRate),
    m_fileEnded(false),
    m_firstRead(true),
    m_isSmallFile(false),
    m_smallFileScanFrame(0)
{
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::PlayableAudioFile - creating " << this << " for instrument " << instrumentId << " with file " << (m_audioFile ? m_audioFile->getShortFilename() : "(none)") << std::endl;
#endif

    if (!m_ringBufferPool) {
        // !!! Problematic -- how do we deal with different playable audio
        // files requiring different buffer sizes?  That shouldn't be the
        // usual case, but it's not unthinkable.
        m_ringBufferPool = new RingBufferPool(bufferSize);
    } else {
        m_ringBufferPool->setBufferSize
            (std::max(bufferSize, m_ringBufferPool->getBufferSize()));
    }

    initialise(bufferSize, smallFileSize);
}


void
PlayableAudioFile::setRingBufferPoolSizes(size_t n, size_t nframes)
{
    if (!m_ringBufferPool) {
        m_ringBufferPool = new RingBufferPool(nframes);
    } else {
        m_ringBufferPool->setBufferSize
            (std::max(nframes, m_ringBufferPool->getBufferSize()));
    }
    m_ringBufferPool->setPoolSize(n);
}


void
PlayableAudioFile::initialise(size_t bufferSize, size_t smallFileSize)
{
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::initialise() " << this << std::endl;
#endif

    checkSmallFileCache(smallFileSize);

    if (!m_isSmallFile) {

        m_file = new std::ifstream(m_audioFile->getAbsoluteFilePath().toLocal8Bit(),
                                   std::ios::in | std::ios::binary);

        if (!*m_file) {
            std::cerr << "ERROR: PlayableAudioFile::initialise: Failed to open audio file " << m_audioFile->getAbsoluteFilePath() << std::endl;
            delete m_file;
            m_file = nullptr;
        }
    }

    // Scan to the beginning of the data chunk we need
    //
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::initialise - scanning to " << m_startIndex << std::endl;
#endif

    if (m_file) {
        scanTo(m_startIndex);
    } else {
        m_fileEnded = false;
        m_currentScanPoint = m_startIndex;
        m_smallFileScanFrame = (size_t)RealTime::realTime2Frame
            (m_currentScanPoint, m_audioFile->getSampleRate());
    }

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::initialise: buffer size is " << bufferSize << " frames, file size is " << m_audioFile->getSize() << std::endl;
#else
    (void)bufferSize;
#endif

    if (m_targetChannels <= 0)
        m_targetChannels = m_audioFile->getChannels();
    if (m_targetSampleRate <= 0)
        m_targetSampleRate = m_audioFile->getSampleRate();

    m_ringBuffers = new RingBuffer<sample_t> *[m_targetChannels];
    for (int ch = 0; ch < m_targetChannels; ++ch) {
        m_ringBuffers[ch] = nullptr;
    }
}

PlayableAudioFile::~PlayableAudioFile()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }

    returnRingBuffers();
    delete[] m_ringBuffers;
    m_ringBuffers = nullptr;

    if (m_isSmallFile) {
        m_smallFileCache.decrementReference(m_audioFile);
    }

    {
        QMutexLocker lock(&m_workBuffersMutex);
        clearWorkBuffers();
    }

#ifdef DEBUG_PLAYABLE 
    //    std::cerr << "PlayableAudioFile::~PlayableAudioFile - destroying - " << this << std::endl;
#endif
}

void
PlayableAudioFile::returnRingBuffers()
{
    for (int i = 0; i < m_targetChannels; ++i) {
        if (m_ringBuffers[i]) {
            m_ringBufferPool->returnBuffer(m_ringBuffers[i]);
            m_ringBuffers[i] = nullptr;
        }
    }
}

bool
PlayableAudioFile::scanTo(const RealTime &time)
{
#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "PlayableAudioFile::scanTo(" << time << ")" << std::endl;
#endif

    m_fileEnded = false; // until we know otherwise -- this flag is an
    // optimisation, not a reliable record

    bool ok = false;

    if (m_isSmallFile) {

        m_currentScanPoint = time;
        m_smallFileScanFrame = (size_t)RealTime::realTime2Frame
            (time, m_audioFile->getSampleRate());
#ifdef DEBUG_PLAYABLE_READ
        std::cerr << "... maps to frame " << m_smallFileScanFrame << std::endl;
#endif
        ok = true;

    } else {

        ok = m_audioFile->scanTo(m_file, time);
        if (ok) {
            m_currentScanPoint = time;
        }
    }

#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "PlayableAudioFile::scanTo(" << time << "): set m_currentScanPoint to " << m_currentScanPoint << std::endl;
#endif

    m_firstRead = true; // so we know to xfade in

    return ok;
}


size_t
PlayableAudioFile::getSampleFramesAvailable()
{
    size_t actual = 0;

    if (m_isSmallFile) {
        size_t cchannels;
        size_t cframes;
        (void)m_smallFileCache.getData(m_audioFile, cchannels, cframes);
        if (cframes > m_smallFileScanFrame)
            return cframes - m_smallFileScanFrame;
        else
            return 0;
    }

    for (int ch = 0; ch < m_targetChannels; ++ch) {
        if (!m_ringBuffers[ch])
            return 0;
        size_t thisChannel = m_ringBuffers[ch]->getReadSpace();
        if (ch == 0 || thisChannel < actual)
            actual = thisChannel;
    }

#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile(" << (m_audioFile ? m_audioFile->getShortFilename() : "(none)") << " " << this << ")::getSampleFramesAvailable: have " << actual << std::endl;
#endif

    return actual;
}

size_t
PlayableAudioFile::addSamples(std::vector<sample_t *> &destination,
                              size_t channels, size_t nframes, size_t offset)
{
#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "PlayableAudioFile::addSamples(" << nframes << "): channels " << channels << ", my target channels " << m_targetChannels << std::endl;
#endif

    // Not a small file?  Use m_ringBuffers.
    if (!m_isSmallFile) {

        size_t qty = 0;
        bool done = m_fileEnded;

        for (int ch = 0; ch < int(channels) && ch < m_targetChannels; ++ch) {
            if (!m_ringBuffers[ch])
                return 0; // !!! fatal
            size_t here = m_ringBuffers[ch]->readAdding(destination[ch] + offset, nframes);
            if (ch == 0 || here < qty)
                qty = here;
            if (done && (m_ringBuffers[ch]->getReadSpace() > 0))
                done = false;
        }

        for (int ch = channels; ch < m_targetChannels; ++ch) {
            m_ringBuffers[ch]->skip(nframes);
        }

        if (done) {
#ifdef DEBUG_PLAYABLE_READ
            std::cerr << "PlayableAudioFile::addSamples(" << nframes << "): reached end, returning buffers" << std::endl;
#endif

            returnRingBuffers();
        }

#ifdef DEBUG_PLAYABLE_READ
        std::cerr << "PlayableAudioFile::addSamples(" << nframes << "): returning " << qty << " frames (at least " << (m_ringBuffers[0] ? m_ringBuffers[0]->getReadSpace() : 0) << " remaining)" << std::endl;
#endif

        return qty;

    } else {  // Small file.  Use m_smallFileCache.

        size_t cchannels;
        size_t cframes;
        float **cached = m_smallFileCache.getData(m_audioFile, cchannels, cframes);

        if (!cached) {
            std::cerr << "WARNING: PlayableAudioFile::addSamples: Failed to find small file in cache" << std::endl;
            m_isSmallFile = false;
        } else {

            size_t scanFrame = m_smallFileScanFrame;

            if (scanFrame >= cframes) {
                m_fileEnded = true;
                return 0;
            }

            size_t endFrame = scanFrame + nframes;
            size_t n = nframes;

            if (endFrame >= cframes) {
                m_fileEnded = true;
                n = cframes - scanFrame;
            }

#ifdef DEBUG_PLAYABLE_READ
            std::cerr << "PlayableAudioFile::addSamples: it's a small file: want frames " << scanFrame << " to " << endFrame << " of " << cframes << std::endl;
#endif

            // size_t xfadeIn = (m_firstRead ? m_xfadeFrames : 0);
            // size_t xfadeOut = (m_fileEnded ? m_xfadeFrames : 0);

            // all this could be neater!

            if (channels == 1 && cchannels == 2) { // mix
                for (size_t i = 0; i < n; ++i) {
                    sample_t v =
                        cached[0][scanFrame + i] +
                        cached[1][scanFrame + i];
                    //if ((i + 1) < xfadeIn)
                    //    v = (v * (i + 1)) / xfadeIn;
                    //if ((n - i) < xfadeOut)
                    //    v = (v * (n - i)) / xfadeOut;
                    destination[0][i + offset] += v;
                }
            } else {
                for (size_t ch = 0; ch < channels; ++ch) {
                    int sch = ch;
                    if (ch >= cchannels) {
                        if (channels == 2 && cchannels == 1)
                            sch = 0;
                        else
                            break;
                    } else {
                        for (size_t i = 0; i < n; ++i) {
                            sample_t v = cached[sch][scanFrame + i];
                            //if ((i + 1) < xfadeIn)
                            //    v = (v * (i + 1)) / xfadeIn;
                            //if ((n - i) < xfadeOut)
                            //    v = (v * (n - i)) / xfadeOut;
                            destination[ch][i + offset] += v;
                        }
                    }
                }
            }

            m_smallFileScanFrame += nframes;
            m_currentScanPoint = m_currentScanPoint +
                RealTime::frame2RealTime(nframes, m_targetSampleRate);
            return nframes;
        }
    }

    return 0;
}

void
PlayableAudioFile::checkSmallFileCache(size_t smallFileSize)
{
    if (m_smallFileCache.has(m_audioFile)) {

#ifdef DEBUG_PLAYABLE
        std::cerr << "PlayableAudioFile::checkSmallFileCache: Found file in small file cache" << std::endl;
#endif

        m_smallFileCache.incrementReference(m_audioFile);
        m_isSmallFile = true;

    } else if (m_audioFile->getSize() <= smallFileSize) {

        std::ifstream file(m_audioFile->getAbsoluteFilePath().toLocal8Bit(),
                           std::ios::in | std::ios::binary);

        if (!file) {
            std::cerr << "ERROR: PlayableAudioFile::checkSmallFileCache: Failed to open audio file " << m_audioFile->getAbsoluteFilePath() << std::endl;
            return ;
        }

#ifdef DEBUG_PLAYABLE
        std::cerr << "PlayableAudioFile::checkSmallFileCache: Adding file to small file cache" << std::endl;
#endif

        // We always encache files with their original number of
        // channels (because they might be called for in any channel
        // configuration subsequently) but with the current sample
        // rate, not their original one.

        m_audioFile->scanTo(&file, RealTime::zero());

        size_t reqd = m_audioFile->getSize() / m_audioFile->getBytesPerFrame();
        unsigned char *buffer = new unsigned char[m_audioFile->getSize()];
        size_t obtained = m_audioFile->getSampleFrames(&file, (char *)buffer, reqd);

//        std::cerr <<"obtained=" << obtained << std::endl;

        size_t nch = getSourceChannels();
        size_t nframes = obtained;
        if (int(getSourceSampleRate()) != m_targetSampleRate) {
#ifdef DEBUG_PLAYABLE
            std::cerr << "PlayableAudioFile::checkSmallFileCache: Resampling badly from " << getSourceSampleRate() << " to " << m_targetSampleRate << std::endl;
#endif
            nframes = size_t(float(nframes) * float(m_targetSampleRate) /
                             float(getSourceSampleRate()));
        }

        std::vector<sample_t *> samples;
        for (size_t ch = 0; ch < nch; ++ch) {
            samples.push_back(new sample_t[nframes]);
        }

        if (!m_audioFile->decode(buffer,
                                 obtained * m_audioFile->getBytesPerFrame(),
                                 m_targetSampleRate,
                                 nch,
                                 nframes,
                                 samples)) {
            std::cerr << "PlayableAudioFile::checkSmallFileCache: failed to decode file" << std::endl;
        } else {
            sample_t **toCache = new sample_t * [nch];
            for (size_t ch = 0; ch < nch; ++ch) {
                toCache[ch] = samples[ch];
            }
            m_smallFileCache.addData(m_audioFile, nch, nframes, toCache);
            m_isSmallFile = true;
        }

        delete[] buffer;

        file.close();
    }

    if (m_isSmallFile) {
        if (m_file) {
            m_file->close();
            delete m_file;
            m_file = nullptr;
        }
    }
}

#if 0
void
PlayableAudioFile::fillBuffers()
{
#ifdef DEBUG_PLAYABLE
    if (m_audioFile) {
        std::cerr << "PlayableAudioFile(" << m_audioFile->getShortFilename() << ")::fillBuffers() [async] -- scanning to " << m_startIndex << std::endl;
    } else {
        std::cerr << "PlayableAudioFile::fillBuffers() [async] -- scanning to " << m_startIndex << std::endl;
    }
#endif

    if (!m_isSmallFile && (!m_file || !*m_file)) {
        m_file = new std::ifstream(m_audioFile->getAbsoluteFilePath().toLocal8Bit(),
                                   std::ios::in | std::ios::binary);
        if (!*m_file) {
            std::cerr << "ERROR: PlayableAudioFile::fillBuffers: Failed to open audio file " << m_audioFile->getAbsoluteFilePath() << std::endl;
            delete m_file;
            m_file = nullptr;
            return ;
        }
    }

    scanTo(m_startIndex);
    updateBuffers();
}
#endif

void
PlayableAudioFile::clearBuffers()
{
    returnRingBuffers();
}

bool
PlayableAudioFile::fillBuffers(const RealTime &currentTime)
{
#ifdef DEBUG_PLAYABLE
    if (!m_isSmallFile) {
        if (m_audioFile) {
            std::cerr << "PlayableAudioFile(" << m_audioFile->getShortFilename() << " " << this << ")::fillBuffers(" << currentTime << "):\n my start time " << m_startTime << ", start index " << m_startIndex << ", duration " << m_duration << std::endl;
        } else {
            std::cerr << "PlayableAudioFile::fillBuffers(" << currentTime << "): my start time " << m_startTime << ", start index " << m_startIndex << ", duration " << m_duration << std::endl;
        }
    }
#endif

    if (currentTime > m_startTime + m_duration) {

#ifdef DEBUG_PLAYABLE
        std::cerr << "PlayableAudioFile::fillBuffers: seeking past end, returning buffers" << std::endl;
#endif

        returnRingBuffers();
        return true;
    }

    if (!m_isSmallFile && (!m_file || !*m_file)) {
        m_file = new std::ifstream(m_audioFile->getAbsoluteFilePath().toLocal8Bit(),
                                   std::ios::in | std::ios::binary);
        if (!*m_file) {
            std::cerr << "ERROR: PlayableAudioFile::fillBuffers: Failed to open audio file " << m_audioFile->getAbsoluteFilePath() << std::endl;
            delete m_file;
            m_file = nullptr;
            return false;
        }
        scanTo(m_startIndex);
    }

    RealTime scanTime = m_startIndex;

    if (currentTime > m_startTime) {
        scanTime = m_startIndex + currentTime - m_startTime;
    }

    //    size_t scanFrames = (size_t)RealTime::realTime2Frame
    //        (scanTime,
    //         m_isSmallFile ? m_targetSampleRate : m_audioFile->getSampleRate());

    if (scanTime != m_currentScanPoint) {
        scanTo(scanTime);
    }

    if (!m_isSmallFile) {
        for (int i = 0; i < m_targetChannels; ++i) {
            if (m_ringBuffers[i])
                m_ringBuffers[i]->reset();
        }
        updateBuffers();
    }

    return true;
}

bool
PlayableAudioFile::updateBuffers()
{
    if (m_isSmallFile)
        return false;
    if (!m_file)
        return false;

    if (m_fileEnded) {
#ifdef DEBUG_PLAYABLE_READ
        std::cerr << "PlayableAudioFile::updateBuffers: at end of file already" << std::endl;
#endif

        return false;
    }

    if (!m_ringBuffers[0]) {

        if (m_targetChannels < 0) {
            std::cerr << "WARNING: PlayableAudioFile::updateBuffers: m_targetChannels < 0, can't allocate ring buffers" << std::endl;
            return false;
        }

        // need a buffer: can we get one?
        if (!m_ringBufferPool->getBuffers(m_targetChannels, m_ringBuffers)) {
            std::cerr << "WARNING: PlayableAudioFile::updateBuffers: no ring buffers available" << std::endl;
            return false;
        }
    }

    size_t nframes = 0;

    for (int ch = 0; ch < m_targetChannels; ++ch) {
        if (!m_ringBuffers[ch])
            continue;
        size_t writeSpace = m_ringBuffers[ch]->getWriteSpace();
        if (ch == 0 || writeSpace < nframes)
            nframes = writeSpace;
    }

    if (nframes == 0) {
#ifdef DEBUG_PLAYABLE_READ
        std::cerr << "PlayableAudioFile::updateBuffers: frames == 0, ignoring" << std::endl;
#endif

        return false;
    }

#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "PlayableAudioFile::updateBuffers: want " << nframes << " frames" << std::endl;
#endif


    RealTime block = RealTime::frame2RealTime(nframes, m_targetSampleRate);
    if (m_currentScanPoint + block >= m_startIndex + m_duration) {
        block = m_startIndex + m_duration - m_currentScanPoint;
        if (block <= RealTime::zero())
            nframes = 0;
        else
            nframes = (size_t)RealTime::realTime2Frame(block, m_targetSampleRate);
        m_fileEnded = true;
    }

    size_t fileFrames = nframes;
    if (m_targetSampleRate != int(getSourceSampleRate())) {
        fileFrames = size_t(float(fileFrames) * float(getSourceSampleRate()) /
                            float(m_targetSampleRate));
    }

#ifdef DEBUG_PLAYABLE_READ
    std::cerr << "Want " << fileFrames << " (" << block << ") from file (" << (m_duration + m_startIndex - m_currentScanPoint - block) << " to go)" << std::endl;
#endif

    // !!! need to be doing this in initialise, want to avoid allocations here
    if ((getBytesPerFrame() * fileFrames) > m_rawFileBufferSize) {
        delete[] m_rawFileBuffer;
        m_rawFileBufferSize = getBytesPerFrame() * fileFrames;
#ifdef DEBUG_PLAYABLE_READ

        std::cerr << "Expanding raw file buffer to " << m_rawFileBufferSize << " chars" << std::endl;
#endif

        m_rawFileBuffer = new char[m_rawFileBufferSize];
    }

    size_t obtained =
        m_audioFile->getSampleFrames(m_file, m_rawFileBuffer, fileFrames);

    if (obtained < fileFrames || m_file->eof()) {
        m_fileEnded = true;
    }

    {
        // We're about to hit the work buffers.
        QMutexLocker lock(&m_workBuffersMutex);

#ifdef DEBUG_PLAYABLE
        std::cerr << "requested " << fileFrames << " frames from file for " << nframes << " frames, got " << obtained << " frames" << std::endl;
#endif

        if (nframes > m_workBufferSize) {

            clearWorkBuffers();

            m_workBufferSize = nframes;

#ifdef DEBUG_PLAYABLE_READ
            std::cerr << "Expanding work buffer to " << m_workBufferSize << " frames" << std::endl;
#endif

            for (int i = 0; i < m_targetChannels; ++i) {
                m_workBuffers.push_back(new sample_t[m_workBufferSize]);
            }

        } else {

            while (m_targetChannels > (int)m_workBuffers.size()) {
                m_workBuffers.push_back(new sample_t[m_workBufferSize]);
            }
        }

        if (m_audioFile->decode((const unsigned char *)m_rawFileBuffer,
                                obtained * getBytesPerFrame(),
                                m_targetSampleRate,
                                m_targetChannels,
                                nframes,
                                m_workBuffers,
                                false)) {

            /* !!! No -- GUI and notification side of things isn't up to this yet,
              so comment it out just in case

            if (m_autoFade) {

                if (m_currentScanPoint < m_startIndex + m_fadeInTime) {

                    size_t fadeSamples =
                            (size_t)RealTime::realTime2Frame(m_fadeInTime, getTargetSampleRate());
                    size_t originSamples =
                            (size_t)RealTime::realTime2Frame(m_currentScanPoint - m_startIndex, // is x - y strictly non-negative?
                                                             getTargetSampleRate());

                    for (size_t i = 0; i < nframes; ++i) {
                        if (i + originSamples > fadeSamples) {
                            break;
                        }
                        float gain = float(i + originSamples) / float(fadeSamples);
                        for (int ch = 0; ch < m_targetChannels; ++ch) {
                            m_workBuffers[ch][i] *= gain;
                        }
                    }
                }

                if (m_currentScanPoint + block >
                    m_startIndex + m_duration - m_fadeOutTime) {

                    size_t fadeSamples =
                            (size_t)RealTime::realTime2Frame(m_fadeOutTime, getTargetSampleRate());
                    size_t originSamples = // counting from end
                            (size_t)RealTime::realTime2Frame
                                    (m_startIndex + m_duration - m_currentScanPoint, // is x - y strictly non-negative?
                                     getTargetSampleRate());

                    for (size_t i = 0; i < nframes; ++i) {
                        float gain = 1.0;
                        if (originSamples < i) gain = 0.0;
                        else {
                            size_t fromEnd = originSamples - i;
                            if (fromEnd < fadeSamples) {
                                gain = float(fromEnd) / float(fadeSamples);
                            }
                        }
                        for (int ch = 0; ch < m_targetChannels; ++ch) {
                            m_workBuffers[ch][i] *= gain;
                        }
                    }
                }
            }
            */

            m_currentScanPoint = m_currentScanPoint + block;

            for (int ch = 0; ch < m_targetChannels; ++ch) {

                if (m_firstRead || m_fileEnded) {
                    float xfade = std::min(a_xfadeFrames, nframes);
                    if (m_firstRead) {
                        for (size_t i = 0; i < xfade; ++i) {
                            m_workBuffers[ch][i] *= float(i + 1) / xfade;
                        }
                    }
                    if (m_fileEnded) {
                        for (size_t i = 0; i < xfade; ++i) {
                            m_workBuffers[ch][nframes - i - 1] *=
                                float(i + 1) / xfade;
                        }
                    }
                }

                if (m_ringBuffers[ch]) {
                    m_ringBuffers[ch]->write(m_workBuffers[ch], nframes);
                }
            }
        }

        // Done.
        //m_workBuffersMutex.unlock();
    }

    m_firstRead = false;

    if (obtained < fileFrames) {
        if (m_file) {
            m_file->close();
            delete m_file;
            m_file = nullptr;
        }
    }

    return true;
}


// How many channels in the base AudioFile?
//
unsigned int
PlayableAudioFile::getSourceChannels()
{
    if (m_audioFile) {
        return m_audioFile->getChannels();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getTargetChannels()
{
    return m_targetChannels;
}

unsigned int
PlayableAudioFile::getBytesPerFrame()
{
    if (m_audioFile) {
        return m_audioFile->getBytesPerFrame();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getSourceSampleRate()
{
    if (m_audioFile) {
        return m_audioFile->getSampleRate();
    }
    return 0;
}

#if 0
unsigned int
PlayableAudioFile::getTargetSampleRate()
{
    return m_targetSampleRate;
}
#endif

#if 0
// How many bits per sample in the base AudioFile?
//
unsigned int
PlayableAudioFile::getBitsPerSample()
{
    if (m_audioFile) {
        return m_audioFile->getBitsPerSample();
    }
    return 0;
}
#endif

void
PlayableAudioFile::clearWorkBuffers()
{
    for (size_t i = 0; i < m_workBuffers.size(); ++i) {
        delete[] m_workBuffers[i];
    }
    m_workBuffers.clear();
}


}
