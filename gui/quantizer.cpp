/***************************************************************************
                          quantizer.cpp  -  description
                             -------------------
    begin                : Thu Aug 17 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
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

#include "rosedebug.h"
#include "quantizer.h"
#include "notepixmapfactory.h"

unsigned int
Quantizer::defaultWholeNoteDuration = 384;

Quantizer::Quantizer()
    : m_durationTable(0),
      m_wholeNoteDuration(defaultWholeNoteDuration)
{
    computeNoteDurations();
}

void
Quantizer::setWholeNoteDuration(unsigned int d)
{
    m_wholeNoteDuration = d;
    computeNoteDurations();
}

void
Quantizer::computeNoteDurations()
{
    m_durationTable.clear();

    m_durationTable[Whole]        = m_wholeNoteDuration;
    m_durationTable[Half]         = m_wholeNoteDuration / 2;
    m_durationTable[Quarter]      = m_wholeNoteDuration / 4;
    m_durationTable[Eighth]       = m_wholeNoteDuration / 8;
    m_durationTable[Sixteenth]    = m_wholeNoteDuration / 16;
    m_durationTable[ThirtySecond] = m_wholeNoteDuration / 32;
    m_durationTable[SixtyFourth]  = m_wholeNoteDuration / 64;

    m_durationTable[WholeDotted]        = m_durationTable[Whole] + m_durationTable[Half];
    m_durationTable[HalfDotted]         = m_durationTable[Half] + m_durationTable[Quarter];
    m_durationTable[QuarterDotted]      = m_durationTable[Quarter] + m_durationTable[Eighth];
    m_durationTable[EighthDotted]       = m_durationTable[Eighth] + m_durationTable[Sixteenth];
    m_durationTable[SixteenthDotted]    = m_durationTable[Sixteenth] + m_durationTable[ThirtySecond];
    m_durationTable[ThirtySecondDotted] = m_durationTable[ThirtySecond] + m_durationTable[SixtyFourth];
    m_durationTable[SixtyFourthDotted]  = m_durationTable[SixtyFourth] + m_durationTable[SixtyFourth] / 2;

}


void
Quantizer::quantize(EventList::iterator from,
                    EventList::iterator to)
{
    // TODO
}

void
Quantizer::quantizeToNoteTypes(EventList::iterator from,
                               EventList::iterator to)
{
    EventList::iterator it = from;

    while (it != to) {

        Note note = quantizeToNoteType( (*it)->getDuration() );
        (*it)->set<Int>("Notation::NoteType", note);

        ++it;
    }
}

Note
Quantizer::quantizeToNoteType(Event::duration drt)
{
    // In the following comments, "note" means "note type", as in
    // "whole", "halfdotted", "sixteenth", etc...
    //
    Note note = WholeDotted;

    // Find which note the event's duration is closest to
    //
    DurationMap::iterator lb = lower_bound(m_durationTable.begin(),
                                           m_durationTable.end(),
                                           drt);
    if (lb == m_durationTable.begin()) {
        // event duration is longer than a WholeDotted note
        // the event should either be displayed as several whole notes
        // linked together, or actually broken down in several events,
        // themselves linked together... big TODO here
        note = WholeDotted;

    } else {

        // lb points at the first note which duration is higher
        // than the one of the event
        // e.g. Given the following set of values : 2, 4, 6, 8
        // lower_bound(5) is 6
        // lower_bound(4) is 4
        // lower_bound(3) is 3
        //

        // Now we have to check if the event's duration is closer to
        // the note lb points to, or the one before
        // e.g. Given the same set of values
        // lower_bound(5.7) is 6 -> quantize to 6
        // lower_bound(4.5) is 6 -> quantize to 4,
        // because it's closer to 4 than to 6
        //
        Event::duration highDuration = *lb,
            lowDuration = *(lb - 1);

        kdDebug(KDEBUG_AREA) << "highDuration : " << highDuration
                             << " - lowDuration : " << lowDuration << "\n";

        if ((highDuration - drt) > (drt - lowDuration)) {
            note = Note(distance(m_durationTable.begin(), lb - 1));
        } else {
            note = Note(distance(m_durationTable.begin(), lb));
        }
    }

    kdDebug(KDEBUG_AREA) << "Quantized to note : " << note << "\n";

    return note;
}
