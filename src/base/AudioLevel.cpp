// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2008
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

#include "AudioLevel.h"
#include <cmath>
#include <iostream>
#include <map>
#include <vector>

namespace Rosegarden {

const float AudioLevel::DB_FLOOR = -1000.0;

struct FaderDescription
{
    FaderDescription(float _minDb, float _maxDb, float _zeroPoint) :
	minDb(_minDb), maxDb(_maxDb), zeroPoint(_zeroPoint) { }

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
        def = (db + 60.0f) * 0.5f + 5.0f;
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
    } else if (def >= 5.0f) {
	db = (def - 5.0f) / 0.5f - 60.0f;
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
	

}

