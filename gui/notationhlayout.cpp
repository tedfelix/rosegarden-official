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
      m_previousAbsoluteTime(0),
      m_previousPos(barMargin),
      m_currentPos(barMargin),
      m_noteWidthTable(LastNote),
      m_currentScale(new Scale(Scale::C))
{
    initNoteWidthTable();
    m_timeUnitsPerBar = m_quantizer.wholeNoteDuration();
    kdDebug(KDEBUG_AREA) << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    delete m_currentScale;
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

NotationElementList::iterator
NotationHLayout::getPreviousNote(NotationElementList::iterator pos)
{
    NotationElementList::iterator prevNote = pos;

    --prevNote;
    
    while ((*prevNote)->event()->type() != "note" &&
           prevNote != m_notationElements.begin()) {
        --prevNote;
    }
    
    return prevNote;
}


Scale::KeySignature
NotationHLayout::getKeyAtPos(NotationElementList::iterator pos)
{
    while ((*pos)->event()->type() != "keychange" &&
           pos != m_notationElements.begin()) {
        --pos;
    }

    if (pos == m_notationElements.begin()) return Scale::C;

    try {
        Scale::KeySignature res = Scale::KeySignature((*pos)->event()->get<Int>("key"));
        return res;
    } catch (Event::NoData) {
        return Scale::C;
    }
}

void
NotationHLayout::layout(NotationElementList::iterator from,
                        NotationElementList::iterator to)
{
    // Adjust current pos according to where we are in the NotationElementList
    //
    if (from == m_notationElements.begin()) {

        setCurrentKey(Scale::C); // TODO add document-wide scale
        
        m_currentPos = m_barMargin; // we are at the beginning of the elements
        m_nbTimeUnitsInCurrentBar = 0;

    } else {

        // we're somewhere further - compute our position by looking
        // for the previous note

        NotationElementList::iterator oneBeforeFrom = getPreviousNote(from);

        NotationElement *elementBeforeFrom = (*oneBeforeFrom);

        m_quantizer.quantize(elementBeforeFrom->event());
        Note previousNote = Note(elementBeforeFrom->event()->get<Int>("Notation::NoteType"));
        m_currentPos = elementBeforeFrom->x();
        m_currentPos += m_noteWidthTable[previousNote] + Staff::noteWidth + m_noteMargin;

        if (m_currentScale->noteIsDecorated(*elementBeforeFrom)) {
            m_currentPos += Staff::accidentWidth;
        }

        m_nbTimeUnitsInCurrentBar = barTimeAtPos(oneBeforeFrom);

        setCurrentKey(getKeyAtPos(from));
    }
    
    m_barPositions.clear();

    // Now layout notes of the given interval
    //
    for (NotationElementList::iterator it = from; it != to; ++it) {
        
        NotationElement *nel = (*it);
    
        // Key Change
        //
        if (nel->event()->type() == "keychange") {
            try {
                Scale::KeySignature key = Scale::KeySignature(nel->event()->get<Int>("key"));
                setCurrentKey(key);
                nel->setX(m_currentPos);

                m_currentPos += 24 + m_noteMargin; // TODO
                m_previousPos = m_currentPos; // yes, this is correct

                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : got a keychange event - moving + 24"
                                     << endl;
                
            } catch (Event::NoData) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : got a keychange event with no key property"
                                     << endl;
            }
            continue;
        }

        // if (nel) is time sig change, reflect that

        // kdDebug(KDEBUG_AREA) << "Layout" << endl;

        //
        // layout event
        //
        m_quantizer.quantize(nel->event());
        
        // kdDebug(KDEBUG_AREA) << "Quantized" << endl;

        // Add note to current bar
        m_previousNbTimeUnitsInCurrentBar = m_nbTimeUnitsInCurrentBar;
        m_nbTimeUnitsInCurrentBar += nel->event()->get<Int>("QuantizedDuration");

        if (nel->absoluteTime() > m_previousAbsoluteTime ||
            it == from) {

            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : moving from "
                                 << m_previousAbsoluteTime
                                 << " to " << nel->absoluteTime()
                                 << endl;

            nel->setX(m_currentPos);

            // check the property is here ?
            Note note = Note(nel->event()->get<Int>("Notation::NoteType"));

            // Move current pos to next note
            m_previousPos = m_currentPos;
            m_currentPos += m_noteWidthTable[note] + Staff::noteWidth + m_noteMargin;

            if (m_currentScale->noteIsDecorated(*nel)) {
                nel->event()->set<Int>("Notation::Accident",
                                       m_currentScale->useSharps() ? Sharp : Flat);
                m_currentPos += Staff::accidentWidth;
            } else {
                nel->event()->set<Int>("Notation::Accident", NoAccidental);
            }
            


            // See if we've completed a bar
            //
            if (m_nbTimeUnitsInCurrentBar > m_timeUnitsPerBar) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : Bar has wrong length" << endl;
                // TODO
            } else if (m_nbTimeUnitsInCurrentBar == m_timeUnitsPerBar) {
                m_nbTimeUnitsInCurrentBar = 0;
                addNewBar(m_currentPos + m_noteMargin);
                m_currentPos += 2 * m_noteMargin + Staff::noteWidth;
            }


        } else {
            
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : staying at "
                                 << m_previousAbsoluteTime
                                 << endl;

            nel->setX(m_previousPos);

            if (nel->absoluteTime() < m_previousAbsoluteTime) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : sanity problem - event absolute time is before previous event's time" << endl;
            }
            
        }
        

        m_previousAbsoluteTime = nel->absoluteTime();

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
    setCurrentKey(Scale::C);
}

void
NotationHLayout::setCurrentKey(Scale::KeySignature key)
{
    delete m_currentScale;
    m_currentScale = new Scale(key);
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
