/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _CHANNELMANAGER_H_
#define _CHANNELMANAGER_H_

#include "base/ChannelInterval.h"
#include "base/Instrument.h"

#include <QObject>

namespace Rosegarden
{
class AllocateChannels;
class Instrument;
class MappedEvent;
class MappedInserterBase;
class Segment;
class RosegardenDocument;

// @class ControllerAndPBList Set of controllers and pitchbends
// @author Tom Breton (Tehom)
struct ControllerAndPBList
{
    ControllerAndPBList(void) {}
    ControllerAndPBList(StaticControllers &controllers)
    : m_controllers(controllers),
        m_havePitchbend(false) {}
    StaticControllers m_controllers;
    bool              m_havePitchbend;
    int               m_pitchbend;
};

// @class ChannelManager Channel manager for use by a mapper.  Base
// class for the specialized channel managers.
// @author Tom Breton (Tehom)
class ChannelManager : public QObject
{
    Q_OBJECT

  friend class InternalSegmentMapper;
  friend class MetronomeMapper;
  friend class ImmediateNote;

 public:
// @class MapperFunctionality Base class to provide covariance with
// MIDI-type mappers.  InternalSegmentMapper and MetronomeMapper
// subclass this, add a pointer member to themselves, and pass that as
// a callback argument.
// @author Tom Breton (Tehom)
class MapperFunctionality
{
 public:
    virtual ControllerAndPBList
        getControllers(Instrument *instrument, RealTime start)=0;
};

class MapperFunctionalitySimple : public MapperFunctionality
{
 public:
    virtual ControllerAndPBList
        getControllers(Instrument *instrument, RealTime start);
};

 protected:
  ChannelManager(Instrument *instrument);

 private:
  // To block a default copy ctor, because this would be tricky to
  // copy correctly.
 ChannelManager(ChannelManager&) : QObject() {}

 public:
  ~ChannelManager(void)
    { freeChannelInterval(); }

  void connectInstrument(Instrument *instrument);

  static void
      sendProgramForInstrument(ChannelId channel, Instrument *instrument,
                               MappedInserterBase &inserter,
                               RealTime insertTime,
                               int trackId);
  static void
      setControllers(ChannelId channel, Instrument *instrument,
                     MappedInserterBase &inserter,
                     RealTime reftime, RealTime insertTime,
                     MapperFunctionality *functionality, int trackId);

protected slots:
  // Something is kicking everything off channel in our device.
  void slotVacateChannel(ChannelId channel);
  // Our instrument and its entire device are being destroyed.  This
  // exists so we can take a shortcut.
  void slotLosingDevice(void);
  // Our instrument is being destroyed.  We may or may not have
  // received slotLosingDevice first.
  void slotLosingInstrument(void);

  // Our instrument now has different settings so we must reinit the
  // channel. 
  void slotInstrumentChanged(void);

  // Our instrument now has/lacks a fixed channel.
  void slotChannelBecomesFixed(void);
  void slotChannelBecomesUnfixed(void);

  public:
  // Free the channel interval it owned.
  void freeChannelInterval(void);

  // Insert event via inserter, pre-inserting appropriate setup.
  void doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                RealTime reftime,
                MapperFunctionality *functionality,
                bool firstOutput, int trackId);

  void setInstrument(Instrument *instrument);

  void setDirty(void) { m_inittedForOutput = false; }

  // Set an interval that this ChannelManager must cover.  This does
  // not do allocation.
  void setRequiredInterval(RealTime start, RealTime end,
                           RealTime startMargin, RealTime endMargin)
  {
      m_start       = start;
      m_end         = end;
      m_startMargin = startMargin;
      m_endMargin   = endMargin;
  }

  // Allocate a sufficient channel interval if possible.  It is safe
  // to call this more than once, ie even if we already have a channel
  // interval.
  void reallocate(bool changedInstrument);

  void debugPrintStatus(void);

 protected:

  /*** Internal functions ***/

  /* Functions about allocating. */

  AllocateChannels *getAllocator(void);
  void  setChannelIdDirectly(void);

  // (Dis)connect signals to allocator.  We disconnect just when we
  // don't have a valid channel given by the allocator.  Note that
  // this doesn't neccessarily correspond to m_usingAllocator's state.
  void connectAllocator(void);
  void disconnectAllocator(void);

  void setAllocationMode(Instrument *instrument);

  /* Functions about setting up the channel */
  
  void setInitted(bool initted)
  { m_inittedForOutput = initted; }
  bool needsInit(void) { return !m_inittedForOutput; }

  void insertChannelSetup(MappedInserterBase &inserter,
                          RealTime reftime, RealTime insertTime,
                          MapperFunctionality *functionality, int trackId);
  /* Data members */

  // The channel interval that is allocated for this segment.
  ChannelInterval m_channel;

  // Whether we are to get a channel interval thru Device's allocator.
  // The alternative is to get one as a fixed channel.  Can be true
  // even when we don't currently have a valid a channel.
  bool            m_usingAllocator;

  // The times required for start and end.  m_channel may be larger
  // but never smaller.
  RealTime        m_start, m_end;

  // Margins required if instrument has changed.
  RealTime        m_startMargin, m_endMargin;

  // The instrument this plays on.  I don't own this.
  Instrument *m_instrument;

  // Whether the output channel has been set up for m_channel.  Here
  // we only deal with having the right channel.  doInsert's firstOutput
  // argument tells us if we need setup for some other reason such as jumping
  // in time.
  bool m_inittedForOutput;
  // Whether we have tried to allocate a channel interval, not
  // neccessarily successfully.  This allows some flexibility without
  // making us search again every time we insert a note.
  bool m_triedToGetChannel;
};

// @class EternalChannelManager Channel manager of an channel
// that encompasses the entire playing time.
// @author Tom Breton (Tehom)
class EternalChannelManager : public ChannelManager
{
 public:
 EternalChannelManager(Instrument *instrument) :
    ChannelManager(instrument)
    {
        setRequiredInterval(ChannelInterval::m_earliestTime,
                            ChannelInterval::m_latestTime,
                            RealTime::zeroTime,
                            RealTime::zeroTime);
    }

    // Reallocate its channel
    void reallocateEternalChannel(void) { reallocate(false); }
};

// @class IntervalChannelManager Channel manager of an channel
// interval. 
// @author Tom Breton (Tehom)
class IntervalChannelManager : public ChannelManager
{
 public:
 IntervalChannelManager(Instrument *instrument) :
    ChannelManager(instrument) {}

    // Reallocate its channel interval.
    void reallocateChannel(RealTime start, RealTime end) {
        setRequiredInterval(start, end, RealTime::zeroTime, RealTime(1,0));
        reallocate(false);
    }
};

}

#endif /* ifndef _CHANNELMANAGER_H_ */
