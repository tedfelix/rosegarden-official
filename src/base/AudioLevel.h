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

#ifndef RG_AUDIO_LEVEL_H
#define RG_AUDIO_LEVEL_H

#include "base/MidiProgram.h"  // for MidiByte

namespace Rosegarden {


/// Audio math utility class.
/**
 * We need to represent audio levels in three different ways: as dB
 * values; as a floating-point multiplier for gain; and as an integer
 * on a scale for fader position and vu level.  This class does the
 * necessary conversions.
 */
class AudioLevel
{
public:

    static constexpr float DB_FLOOR = -1000.0;

    enum FaderType {
             ShortFader = 0, // -40 -> +6  dB
              LongFader = 1, // -70 -> +10 dB
            IEC268Meter = 2, // -70 ->  0  dB
        IEC268LongMeter = 3, // -70 -> +10 dB (0dB aligns with LongFader)
           PreviewLevel = 4
    };

    static float multiplier_to_dB(float multiplier);
    static float dB_to_multiplier(float dB);

    static float fader_to_dB(int level, int maxLevel, FaderType type);
    static int   dB_to_fader(float dB, int maxFaderLevel, FaderType type);

    static float fader_to_multiplier(int level, int maxLevel, FaderType type);
    static int   multiplier_to_fader(float multiplier, int maxFaderLevel,
                                     FaderType type);

    // fast if "levels" doesn't change often -- for audio segment previews
    static int   multiplier_to_preview(float multiplier, int levels);
    static float preview_to_multiplier(int level, int levels);

    // Set or retrieve the number of the pan law.
    static void setPanLaw(int panLaw) { m_panLaw = panLaw; }
    static int getPanLaw() { return m_panLaw; }

    /// Apply pan law.  Assumes pan range 0 - 100 - 200.
    static float panGainLeft(float pan);
    static float panGainRight(float pan);

    /// Convert MIDI pan (0 - 64 - 127) to Audio pan (0 - 100 - 200).
    /**
     * You may need to subtract 100 from this as some parts of the system deal
     * in -100 - 100 pan.  E.g. the pan knobs in the UI.
     */
    static double AudioPanD(MidiByte midiPan);
    /// Integer version of AudioPanD().
    static int AudioPanI(MidiByte midiPan);

    /// Convert Audio pan (0 - 100 - 200) to MIDI pan (0 - 64 - 127).
    static MidiByte MIDIPanI(int audioPan);

private:

    static int m_panLaw;  // number of pan law currently in use
};


}

#endif

    
