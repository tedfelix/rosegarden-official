// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
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

#include "SoundDriver.h"

// An empty sound driver for when we don't want sound support
// but still want to build the sequencer.
//

#ifndef _DUMMYDRIVER_H_
#define _DUMMYDRIVER_H_

namespace Rosegarden
{

class DummyDriver : public SoundDriver
{
public:
    DummyDriver(MappedStudio *studio):
        SoundDriver(studio, std::string("DummyDriver - no sound")) {;}
    DummyDriver(MappedStudio *studio, const std::string & name):
        SoundDriver(studio, std::string("DummyDriver: " + name)) {;}
    virtual ~DummyDriver() {;}

    virtual void initialiseMidi()  { m_recordComposition.clear();}
    virtual void initialiseAudio()  {;}
    virtual void initialisePlayback(const RealTime & /*position*/) {;}
    virtual void stopPlayback() {;}
    virtual void resetPlayback(const RealTime & /*position*/,
                               const RealTime & /*latency*/) {;}
    virtual void allNotesOff()  {;}
    virtual void processNotesOff(const RealTime & /*time*/) {;}
    
    virtual RealTime getSequencerTime() { return RealTime(0, 0);}

    virtual MappedComposition*
        getMappedComposition(const RealTime & /*playLatency*/)
        { return &m_recordComposition;}

    virtual void processEventsOut(const MappedComposition & /*mC*/,
                                  const Rosegarden::RealTime & /*playLatency*/,
                                  bool /*now*/) {;}

    // Activate a recording state
    //
    virtual void record(const RecordStatus& /*recordStatus*/ ) {;}

    // Process anything that's pending
    //
    virtual void processPending(const RealTime & /*playLatency*/ ) {;}

    // Return the last recorded audio level
    //
    virtual float getLastRecordedAudioLevel() { return 0.0; }

    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId /*id*/,
                                   unsigned long /*pluginId*/,
                                   int /*position*/) {;}

    virtual void removePluginInstance(InstrumentId /*id*/,
                                      int /*position*/) {;}

    virtual void setPluginInstancePortValue(InstrumentId /*id*/,
                                            int /*position*/,
                                            unsigned long /*portNumber*/,
                                            float /*value*/) {;}

    virtual void setPluginInstanceBypass(InstrumentId /*id*/,
                                         int /*position*/,
                                         bool /*value*/) {;}

    virtual bool checkForNewClients() { return false; }

protected:
    virtual void processMidiOut(const MappedComposition & /*mC*/,
                                const RealTime & /*playLatency*/,
                                bool /*now*/) {;}
    virtual void processAudioQueue(const RealTime & /*playLatency*/,
                                   bool /*now*/) {;}
    virtual void generateInstruments()  {;}

};

}

#endif // _DUMMYDRIVER_H_

