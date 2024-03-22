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

#ifndef PLAYABLEDATA_H
#define PLAYABLEDATA_H

#include "base/RealTime.h"
#include "base/Instrument.h"

namespace Rosegarden
{

class AudioFile;

class PlayableData
{
 public:
    typedef float sample_t;

    virtual ~PlayableData() { }

    virtual size_t getSampleFramesAvailable() = 0;

    virtual bool isFullyBuffered() const = 0;

    virtual bool isBuffered() const = 0;

    virtual RealTime getStartTime() const = 0;

    virtual RealTime getEndTime() const = 0;

    virtual RealTime getDuration() const = 0;

    virtual size_t addSamples
        (std::vector<sample_t *> &target,
         size_t channels,
         size_t nframes,
         size_t offset = 0) = 0;

    virtual void clearBuffers() = 0;

    virtual bool fillBuffers(const RealTime &currentTime) = 0;

    virtual bool updateBuffers() = 0;

    virtual InstrumentId getInstrument() const = 0;

    virtual AudioFile* getAudioFile() const = 0;

    virtual void cancel() = 0;

    virtual int getRuntimeSegmentId() const = 0;

    virtual bool isSmallFile() const = 0;

    virtual unsigned int getTargetChannels() const = 0;

    virtual void deactivate() { } /* default implementation do nothing */
};

}

#endif
