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

#include "Track.h"


#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


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

Track::Track(const TrackId &id, const bool &muted, 
             const TrackType &tt, const std::string &label,
             const unsigned int &position, const int &instrument):
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




// Our virtual method for exporting Xml.
//
//
std::string
Track::toXmlString()
{
    std::stringstream track;

    track << "<track id=\"";
    track << m_id << "\" type=\"";

    switch(m_type)
    {
        case Midi:
            track << "midi";
            break;

        case Audio:
            track << "audio";
            break;

        default:
            track << "unknown";
            break;
    }


    track << "\" label=\"" << m_label;
    track << "\" position=\"" << m_position;

    track << "\" muted=";

    if (m_muted)
        track << "\"true\"";
    else
        track << "\"false\"";

    track << " instrument=\"" << m_instrument << "\"";

    track << "/>" << std::ends;

    return track.str();

}


}


