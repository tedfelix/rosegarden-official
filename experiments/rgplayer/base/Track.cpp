// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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
#include <iostream>
#include <cstdio>

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

#include "Composition.h"

namespace Rosegarden
{

Track::Track():
   m_id(0),
   m_muted(false),
   m_position(-1),
   m_instrument(0),
   m_owningComposition(0),
   m_input_device(Device::ALL_DEVICES),
   m_input_channel(-1),
   m_armed(false),
   m_clef(0),
   m_transpose(0),
   m_color(0),
   m_highestPlayable(127),
   m_lowestPlayable(0)
{
}

Track::Track(TrackId id,
             InstrumentId instrument,
             int position,
             const std::string &label,
             bool muted):
   m_id(id),
   m_muted(muted),
   m_label(label),
   m_position(position),
   m_instrument(instrument),
   m_owningComposition(0),
   m_input_device(Device::ALL_DEVICES),
   m_input_channel(-1),
   m_armed(false),
   m_clef(0),
   m_transpose(0),
   m_color(0),
   m_highestPlayable(127),
   m_lowestPlayable(0)
{
}

Track::~Track()
{
}

void Track::setMuted(bool muted)
{
    if (m_muted == muted) return;

    m_muted = muted;

    if (m_owningComposition)
        m_owningComposition->notifyTrackChanged(this);
}

void Track::setLabel(const std::string &label)
{
    if (m_label == label) return;

    m_label = label;

    if (m_owningComposition)
        m_owningComposition->notifyTrackChanged(this);
}

void Track::setPresetLabel(const std::string &label)
{
    if (m_presetLabel == label) return;

    m_presetLabel = label;

    if (m_owningComposition)
        m_owningComposition->notifyTrackChanged(this);
}

void Track::setInstrument(InstrumentId instrument)
{
    if (m_instrument == instrument) return;

    m_instrument = instrument;

    if (m_owningComposition)
        m_owningComposition->notifyTrackChanged(this);
}


void Track::setArmed(bool armed) 
{ 
    if (m_armed == armed) return;

    m_armed = armed; 
    
    if (m_owningComposition)
        m_owningComposition->notifyTrackChanged(this);
} 

void Track::setMidiInputDevice(DeviceId id) 
{ 
    if (m_input_device == id) return;

    m_input_device = id; 

    if (m_owningComposition)
        m_owningComposition->notifyTrackChanged(this);
}

void Track::setMidiInputChannel(char ic) 
{ 
    if (m_input_channel == ic) return;

    m_input_channel = ic; 

    if (m_owningComposition)
        m_owningComposition->notifyTrackChanged(this);
}


// Our virtual method for exporting Xml.
//
//
std::string Track::toXmlString()
{

    std::stringstream track;

    track << "<track id=\"" << m_id;
    track << "\" label=\"" << encode(m_label);
    track << "\" position=\"" << m_position;

    track << "\" muted=";

    if (m_muted)
        track << "\"true\"";
    else
        track << "\"false\"";

    track << " instrument=\"" << m_instrument << "\"";

    track << " defaultLabel=\"" << m_presetLabel << "\"";
    track << " defaultClef=\"" << m_clef << "\"";
    track << " defaultTranspose=\"" << m_transpose << "\"";
    track << " defaultColour=\"" << m_color << "\"";
    track << " defaultHighestPlayable=\"" << m_highestPlayable << "\"";
    track << " defaultLowestPlayable=\"" << m_lowestPlayable << "\"";

#if (__GNUC__ < 3)
    track << "/>"<< std::ends;
#else
    track << "/>";
#endif

    return track.str();

}

}


