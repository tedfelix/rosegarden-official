/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2020 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#pragma once

#include "base/Instrument.h"  // For InstrumentId
#include "base/MidiProgram.h"  // For MidiByte
#include "base/RealTime.h"

#include <set>

namespace Rosegarden
{


/// Pending Note Off event for the NoteOffQueue
struct NoteOffEvent
{
    NoteOffEvent() :
        realTime(),
        pitch(0),
        channel(0),
        instrumentId(0)
    { }

    NoteOffEvent(const RealTime &realTime,
                 MidiByte pitch,
                 MidiByte channel,
                 InstrumentId instrumentId) :
        realTime(realTime),
        pitch(pitch),
        channel(channel),
        instrumentId(instrumentId)
    { }

    RealTime realTime;
    MidiByte pitch;
    MidiByte channel;
    InstrumentId instrumentId;
};

struct NoteOffEventCmp
{
    bool operator()(const NoteOffEvent *lhs, const NoteOffEvent *rhs)
    {
        return (lhs->realTime < rhs->realTime);
    }
};

typedef std::multiset<NoteOffEvent *, NoteOffEventCmp> NoteOffQueue;


}
