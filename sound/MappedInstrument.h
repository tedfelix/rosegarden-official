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

#include "Instrument.h"
#include "MappedDevice.h"

#ifndef _MAPPEDINSTRUMENT_H_
#define _MAPPEDINSTRUMENT_H_

// A scaled-down version of an Instrument that we keep Sequencer
// side.  IDs match with those on the GUI.
//
//

namespace Rosegarden
{

typedef int MappedInstrumentSubOrdering;

class MappedInstrument
{
public:

    // GUI uses this constructor because it already knows
    // the name of the Instrument
    //
    MappedInstrument(Instrument::InstrumentType type,
                     MidiByte channel,
                     InstrumentId id);

    // Driver uses this constructor (because the gui will want
    // to know the name)
    //
    MappedInstrument(Instrument::InstrumentType type,
                     MidiByte channel,
                     InstrumentId id,
                     MappedInstrumentSubOrdering subOrdering,
                     const std::string &name,
                     DeviceId device);

    // from instrument
    MappedInstrument(const Instrument &instrument);
    MappedInstrument(Instrument *instrument);

    // extra and copy
    MappedInstrument(MappedInstrument *mI);


    ~MappedInstrument();

    void setId(InstrumentId id) { m_id = id; }
    InstrumentId getId() const { return m_id; }

    void setChannel(MidiByte channel) { m_channel = channel; }
    MidiByte getChannel() const { return m_channel; }

    void setType(Instrument::InstrumentType type) { m_type = type; }
    Instrument::InstrumentType getType() const { return m_type; }

    void setName(const std::string &name) { m_name = name; }
    const std::string& getName() const { return m_name; }

    void setDevice(DeviceId device) { m_device = device; }
    DeviceId getDevice() const { return m_device; }

    void setSubOrdering(MappedInstrumentSubOrdering order) { m_subOrdering = order; }
    MappedInstrumentSubOrdering getSubOrdering() const { return m_subOrdering; }

private:

    Instrument::InstrumentType  m_type;
    MidiByte                    m_channel;
    InstrumentId                m_id;
    std::string                 m_name;
    DeviceId                    m_device;

    MappedInstrumentSubOrdering m_subOrdering;

};

}

#endif // _MAPPEDINSTRUMENT_H_
