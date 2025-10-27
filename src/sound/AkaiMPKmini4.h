/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2020-2025 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#pragma once


#include "base/MidiProgram.h"  // For MidiByte


namespace Rosegarden
{


class MappedEvent;


/**
 * Support for the Akai MPK Mini IV control surface (MIDI port).
 * Christophe MALHAIRE 26/10/2025
 * Inspired by: KorgNanoKontrol2.cpp
 */


class AkaiMPKmini4
{
public:
    AkaiMPKmini4();

    /// Call this after the device is connected to set it up.
    // cppcheck-suppress functionStatic
    void init();

    /// Call when the document is modified to update the LEDs.
    void documentModified(); // currently not used

    void stopped();
    void playing();
    void recording();

    /// Handle MappedEvent's from the external controller port.
    void processEvent(const MappedEvent *i_event);

private:
    // Current 8-channel page.
    unsigned m_page{0};

    void processKnob(MidiByte controlNumber, MidiByte value) const;

    void setPlayStopLED(bool play); // single Play/Stop button
    void setRecordLED(bool record);
    
    bool m_play{false};
    bool m_record{false};
    bool m_stop{false};
    bool m_modified{false};
};


}
