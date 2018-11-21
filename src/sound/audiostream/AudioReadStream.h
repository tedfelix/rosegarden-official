/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_AUDIO_READ_STREAM_H
#define RG_AUDIO_READ_STREAM_H

#include "base/ThingFactory.h"
#include "sound/RingBuffer.h"

namespace Rosegarden {

class Resampler;

/* Not thread-safe -- one per thread please. */

class AudioReadStream
{
public:
    class FileDRMProtected : virtual public std::exception
    {
    public:
        FileDRMProtected(QString file) throw();
        ~FileDRMProtected() throw() override { }
        const char *what() const throw() override;

    protected:
        QString m_file;
    };

    virtual ~AudioReadStream();

    bool isOK() const { return (m_channelCount > 0); }

    virtual QString getError() const { return ""; }

    size_t getChannelCount() const { return m_channelCount; }
    size_t getSampleRate() const { return m_sampleRate; }
    
    void setRetrievalSampleRate(size_t);

    /**
     * Retrieve \count frames of audio data (that is, \count *
     * getChannelCount() samples) from the source and store in
     * \frames.  Return the number of samples actually retrieved; this
     * will differ from \count only when the end of stream is reached.
     *
     * If a retrieval sample rate has been set, the audio will be
     * resampled to that rate (and \count refers to the number of
     * frames at the retrieval rate rather than the file's original
     * rate).
     */
    size_t getInterleavedFrames(size_t count, float *frames);
    
protected:
    AudioReadStream();
    virtual size_t getFrames(size_t count, float *frames) = 0;
    size_t m_channelCount;
    size_t m_sampleRate;
    size_t m_retrievalRate;
    Resampler *m_resampler;
    RingBuffer<float> *m_resampleBuffer;
};

template <typename T>
class AudioReadStreamBuilder :
    public ConcreteThingBuilder<T, AudioReadStream, QString>
{
public:
    AudioReadStreamBuilder(QUrl uri, QStringList extensions) :
        ConcreteThingBuilder<T, AudioReadStream, QString>(uri, extensions) {
    }
};

}

#endif
