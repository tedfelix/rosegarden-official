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

#include "AudioLevel.h"
#include <cmath>

namespace Rosegarden {

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
};

float
AudioLevel::multiplier_to_dB(float multiplier)
{
    return 10 * log10f(multiplier);
}

float
AudioLevel::dB_to_multiplier(float dB)
{
    return powf(10.0, dB / 10.0);
}

float
AudioLevel::fader_to_dB(int level, int maxLevel, FaderType type)
{
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
	float scale = zeroLevel / sqrtf(-faderTypes[type].minDb);
	value /= scale;
	float dB = powf(value, 2.0);
	return -dB;

    }
}

int
AudioLevel::dB_to_fader(float dB, int maxLevel, FaderType type)
{
    int zeroLevel = int(maxLevel * faderTypes[type].zeroPoint);
    
    if (dB >= 0.0) {

	float value = sqrtf(dB);
	float scale = (maxLevel - zeroLevel) / sqrtf(faderTypes[type].maxDb);
	value *= scale;
	int level = int(value + 0.01) + zeroLevel;
	if (level > maxLevel) level = maxLevel;
	return level;

    } else {

	float value = sqrtf(-dB);
	float scale = zeroLevel / sqrtf(-faderTypes[type].minDb);
	value *= scale;
	int level = zeroLevel - int(value + 0.01);
	if (level < 0) level = 0;
	return level;

    }
}
	
float
AudioLevel::fader_to_multiplier(int level, int maxLevel, FaderType type)
{
    if (level == 0) return 0.0; // special case for silence
    return dB_to_multiplier(fader_to_dB(level, maxLevel, type));
}

int
AudioLevel::multiplier_to_fader(float multiplier, int maxLevel, FaderType type)
{
    if (multiplier == 0.0) return 0; // special case for silence
    return dB_to_fader(multiplier_to_dB(multiplier), maxLevel, type);
}

}

