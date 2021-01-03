/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/AudioLevel.h"
#include <cmath>
#include <iostream>
#include <map>
#include <vector>

namespace Rosegarden {

struct FaderDescription
{
    FaderDescription(float i_minDb, float i_maxDb, float i_zeroPoint) :
        minDb(i_minDb), maxDb(i_maxDb), zeroPoint(i_zeroPoint) { }

    float minDb;
    float maxDb;
    float zeroPoint; // as fraction of total throw
};

static const FaderDescription faderTypes[] = {
    FaderDescription(-40.0,  +6.0, 0.75), // short
    FaderDescription(-70.0, +10.0, 0.80), // long
    FaderDescription(-70.0,   0.0, 1.00), // IEC268
    FaderDescription(-70.0, +10.0, 0.80), // IEC268 long
    FaderDescription(-40.0,   0.0, 1.00), // preview
};

typedef std::vector<float> LevelList;
static std::map<int, LevelList> previewLevelCache;
static const LevelList &getPreviewLevelCache(int levels);

float
AudioLevel::multiplier_to_dB(float multiplier)
{
    if (multiplier == 0.0) return DB_FLOOR;
    float dB = 10 * log10f(multiplier);
    return dB;
}

float
AudioLevel::dB_to_multiplier(float dB)
{
    if (dB == DB_FLOOR) return 0.0;
    float m = powf(10.0, dB / 10.0);
    return m;
}

/* IEC 60-268-18 fader levels.  Thanks to Steve Harris. */

static float iec_dB_to_fader(float db)
{
    float def = 0.0f; // Meter deflection %age

    if (db < -70.0f) {
        def = 0.0f;
    } else if (db < -60.0f) {
        def = (db + 70.0f) * 0.25f;
    } else if (db < -50.0f) {
        def = (db + 60.0f) * 0.5f + 2.5f; // corrected from 5.0f base, thanks Robin Gareus
    } else if (db < -40.0f) {
        def = (db + 50.0f) * 0.75f + 7.5f;
    } else if (db < -30.0f) {
        def = (db + 40.0f) * 1.5f + 15.0f;
    } else if (db < -20.0f) {
        def = (db + 30.0f) * 2.0f + 30.0f;
    } else {
        def = (db + 20.0f) * 2.5f + 50.0f;
    }

    return def;
}

static float iec_fader_to_dB(float def)  // Meter deflection %age
{
    float db = 0.0f;

    if (def >= 50.0f) {
        db = (def - 50.0f) / 2.5f - 20.0f;
    } else if (def >= 30.0f) {
        db = (def - 30.0f) / 2.0f - 30.0f;
    } else if (def >= 15.0f) {
        db = (def - 15.0f) / 1.5f - 40.0f;
    } else if (def >= 7.5f) {
        db = (def - 7.5f) / 0.75f - 50.0f;
    } else if (def >= 2.5f) {
        db = (def - 2.5f) / 0.5f - 60.0f;
    } else {
        db = (def / 0.25f) - 70.0f;
    }

    return db;
}

float
AudioLevel::fader_to_dB(int level, int maxLevel, FaderType type)
{
    if (level == 0) return DB_FLOOR;

    if (type == IEC268Meter || type == IEC268LongMeter) {

        float maxPercent = iec_dB_to_fader(faderTypes[type].maxDb);
        float percent = float(level) * maxPercent / float(maxLevel);
        float dB = iec_fader_to_dB(percent);
        return dB;

    } else { // scale proportional to sqrt(fabs(dB))

        int zeroLevel = int(maxLevel * faderTypes[type].zeroPoint);

        if (level >= zeroLevel) {

            float value = level - zeroLevel;
            float scale = float(maxLevel - zeroLevel) /
                sqrtf(faderTypes[type].maxDb);
            value /= scale;
            float dB = powf(value, 2.0);
            return dB;

        } else {

            float value = zeroLevel - level;
            float scale = zeroLevel / sqrtf(0.0 - faderTypes[type].minDb);
            value /= scale;
            float dB = powf(value, 2.0);
            return 0.0 - dB;
        }
    }
}


int
AudioLevel::dB_to_fader(float dB, int maxLevel, FaderType type)
{
    if (dB == DB_FLOOR) return 0;

    if (type == IEC268Meter || type == IEC268LongMeter) {

        // The IEC scale gives a "percentage travel" for a given dB
        // level, but it reaches 100% at 0dB.  So we want to treat the
        // result not as a percentage, but as a scale between 0 and
        // whatever the "percentage" for our (possibly >0dB) max dB is.

        float maxPercent = iec_dB_to_fader(faderTypes[type].maxDb);
        float percent = iec_dB_to_fader(dB);
        int faderLevel = int((maxLevel * percent) / maxPercent + 0.01);

        if (faderLevel < 0) faderLevel = 0;
        if (faderLevel > maxLevel) faderLevel = maxLevel;
        return faderLevel;

    } else {

        int zeroLevel = int(maxLevel * faderTypes[type].zeroPoint);

        if (dB >= 0.0) {

            float value = sqrtf(dB);
            float scale = (maxLevel - zeroLevel) / sqrtf(faderTypes[type].maxDb);
            value *= scale;
            int level = int(value + 0.01) + zeroLevel;
            if (level > maxLevel) level = maxLevel;
            return level;

        } else {

            dB = 0.0 - dB;
            float value = sqrtf(dB);
            float scale = zeroLevel / sqrtf(0.0 - faderTypes[type].minDb);
            value *= scale;
            int level = zeroLevel - int(value + 0.01);
            if (level < 0) level = 0;
            return level;
        }
    }
}


float
AudioLevel::fader_to_multiplier(int level, int maxLevel, FaderType type)
{
    if (level == 0) return 0.0;
    return dB_to_multiplier(fader_to_dB(level, maxLevel, type));
}

int
AudioLevel::multiplier_to_fader(float multiplier, int maxLevel, FaderType type)
{
    if (multiplier == 0.0) return 0;
    float dB = multiplier_to_dB(multiplier);
    int fader = dB_to_fader(dB, maxLevel, type);
    return fader;
}


const LevelList &
getPreviewLevelCache(int levels)
{
    LevelList &ll = previewLevelCache[levels];
    if (ll.empty()) {
        for (int i = 0; i <= levels; ++i) {
            float m = AudioLevel::fader_to_multiplier
                (i, levels, AudioLevel::PreviewLevel);
            if (levels == 1) m /= 100; // noise
            ll.push_back(m);
        }
    }
    return ll;
}

int
AudioLevel::multiplier_to_preview(float m, int levels)
{
    const LevelList &ll = getPreviewLevelCache(levels);
    int result = -1;

    int lo = 0, hi = levels;

    // binary search
    int level = -1;
    while (result < 0) {
        int newlevel = (lo + hi) / 2;
        if (newlevel == level ||
            newlevel == 0 ||
            newlevel == levels) {
            result = newlevel;
            break;
        }
        level = newlevel;
        if (ll[level] >= m) {
            hi = level;
        } else if (ll[level+1] >= m) {
            result = level;
        } else {
            lo = level;
        }
    }

    return result;
}

float
AudioLevel::preview_to_multiplier(int level, int levels)
{
    const LevelList &ll = getPreviewLevelCache(levels);
    return ll[level];
}

int AudioLevel::m_panLaw = 0;

float
AudioLevel::panGainLeft(float pan)  // Apply panning law to left channel
{
    if (m_panLaw == 3) {
        // -3dB Panning Law (variant)
        //
        // This law has the same characteristics as the -3dB law described
        // below, except that a channel's gain begins at +3dB and decreases to
        // 0dB as the control is moved through the center position.  This
        // setting must be used with caution, as the increased edge gains could
        // introduce clipping.
        //
        return sqrtf(fabsf((100.0 - pan) / 100.0));  // -3dB pan law  (variant)

    } else if (m_panLaw == 2) {
        // -6dB Panning Law
        //
        // A channel's gain begins at 0dB and decreases to -6dB as the control
        // is moved to the center.  From there on the signal continues to
        // decrease until it is completely attenuated at the opposite edge.
        // The 3dB dip in the combined power of both channels when the control
        // is centered will cause the extremes to sound louder than the center.
        //
        return (100.0 - pan) / 200.0;

    } else if (m_panLaw == 1) {
        // -3dB Panning Law
        //
        // A channel's gain begins at 0dB and decreases to -3dB as the control
        // is centered.  From there on the signal continues to decrease until
        // it is completely attenuated at the opposite edge.  The combined
        // power of both channels remains constant throughout the panning
        // range, resulting in an apparent constant loudness.
        //
        return sqrtf(fabsf((100.0 - pan) / 200.0));

    } else {
        // OdB Panning Law (default)
        //
        // A channel's gain begins at 0dB and remains at 0dB as the control is
        // moved to the the center.  From there on the signal decreases
        // linearly till it is completely attenuated at the opposite edge.
        // Since both channels have a gain of 0dB at the center, the net
        // effect is a 3dB boost in apparent loudness when the control is
        // centered.  This is a basic balance pot.
        //
        return (pan > 0.0) ? (100.0 - pan) / 100.0 : 1.0;

    }
}

float
AudioLevel::panGainRight(float pan)  // Apply panning law to right channel
{
    if (m_panLaw == 3) {
        return sqrtf(fabsf((100.0 + pan) / 100.0));  // -3dB pannig law (variant)

    } else if (m_panLaw == 2) {
        return (100.0 + pan) / 200.0;  // -6dB pan law

    } else if (m_panLaw == 1) {
        return sqrtf(fabsf((100.0 + pan) / 200.0));  // -3dB panning law

    } else {
        return (pan < 0.0) ? (100.0 + pan) / 100.0 : 1.0;  // 0dB panning law (default)

    }
}

double AudioLevel::AudioPanD(MidiByte midiPan)
{
    // Scale 0-64 from 0 to 100.
    if (midiPan <= 64)
        return static_cast<double>(midiPan) / 64.0 * 100.0;

    // Scale 64-127 from 100 to 200.
    return static_cast<double>(midiPan - 1) / 63.0 * 100.0;
}

int AudioLevel::AudioPanI(MidiByte midiPan)
{
    // ??? A table would be faster.

    // Scale 0-64 from 0 to 100.
    if (midiPan <= 64) {
        // Adding 32 (half of 64) to get proper rounding.
        return (midiPan * 100 + 32) / 64;
    }

    // Scale 64-127 from 100 to 200.
    // Adding 31 (half of 63 truncated) to get proper rounding.
    return ((midiPan - 1) * 100 + 31) / 63;
}

MidiByte AudioLevel::MIDIPanI(int audioPan)
{
    // Scale 0-100 to 0-64.  (Add 50 to get proper rounding.)
    if (audioPan <= 100)
        return (audioPan * 64 + 50) / 100;

    // Scale 100-200 to 64-127.  (Add 50 to get proper rounding.)
    return (audioPan * 63 + 50) / 100 + 1;
}


}
