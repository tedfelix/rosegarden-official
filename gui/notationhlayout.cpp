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

#include <qcolor.h>

#include "staff.h"
#include "notationhlayout.h"
#include "rosedebug.h"

NotationHLayout::NotationHLayout(NotationElementList& elements,
                                 unsigned int barWidth,
                                 unsigned int beatsPerBar,
                                 unsigned int barMargin,
                                 unsigned int noteMargin)
    : m_notationElements(elements),
      m_barWidth(barWidth),
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

unsigned int
NotationHLayout::barTimeAtPos(NotationElementList::iterator pos)
{
    unsigned int res = 0;

    for (NotationElementList::iterator it = m_notationElements.begin();
         it != pos; ++it)
        res += (*it)->event()->get<Int>("QuantizedDuration");

    return res % m_timeUnitsPerBar;
}


void
NotationHLayout::layout(NotationElementList::iterator from, NotationElementList::iterator to)
{
    // Adjust current pos according to where we are in the NotationElementList
    //
    if (from == m_notationElements.begin()) {
        m_currentPos = m_barMargin; // we are at the beginning of the elements
        m_nbTimeUnitsInCurrentBar = 0;
    } else {

        NotationElementList::iterator oneBeforeFrom = from;
        --oneBeforeFrom;
        NotationElement *elementBeforeFrom = (*oneBeforeFrom);
        // Make sure the event has a note
        m_quantizer.quantize(elementBeforeFrom->event());
        Note previousNote = Note(elementBeforeFrom->event()->get<Int>("Notation::NoteType"));
        m_currentPos = elementBeforeFrom->x();
        m_currentPos += m_noteWidthTable[previousNote] + Staff::noteWidth + m_noteMargin;

        m_nbTimeUnitsInCurrentBar = barTimeAtPos(oneBeforeFrom);

    }
    
    m_barPositions.clear();

    for (NotationElementList::iterator it = from; it != to; ++it) {
        
        NotationElement *nel = (*it);
    
        // if (nel) is time sig change, reflect that

        // kdDebug(KDEBUG_AREA) << "Layout" << endl;

        if (nel->isGroup()) {
            //
            // layout group
            //
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : layout group" << endl;
            NotationElementList *group = nel->group(); // TODO - how do I make this ?
            layoutGroup(group->begin(), group->end());

        } else {
            //
            // layout single event
            //
            m_quantizer.quantize(nel->event());
        
            // kdDebug(KDEBUG_AREA) << "Quantized" << endl;

            // Add note to current bar
            m_previousNbTimeUnitsInCurrentBar = m_nbTimeUnitsInCurrentBar;
            m_nbTimeUnitsInCurrentBar += nel->event()->get<Int>("QuantizedDuration");

            nel->setX(m_currentPos);

            Note note = Note(nel->event()->get<Int>("Notation::NoteType")); // check the property is here ?

            // Move current pos to next note
            m_currentPos += m_noteWidthTable[note] + Staff::noteWidth + m_noteMargin;

            // See if we've completed a bar
            //
            if (m_nbTimeUnitsInCurrentBar > m_timeUnitsPerBar) {
                kdDebug(KDEBUG_AREA) << "Bar has wrong length" << endl;
                // TODO
            } else if (m_nbTimeUnitsInCurrentBar == m_timeUnitsPerBar) {
                m_nbTimeUnitsInCurrentBar = 0;
                addNewBar(m_currentPos + m_noteMargin);
                m_currentPos += 2 * m_noteMargin + Staff::noteWidth;
            }
        }
    }
    
}

void
NotationHLayout::layoutGroup(NotationElementList::iterator from, NotationElementList::iterator to)
{
    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : layout group to pos "
                         << m_currentPos << endl;

    for (NotationElementList::iterator it = from; it != to; ++it) {
        
        NotationElement *el = (*it);

        m_quantizer.quantize(el->event());
        el->setX(m_currentPos);
    }

    NotationElement *firstNoteOfGroup = (*from);

    Note note = Note(firstNoteOfGroup->event()->get<Int>("Notation::NoteType"));
    m_currentPos += m_noteWidthTable[note] + Staff::noteWidth + m_noteMargin;

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

void
NotationHLayout::reset()
{
    m_currentPos = m_barMargin;
    m_nbTimeUnitsInCurrentBar = 0;
    m_previousNbTimeUnitsInCurrentBar = 0;
    m_barPositions.clear();
}

NotationElementList::iterator
NotationHLayout::findClosestNote(NotationElementList::iterator insertPoint,
                                 bool &noteIsRest)
{
    NotationElementList::iterator nextEl = insertPoint,
        prevEl = insertPoint, result = insertPoint;

    ++nextEl;

    if (prevEl != m_notationElements.begin()) --prevEl;

    double distToNext = 10e9,
        distToPrevious = 10e9;
    
    // Compute dist. to next note
    if (nextEl != m_notationElements.end())
        distToNext = (*nextEl)->x() - (*insertPoint)->x();

    if (prevEl != m_notationElements.begin())
        distToPrevious = (*insertPoint)->x() - (*prevEl)->x();

    if (distToPrevious > distToNext)
        result = nextEl;
    else
        result = prevEl;

    noteIsRest = ((*result)->isRest());

    return result;
}


NotationElementList::iterator
NotationHLayout::insertNote(NotationElement *el)
{
//     ElementHPos elementPos(xPos);
    
    kdDebug(KDEBUG_AREA) << "insertNote : approx. x : " << el->x() << endl;

    NotationElementList::iterator insertPoint = m_notationElements.insert(el);

    bool noteIsRest = false;

    NotationElementList::iterator closestNote = findClosestNote(insertPoint, noteIsRest);

    if (closestNote != m_notationElements.end()) {

        el->setX((*closestNote)->x());

        if (noteIsRest) {} // replace rest (or part of it) with note
        // else don't do anything special, just put note where it is

    } else {
        --closestNote;
        el->setX((*closestNote)->x() + Staff::noteWidth + m_noteMargin);
    }
    

    return insertPoint;
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

// bool compareNoteElement(NotationElement *el1, NotationElement *el2)
// {
//     kdDebug(KDEBUG_AREA) << "compareNoteElement : el1->x : " << el1->x()
//                          << "(item : " << el1->canvasItem()->x()
//                          << ") - el2->x : "<< el2->x() << endl;

//     // Nifty trick to show what items we're comparing with
//     //
// //     QCanvas *canvas = el1->canvasItem()->canvas();
    
// //     QCanvasLine *mark = new QCanvasLine(canvas);
// //     mark->setPoints(el1->canvasItem()->x(), 0, el1->canvasItem()->x(), el1->x());
// //     mark->show();

//     return el1->canvasItem()->x() < el2->x();
// }
