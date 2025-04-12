/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2020-2024 the Rosegarden development team.

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


/// Support for the Korg nanoKONTROL2 control surface.
/**
 * This only supports the nanoKONTROL2 in "CC" mode.  To reset the nanoKONTROL2
 * to factory defaults (removing any custom programming) and switch to CC
 * mode:
 *
 *   Hold down the PREV TRACK, NEXT TRACK, and CYCLE buttons and then
 *   connect the nanoKONTROL2 to USB.
 *
 * More support for the nanoKONTROL2 is present in AlsaDriver and elsewhere.
 * Search the sourcebase on "nanokontrol2" (case insensitive, non-whole-word)
 * to find it.
 */
class KorgNanoKontrol2
{
public:
    KorgNanoKontrol2();

    /// Call this after the device is connected to set it up.
    // cppcheck-suppress functionStatic
    void init();

    /// Call when the document is modified to update the LEDs.
    void documentModified();

    void stopped();
    void playing();
    void recording();

    /// Handle MappedEvent's from the external controller port.
    void processEvent(const MappedEvent *event);

private:
    // Current 8-channel page.
    unsigned m_page{0};

    void processFader(MidiByte controlNumber, MidiByte value) const;
    void processKnob(MidiByte controlNumber, MidiByte value) const;
    void processSolo(MidiByte controlNumber) const;
    void processMute(MidiByte controlNumber) const;
    void processRecord(MidiByte controlNumber) const;

    static void testLEDs(bool on);
    void initLEDs();
    bool m_firstRefresh{true};
    void refreshLEDs();

    bool m_solo[8]{false};
    // If a track is muted, its LED is off.  Unmuted, its LED is on.
    bool m_mute[8]{true};
    bool m_recordArmed[8]{false};

    bool m_play{false};
    bool m_record{false};
    bool m_stop{false};
    void setPlayRecordStopLEDs(bool play, bool record, bool stop);

    bool m_rewind{false};
    bool m_fastForward{false};

    bool m_cycle{false};

};


}
