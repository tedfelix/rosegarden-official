/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDISTRIP_H
#define RG_MIDISTRIP_H

#include "base/Instrument.h"

#include <vector>


namespace Rosegarden
{


class Fader;
class MidiMixerVUMeter;
class Rotary;


/// A strip of controls on the MIDI Mixer window.
// ??? Need to move functionality from MidiMixerWindow into here.
//     See AudioStrip.
class MidiStrip
{
public:
    InstrumentId m_id{0};
    MidiMixerVUMeter *m_vuMeter{nullptr};
    Fader *m_volumeFader{nullptr};
    // ??? We can get rid of this once we get IntrumentId and controller
    //     number into the Rotary objects via QObject properties.  Then
    //     all we need is the Rotary * here.
    struct RotaryInfo
    {
        MidiByte controllerNumber{0};
        Rotary *rotary{nullptr};
    };
    std::vector<RotaryInfo> m_controllerRotaries;
};


}

#endif
