// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

#ifndef _ROSEGARDENSEQUENCERIFACE_H_
#define _ROSEGARDENSEQUENCERIFACE_H_

#include <dcopobject.h>
#include "rosegardendcop.h"
#include "Event.h"

class RosegardenSequencerIface : virtual public DCOPObject
{
    K_DCOP
public:
    k_dcop:

    // close the sequencer
    //
    virtual void quit() = 0;

    // play from a given time with given parameters
    //
    virtual int play(long timeSec,
                     long timeUsec,
                     long playLatencySec,
                     long playLatencyUSec,
                     long fetchLatencySec,
                     long fetchLatencyUSec,
                     long readAheadSec,
                     long readAheadUSec) = 0;

    // record from a given time with given parameters
    //
    virtual int record(long timeSec,
                       long timeUSec,
                       long playLatencySec,
                       long playLatencyUSec,
                       long fetchLatencySec,
                       long fetchLatencyUSec,
                       long readAheadSec,
                       long readAheadUSec,
                       int recordMode) = 0;

    // stop the sequencer
    //
    virtual void stop() = 0;

    // Set the sequencer to a given time
    //
    virtual void jumpTo(long posSec, long posUSec) = 0;

    // Set a loop on the sequencer
    //
    virtual void setLoop(long loopStartSec,
                         long loopStartUSec,
                         long loopEndSec,
                         long loopEndUSec) = 0;

    // Get the status of the Sequencer
    //
    virtual int getSoundSystemStatus() = 0;

    // Add and delete audio files on the Sequencer
    //
    virtual int addAudioFile(const QString &fileName, int id) = 0;
    virtual int removeAudioFile(int id) = 0;
    virtual void clearAllAudioFiles() = 0;

    // Single set function as the MappedInstrument is so lightweight.
    // Any mods on the GUI are sent only through this method.
    //
    virtual void setMappedInstrument(int type,
                                     short int channel,
                                     unsigned int id) = 0;


};

#endif // _ROSEGARDENSEQUENCERIFACE_H_
