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
      m_noteWidthMap(LastNote)
{
    initNoteWidthTable();
    m_timeUnitsPerBar = m_quantizer.wholeNoteDuration();
}

void
NotationHLayout::layout(Event *el)
{
    // if (el) is time sig change, reflect that

    // kdDebug(KDEBUG_AREA) << "Layout" << endl;

    m_quantizer.quantize( el );

    // kdDebug(KDEBUG_AREA) << "Quantized" << endl;

    // Add note to current bar
    m_previousNbTimeUnitsInCurrentBar = m_nbTimeUnitsInCurrentBar;
    m_nbTimeUnitsInCurrentBar += el->get<Int>("QuantizedDuration");

    // kdDebug(KDEBUG_AREA) << "m_nbTimeUnitsInCurrentBar : " << m_nbTimeUnitsInCurrentBar << endl;

    if (m_nbTimeUnitsInCurrentBar > m_timeUnitsPerBar) {
        kdDebug(KDEBUG_AREA) << "split element" << endl;
        // TODO
    } else if (m_nbTimeUnitsInCurrentBar == m_timeUnitsPerBar) {
        kdDebug(KDEBUG_AREA) << "start a new bar" << endl;
        // TODO
        m_nbTimeUnitsInCurrentBar = 0;
    }    

    kdDebug(KDEBUG_AREA) << "set to m_currentPos = " << m_currentPos << endl;
    el->set<Int>("Notation::X", m_currentPos);

    Note note = Note(el->get<Int>("Notation::NoteType")); // check the property is here ?

    // Move current pos to next note
    m_currentPos += m_noteWidthMap[note] + Staff::noteWidth + m_noteMargin;

    kdDebug(KDEBUG_AREA) << "m_currentPos pushed to = " << m_currentPos << endl;
}

void
NotationHLayout::initNoteWidthTable(void)
{
    m_noteWidthMap[Whole]        = m_barWidth;
    m_noteWidthMap[Half]         = m_barWidth / 2;
    m_noteWidthMap[Quarter]      = m_barWidth / 4;
    m_noteWidthMap[Eighth]       = m_barWidth / 8;
    m_noteWidthMap[Sixteenth]    = m_barWidth / 16;
    m_noteWidthMap[ThirtySecond] = m_barWidth / 32;
    m_noteWidthMap[SixtyFourth]  = m_barWidth / 64;

    m_noteWidthMap[WholeDotted]        = m_noteWidthMap[Whole]        + m_noteWidthMap[Half];
    m_noteWidthMap[HalfDotted]         = m_noteWidthMap[Half]         + m_noteWidthMap[Quarter];
    m_noteWidthMap[QuarterDotted]      = m_noteWidthMap[Quarter]      + m_noteWidthMap[Eighth];
    m_noteWidthMap[EighthDotted]       = m_noteWidthMap[Eighth]       + m_noteWidthMap[Sixteenth];
    m_noteWidthMap[SixteenthDotted]    = m_noteWidthMap[Sixteenth]    + m_noteWidthMap[ThirtySecond];
    m_noteWidthMap[ThirtySecondDotted] = m_noteWidthMap[ThirtySecond] + m_noteWidthMap[SixtyFourth];
    m_noteWidthMap[SixtyFourthDotted]  = m_noteWidthMap[SixtyFourth]  + m_noteWidthMap[SixtyFourth] / 2;

}

const vector<unsigned int>&
NotationHLayout::splitNote(unsigned int noteLen)
{
    static vector<unsigned int> notes;

    notes.clear();

    unsigned int timeUnitsLeftInThisBar = m_timeUnitsPerBar - m_previousNbTimeUnitsInCurrentBar,
        timeUnitsLeftInNote = m_nbTimeUnitsInCurrentBar - m_timeUnitsPerBar;

    unsigned int nbWholeNotes = timeUnitsLeftInNote / m_quantizer.wholeNoteDuration();
    
    // beginning of the note - what fills up the bar
    notes.push_back(timeUnitsLeftInThisBar);

    // the whole notes (if any)
    for (unsigned int i = 0; i < nbWholeNotes; ++i) {
        notes.push_back(Whole);
        timeUnitsLeftInNote -= m_timeUnitsPerBar;
    }
    
    notes.push_back(timeUnitsLeftInNote);

    m_nbTimeUnitsInCurrentBar = timeUnitsLeftInNote;

    return notes;
    
//     $nbBeatsInThisBar = $nbBeatsPerBar - $previousNbBeats;
//     $beatsLeftInNote =  $nbBeats - $nbBeatsPerBar;

//     print "$nbBeatsInThisBar | ";

//     $nbWholeNotes = $beatsLeftInNote / $nbBeatsPerBar;
//     # print "\n nbWholeNotes : $nbWholeNotes\n";

//     for ($i = 0; $i < $nbWholeNotes; ++$i) {
//       print "$nbBeatsPerBar | ";
//       $beatsLeftInNote -= $nbBeatsPerBar;
//     }

//     print "$beatsLeftInNote ";
//     $nbBeats = $beatsLeftInNote;
    
}
