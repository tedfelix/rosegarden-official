// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _AUDIO_LEVEL_H_
#define _AUDIO_LEVEL_H_

namespace Rosegarden {

/**
 * We need to represent audio levels in three different ways: as dB
 * values; as a floating-point multiplier for gain; and as an integer
 * on a scale for fader position and vu level.  This class does the
 * necessary conversions.
 */

class AudioLevel
{
public:

    enum FaderType {
	ShortFader = 0, // -40 -> +6  dB
  	 LongFader = 1  // -70 -> +10 dB
    };

    static float multiplier_to_dB(float multiplier);
    static float dB_to_multiplier(float dB);

    static float fader_to_dB(int level, int maxLevel, FaderType type);
    static int   dB_to_fader(float dB, int maxFaderLevel, FaderType type);

    static float fader_to_multiplier(int level, int maxLevel, FaderType type);
    static int   multiplier_to_fader(float multiplier, int maxFaderLevel,
				     FaderType type);

    /**
     * The Instrument stores "volume" as a single byte integer, so we
     * treat it as a long-fader position with max level 255.
     */

    static float volume_to_dB(int v)
    { return fader_to_dB(v, 255, LongFader); }

    static float volume_to_multiplier(int v)
    { return fader_to_multiplier(v, 255, LongFader); }

    static int dB_to_volume(float dB)
    { return dB_to_fader(dB, 255, LongFader); }

    static int multiplier_to_volume(float m)
    { return multiplier_to_fader(m, 255, LongFader); }
};

}

#endif

    
