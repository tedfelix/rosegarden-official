/***************************************************************************
                          scale.cpp  -  description
                             -------------------
    begin                : Mon May 7 2001
    copyright            : (C) 2001 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "scale.h"

#include "rosedebug.h"
#include "notationelement.h"

static unsigned int Scales[][7] = { // pitches defined in staff.cpp
    { 0, 2, 4, 5, 7, 9,  11 }, // C
    { 0, 2, 4, 6, 7, 9,  11 }, // G
    { 1, 2, 4, 6, 7, 9,  11 }, // D
    { 1, 2, 4, 6, 8, 9,  11 }, // A
    { 1, 3, 4, 6, 8, 9,  11 }, // E
    { 1, 3, 4, 6, 8, 10, 11 }, // B
    { 1, 3, 5, 6, 8, 10, 11 }, // F#
    { 1, 3, 5, 6, 8, 10, 0 } // C#
};

Scale::Scale(KeySignature keysig)
    : m_keySignature(keysig),
      m_useSharps(keysig < F),
      m_notes(12)
{
    for (unsigned int i = 0; i < m_notes.size(); ++i) {
        m_notes[i] = false;
    }

    if (keysig > 7) keysig = KeySignature(keysig - 7);

    for (unsigned int i = 0; i < 7; ++i) {
//         kdDebug(KDEBUG_AREA) << QString("Scales[%1][%2]").arg(keysig).arg(i)
//                              << " = " << Scales[keysig][i] << endl;

        m_notes[Scales[keysig][i]] = true;
    }
}

bool
Scale::pitchIsInScale(unsigned int pitch) const
{
//     kdDebug(KDEBUG_AREA) << QString("Scale::pitchIsInScale(%1)").arg(pitch) << endl;

    if (pitch >= m_notes.size()) { // TODO - this will break if pitch < 0

        pitch = pitch % m_notes.size();
//         kdDebug(KDEBUG_AREA) << "Scale::pitchIsInScale() - pitch corrected to "
//                              << pitch << endl;
    }

    bool res = m_notes[pitch];

//     kdDebug(KDEBUG_AREA) << "Scale::pitchIsInScale() - res = "
//                          << res << endl;

    return res;
}

bool
Scale::noteIsDecorated(const NotationElement &el) const
{
    try {
        int pitch = el.event()->get<Int>("pitch");

        return pitchIsDecorated(pitch);

    } catch (Event::NoData) {
        kdDebug(KDEBUG_AREA) << "Scale::noteIsDecorated() : couldn't get pitch for element"
                             << endl;
        return false;
    }
}

//////////////////////////////////////////////////////////////////////
