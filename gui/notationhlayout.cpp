/***************************************************************************
                          notationhlayout.cpp  -  description
                             -------------------
    begin                : Thu Aug 3 2000
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

#include "staff.h"
#include "notationhlayout.h"
#include "rosedebug.h"

NotationHLayout::NotationHLayout(unsigned int barWidth,
                                 unsigned int beatsPerBar,
                                 unsigned int barMargin,
                                 unsigned int noteMargin)
    : m_barWidth(barWidth),
      m_beatsPerBar(beatsPerBar),
      m_barMargin(barMargin),
      m_noteMargin(noteMargin),
      m_nbBeatsInCurrentBar(0),
      m_currentPos(barMargin),
      m_noteLengthMap(LastNote)
{
    initNoteLengthTable();
}

void
NotationHLayout::layout(Event *el)
{
    // if (el) is time sig change, reflect that

    // kdDebug(KDEBUG_AREA) << "Layout" << endl;

    m_quantizer.quantize( el );

    // kdDebug(KDEBUG_AREA) << "Quantized" << endl;

    // Add note to current bar
    m_nbBeatsInCurrentBar += el->get<Int>("QuantizedDuration") / m_quantizer.wholeNoteDuration();

    // kdDebug(KDEBUG_AREA) << "m_nbBeatsInCurrentBar : " << m_nbBeatsInCurrentBar << endl;

    if (m_nbBeatsInCurrentBar > m_beatsPerBar) {
        kdDebug(KDEBUG_AREA) << "split element" << endl;
        // TODO
    } else if (m_nbBeatsInCurrentBar == m_beatsPerBar) {
        kdDebug(KDEBUG_AREA) << "start a new bar" << endl;
        // TODO
    }    

    kdDebug(KDEBUG_AREA) << "set to m_currentPos = " << m_currentPos << endl;
    el->set<Int>("Notation::X", m_currentPos);

    Note note = Note(el->get<Int>("Notation::NoteType")); // check the property is here ?

    // Move current pos to next note
    m_currentPos += m_noteLengthMap[note] + Staff::noteWidth + m_noteMargin;

    kdDebug(KDEBUG_AREA) << "m_currentPos pushed to = " << m_currentPos << endl;
}

void
NotationHLayout::initNoteLengthTable(void)
{
    m_noteLengthMap[Whole]        = m_barWidth;
    m_noteLengthMap[Half]         = m_barWidth / 2;
    m_noteLengthMap[Quarter]      = m_barWidth / 4;
    m_noteLengthMap[Eighth]       = m_barWidth / 8;
    m_noteLengthMap[Sixteenth]    = m_barWidth / 16;
    m_noteLengthMap[ThirtySecond] = m_barWidth / 32;
    m_noteLengthMap[SixtyFourth]  = m_barWidth / 64;

    m_noteLengthMap[WholeDotted]        = m_noteLengthMap[Whole]        + m_noteLengthMap[Half];
    m_noteLengthMap[HalfDotted]         = m_noteLengthMap[Half]         + m_noteLengthMap[Quarter];
    m_noteLengthMap[QuarterDotted]      = m_noteLengthMap[Quarter]      + m_noteLengthMap[Eighth];
    m_noteLengthMap[EighthDotted]       = m_noteLengthMap[Eighth]       + m_noteLengthMap[Sixteenth];
    m_noteLengthMap[SixteenthDotted]    = m_noteLengthMap[Sixteenth]    + m_noteLengthMap[ThirtySecond];
    m_noteLengthMap[ThirtySecondDotted] = m_noteLengthMap[ThirtySecond] + m_noteLengthMap[SixtyFourth];
    m_noteLengthMap[SixtyFourthDotted]  = m_noteLengthMap[SixtyFourth]  + m_noteLengthMap[SixtyFourth] / 2;

}
