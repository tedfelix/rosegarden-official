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
      m_timeUnitsPerBar(0),
      m_beatsPerBar(beatsPerBar),
      m_barMargin(barMargin),
      m_noteMargin(noteMargin),
      m_nbTimeUnitsInCurrentBar(0),
      m_previousNbTimeUnitsInCurrentBar(0),
      m_currentPos(barMargin),
      m_noteWidthTable(LastNote)
{
    initNoteWidthTable();
    m_timeUnitsPerBar = m_quantizer.wholeNoteDuration();
    kdDebug(KDEBUG_AREA) << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    kdDebug(KDEBUG_AREA) << "NotationHLayout::~NotationHLayout()" << endl;
}


void
NotationHLayout::layout(NotationElement *el)
{
    // if (el) is time sig change, reflect that

    // kdDebug(KDEBUG_AREA) << "Layout" << endl;

    m_quantizer.quantize(el->event());

    // kdDebug(KDEBUG_AREA) << "Quantized" << endl;

    // Add note to current bar
    m_previousNbTimeUnitsInCurrentBar = m_nbTimeUnitsInCurrentBar;
    m_nbTimeUnitsInCurrentBar += el->event()->get<Int>("QuantizedDuration");

    // kdDebug(KDEBUG_AREA) << "m_nbTimeUnitsInCurrentBar : " << m_nbTimeUnitsInCurrentBar << endl;

    kdDebug(KDEBUG_AREA) << "set to m_currentPos = " << m_currentPos << endl;
    el->setX(m_currentPos);

    Note note = Note(el->event()->get<Int>("Notation::NoteType")); // check the property is here ?

    // Move current pos to next note
    m_currentPos += m_noteWidthTable[note] + Staff::noteWidth + m_noteMargin;

    kdDebug(KDEBUG_AREA) << "m_currentPos pushed to = " << m_currentPos << endl;

    // See if we've completed a bar
    //
    if (m_nbTimeUnitsInCurrentBar > m_timeUnitsPerBar) {
        kdDebug(KDEBUG_AREA) << "Bar has wrong length" << endl;
        // TODO
    } else if (m_nbTimeUnitsInCurrentBar == m_timeUnitsPerBar) {
        kdDebug(KDEBUG_AREA) << "start a new bar" << endl;
        m_nbTimeUnitsInCurrentBar = 0;
        addNewBar(m_currentPos + m_noteMargin);
        m_currentPos += 2 * m_noteMargin + Staff::noteWidth;
        kdDebug(KDEBUG_AREA) << "m_currentPos (bar) pushed to = " << m_currentPos << endl;
    }    

}

void
NotationHLayout::initNoteWidthTable()
{
    m_noteWidthTable[Whole]        = m_barWidth;
    m_noteWidthTable[Half]         = m_barWidth / 2;
    m_noteWidthTable[Quarter]      = m_barWidth / 4;
    m_noteWidthTable[Eighth]       = m_barWidth / 8;
    m_noteWidthTable[Sixteenth]    = m_barWidth / 16;
    m_noteWidthTable[ThirtySecond] = m_barWidth / 32;
    m_noteWidthTable[SixtyFourth]  = m_barWidth / 64;

    m_noteWidthTable[WholeDotted]        = m_noteWidthTable[Whole]        + m_noteWidthTable[Half];
    m_noteWidthTable[HalfDotted]         = m_noteWidthTable[Half]         + m_noteWidthTable[Quarter];
    m_noteWidthTable[QuarterDotted]      = m_noteWidthTable[Quarter]      + m_noteWidthTable[Eighth];
    m_noteWidthTable[EighthDotted]       = m_noteWidthTable[Eighth]       + m_noteWidthTable[Sixteenth];
    m_noteWidthTable[SixteenthDotted]    = m_noteWidthTable[Sixteenth]    + m_noteWidthTable[ThirtySecond];
    m_noteWidthTable[ThirtySecondDotted] = m_noteWidthTable[ThirtySecond] + m_noteWidthTable[SixtyFourth];
    m_noteWidthTable[SixtyFourthDotted]  = m_noteWidthTable[SixtyFourth]  + m_noteWidthTable[SixtyFourth] / 2;

}

// const vector<unsigned int>&
// NotationHLayout::splitNote(unsigned int noteLen)
// {
//     static vector<unsigned int> notes;

//     notes.clear();

//     unsigned int timeUnitsLeftInThisBar = m_timeUnitsPerBar - m_previousNbTimeUnitsInCurrentBar,
//         timeUnitsLeftInNote = m_nbTimeUnitsInCurrentBar - m_timeUnitsPerBar;

//     unsigned int nbWholeNotes = timeUnitsLeftInNote / m_quantizer.wholeNoteDuration();
    
//     // beginning of the note - what fills up the bar
//     notes.push_back(timeUnitsLeftInThisBar);

//     // the whole notes (if any)
//     for (unsigned int i = 0; i < nbWholeNotes; ++i) {
//         notes.push_back(Whole);
//         timeUnitsLeftInNote -= m_timeUnitsPerBar;
//     }
    
//     notes.push_back(timeUnitsLeftInNote);

//     m_nbTimeUnitsInCurrentBar = timeUnitsLeftInNote;

//     return notes;
    
// }

void
NotationHLayout::addNewBar(unsigned int barPos)
{
    m_barPositions.push_back(barPos);
    kdDebug(KDEBUG_AREA) << "NotationHLayout::addNewBar(" << barPos << ") - size : "
                         << m_barPositions.size() << "\n";

}

NotationHLayout::barpositions&
NotationHLayout::barPositions()
{
    return m_barPositions;
}

const NotationHLayout::barpositions&
NotationHLayout::barPositions() const
{
    return m_barPositions;
}
