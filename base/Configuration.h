// -*- c-basic-offset: 4 -*-

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


// Class to hold extraenous bits of configuration which
// don't sit inside the Composition itself - sequencer
// and other general stuff that we want to keep separate.
//
//

#include <string>

#include "Instrument.h"
#include "RealTime.h"
#include "XmlExportable.h"

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

namespace Rosegarden
{

class Configuration : public XmlExportable
{
public:

    Configuration(); // defaults
    ~Configuration();

    void setFetchLatency(const RealTime &value) { m_fetchLatency = value; }
    RealTime getFetchLatency() const { return m_fetchLatency; }

    void setMetronomePitch(MidiByte value) { m_metronomePitch = value; }
    MidiByte getMetronomePitch() const { return m_metronomePitch; }

    void setMetronomeBarVelocity(MidiByte value)
        { m_metronomeBarVelocity = value; }
    MidiByte getMetronomeBarVelocity() const { return m_metronomeBarVelocity; }

    void setMetronomeBeatVelocity(MidiByte value)
        { m_metronomeBeatVelocity = value; }
    MidiByte getMetronomeBeatVelocity() const {return m_metronomeBeatVelocity;}

    void setMetronomeDuration(const RealTime &value)
        { m_metronomeDuration = value; }
    RealTime getMetronomeDuration() const { return m_metronomeDuration; }

    // for exporting
    //
    virtual std::string toXmlString();

private:

    RealTime     m_fetchLatency;

    MidiByte     m_metronomePitch;
    MidiByte     m_metronomeBarVelocity;
    MidiByte     m_metronomeBeatVelocity;
    RealTime     m_metronomeDuration;

//     DoubleClickClient   m_client;
    
};

}

#endif // _AUDIODEVICE_H_
