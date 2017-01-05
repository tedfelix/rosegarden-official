/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_AUDIO_FILE_WRITE_STREAM_H
#define RG_AUDIO_FILE_WRITE_STREAM_H

#include "base/ThingFactory.h"

#include <QFile>
#include <QString>

namespace Rosegarden {

/* Not thread-safe -- one per thread please. */

class AudioWriteStream
{
public:
    class Target {
    public:
        Target(QString path, size_t channelCount, size_t sampleRate) :
            m_path(path), m_channelCount(channelCount), m_sampleRate(sampleRate)
        { }

        QString getPath() const { return m_path; }
        size_t getChannelCount() const { return m_channelCount; }
        size_t getSampleRate() const { return m_sampleRate; }

        void invalidate() { m_channelCount = 0; }

    private:
        QString m_path;
        size_t m_channelCount;
        size_t m_sampleRate;
    };

    virtual ~AudioWriteStream() { }

    bool isOK() const { return (m_target.getChannelCount() > 0); }

    virtual QString getError() const { return ""; }

    QString getPath() const { return m_target.getPath(); }
    size_t getChannelCount() const { return m_target.getChannelCount(); }
    size_t getSampleRate() const { return m_target.getSampleRate(); }
    
    virtual bool putInterleavedFrames(size_t count, float *frames) = 0;

    void remove()
    {
        QFile file(m_target.getPath());
        file.remove();
    }

protected:
    AudioWriteStream(Target t) : m_target(t) { }
    Target m_target;
};

template <typename T>
class AudioWriteStreamBuilder :
public ConcreteThingBuilder<T, AudioWriteStream, AudioWriteStream::Target>
{
public:
    AudioWriteStreamBuilder(QUrl uri, QStringList extensions) :
        ConcreteThingBuilder<T, AudioWriteStream, AudioWriteStream::Target>
        (uri, extensions) {
    }
};

}

#endif
