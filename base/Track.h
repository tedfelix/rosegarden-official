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

// Representation of a Track
//
//


#ifndef _TRACK_H_
#define _TRACK_H_

#include "XmlExportable.h"
#include "Instrument.h"
#include <string>


namespace Rosegarden
{

typedef unsigned int TrackId;

/**
 * A Track represents a line on the SegmentCanvas on the
 * Rosegarden GUI.  A Track is owned by a Composition and
 * has reference to an Instrument from which the playback
 * characteristics of the Track can be derived.  A Track
 * has no type itself - the type comes only from the
 * Instrument relationship.
 *
 */

class Track : public XmlExportable
{

public:
    Track();
    Track(TrackId id,
          InstrumentId instrument, 
          TrackId position,
          const std::string &label,
          bool muted);

    ~Track();

    void setId(TrackId id) { m_id = id; }
    TrackId getId() const { return m_id; }

    void setMuted(bool muted) { m_muted = muted; }
    bool isMuted() { return m_muted; }

    void setPosition(TrackId position) { m_position = position; }
    TrackId getPosition() { return m_position; }

    void setLabel(const std::string &label) { m_label = label; }
    std::string const getLabel() { return m_label; }

    void setInstrument(InstrumentId instrument) { m_instrument = instrument; }
    InstrumentId getInstrument() { return m_instrument; }

    // Implementation of virtual
    //
    virtual std::string toXmlString();

private:

    TrackId        m_id;
    bool           m_muted;
    std::string    m_label;
    TrackId        m_position;  // we can use TrackId for position
    InstrumentId   m_instrument;

};

}

#endif // _TRACK_H_
 
