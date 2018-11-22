/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Track.h"

#include "Composition.h"
#include "StaffExportTypes.h"  // StaffTypes and Brackets

#include <sstream>  // std::stringstream


namespace Rosegarden
{


const TrackId NO_TRACK = 0xDEADBEEF;

Track::Track(TrackId id,
             InstrumentId instrument,
             int position,
             const std::string &label,
             bool muted) :
   m_id(id),
   m_muted(muted),
   m_archived(false),
   m_solo(false),
   m_label(label),
   m_shortLabel(""),
   m_position(position),
   m_instrument(instrument),
   m_owningComposition(nullptr),
   m_input_device(Device::ALL_DEVICES),
   m_input_channel(-1),
   m_thruRouting(Auto),
   m_armed(false),
   m_clef(0),
   m_transpose(0),
   m_color(0),
   m_highestPlayable(127),
   m_lowestPlayable(0),
   m_staffSize(StaffTypes::Normal),
   m_staffBracket(Brackets::None)
{
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

void Track::setThruRouting(ThruRouting thruRouting)
{
    // No change, bail.
    if (m_thruRouting == thruRouting)
        return;

    m_thruRouting = thruRouting;

    if (m_owningComposition)
        m_owningComposition->notifyTrackChanged(this);
}


// Our virtual method for exporting Xml.
//
//
std::string Track::toXmlString() const
{

    std::stringstream track;

    track << "<track id=\"" << m_id;
    track << "\" label=\"" << encode(m_label);
    track << "\" shortLabel=\"" << encode(m_shortLabel);
    track << "\" position=\"" << m_position << "\"";

    track << " muted=\"" << (m_muted ? "true" : "false") << "\"";

    track << " archived=\"" << (m_archived ? "true" : "false") <<"\"";

    track << " solo=\"" << (m_solo ? "true" : "false") << "\"";

    track << " instrument=\"" << m_instrument << "\"";

    track << " defaultLabel=\"" << m_presetLabel << "\"";
    track << " defaultClef=\"" << m_clef << "\"";
    track << " defaultTranspose=\"" << m_transpose << "\"";
    track << " defaultColour=\"" << m_color << "\"";
    track << " defaultHighestPlayable=\"" << m_highestPlayable << "\"";
    track << " defaultLowestPlayable=\"" << m_lowestPlayable << "\"";

    track << " staffSize=\"" << m_staffSize << "\"";
    track << " staffBracket=\"" << m_staffBracket << "\"";

    track << " inputDevice=\"" << m_input_device << "\"";
    track << " inputChannel=\"" << static_cast<int>(m_input_channel) << "\"";
    track << " thruRouting=\"" << static_cast<int>(m_thruRouting) << "\"";

    track << "/>";

    return track.str();

}


}
