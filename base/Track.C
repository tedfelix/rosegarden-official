// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "Track.h"

namespace Rosegarden
{

Track::Track():
   m_id(0),
   m_muted(false),
   m_type(Midi),
   m_label("untitled"),
   m_position(0),
   m_instrument(0)
{
}

Track::Track(const int &id, const bool &muted, 
             const TrackType &tt, const string &label,
             const int &position, const int &instrument):
   m_id(id),
   m_muted(muted),
   m_type(tt),
   m_label(label),
   m_position(position),
   m_instrument(instrument)
{
}
   

Track::~Track()
{
}

}


