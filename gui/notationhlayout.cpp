
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "staff.h"
#include "notationhlayout.h"
#include "rosedebug.h"
#include "NotationTypes.h"



NotationHLayout::NotationHLayout(Staff &staff, //!!! maybe not needed, just trying to build up consistent interfaces for h & v layout
                                 NotationElementList& elements,
                                 unsigned int barWidth,
                                 unsigned int barMargin,
                                 unsigned int noteMargin)
    : m_notationElements(elements),
      m_barWidth(barWidth),
      m_barMargin(barMargin),
      m_noteMargin(noteMargin),
      m_nbTimeUnitsInCurrentBar(0),
      m_previousAbsoluteTime(0),
      m_timeSignature(TimeSignature::DefaultTimeSignature)
{

    //!!! ask the time signature...
//    m_timeUnitsPerBar = Note(Note::WholeNote).getDuration();
    kdDebug(KDEBUG_AREA) << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    // empty
}


unsigned int
NotationHLayout::barTimeAtPos(NotationElementList::iterator pos)
{
    //!!!

    unsigned int res = 0;

    for (NotationElementList::iterator it = m_notationElements.begin();
         it != pos; ++it)
        res += (*it)->event()->get<Int>("QuantizedDuration");

    //!!! no -- wholly wrong when the time signature changes during the piece
    // really we should not be having to call a method like this at all
    return res % m_timeSignature.getBarDuration();
}

NotationElementList::iterator
NotationHLayout::getPreviousNote(NotationElementList::iterator pos)
{
    return m_notationElements.findPrevious
        (Note::EventPackage, Note::EventType, pos);
}

void
NotationHLayout::layout(NotationElementList::iterator from,
                        NotationElementList::iterator to)
{
    double x = m_barMargin;
    m_timeSignature = TimeSignature::DefaultTimeSignature;
    m_nbTimeUnitsInCurrentBar = 0;

    if (from != m_notationElements.begin()) {

        // Adjust according to where we are in the NotationElementList

        NotationElementList::iterator prevTS = m_notationElements.findPrevious
            (TimeSignature::EventPackage, TimeSignature::EventType, from);
        if (prevTS != m_notationElements.end()) {
            m_timeSignature = TimeSignature(*(*prevTS)->event());
        }

        // we're somewhere further - compute our position by looking
        // for the previous note

        NotationElementList::iterator oneBeforeFrom = getPreviousNote(from);
        NotationElement *elementBeforeFrom = (*oneBeforeFrom);

        m_quantizer.quantize(elementBeforeFrom->event());

        Note::Type previousNote = elementBeforeFrom->event()->get<Int>("Notation::NoteType");
        bool previousDotted = elementBeforeFrom->event()->get<Bool>("Notation::NoteDotted");
        x = elementBeforeFrom->x();
        x += getNoteWidth(previousNote, previousDotted) +
            Staff::noteWidth + m_noteMargin;

//!!! do this right
/*!        if (m_currentScale->noteIsDecorated(*elementBeforeFrom)) {
            m_currentPos += Staff::accidentWidth;
        }
*/
        //!!! no, shouldn't be doing this
        m_nbTimeUnitsInCurrentBar = barTimeAtPos(oneBeforeFrom);

/*!        setCurrentKey(getKeyAtPos(from)); */
    }

    //!!! ugh, this sort of thing sounds like it'll interact badly
    //with attempts to layout only parts of a staff
    m_barPositions.clear();

    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): starting, initial x is " << x << endl;

    // Now layout notes of the given interval
    //
    for (NotationElementList::iterator it = from; it != to; ++it) {
        
        NotationElement *nel = (*it);
        nel->setX(x);

        kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): setting element's x to " << x << endl;

        if (nel->event()->isa(Key::EventPackage, Key::EventType)) {

            try {
                Key key(nel->event()->get<String>("key"));
                x += 24 + m_noteMargin; //!!! TODO

                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : got a keychange event - moving + 24"
                                     << endl;
                
            } catch (Event::NoData) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : got a keychange event with no key property"
                                     << endl;
            }

        } else if ((*it)->event()->isa(Clef::EventPackage, Clef::EventType)) {

            x += 24 + m_noteMargin; //!!! fix

            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : got a clef event - moving + 24"
                                 << endl;

        } else if (nel->event()->isa(TimeSignature::EventPackage,
                                     TimeSignature::EventType)) {

            x += 24 + m_noteMargin; //!!! fix

            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : got a timesig event - moving + 24"
                                 << endl;

            m_timeSignature = TimeSignature(*nel->event());
            //... and put in a new barline &c

        } else if (nel->isNote() || nel->isRest()) {

            // layout event
            //
            m_quantizer.quantize(nel->event());
        
            // find out if we have a chord; if not, move on

            NotationElementList::iterator ni(it);

            if (it == to || !nel->isNote() ||
                (++ni) == to || !(*ni)->isNote() ||
                (*ni)->absoluteTime() != nel->absoluteTime()) {

                // okay, we aren't a note being followed by a note with the
                // same absolute time... so don't hang back

                // Add note to current bar
                //!!! may be wrong for chords whose notes differ in duration
                m_nbTimeUnitsInCurrentBar += nel->event()->get<Int>("QuantizedDuration");

                // check the property is here ?
                Note::Type note = nel->event()->get<Int>("Notation::NoteType");
                bool dotted = nel->event()->get<Bool>("Notation::NoteDotted");

                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : moving from "
                                     << x << "..." << endl;

                x += getNoteWidth(note, dotted) +
                    Staff::noteWidth + m_noteMargin;

                kdDebug(KDEBUG_AREA) << " to " << x << endl;

                try {
                    Accidental a = (Accidental)nel->event()->get<Int>
                        ("Notation::Accidental");
                    if (a != NoAccidental) x += Staff::accidentWidth;
                } catch (Event::NoData) {
                    // not a problem
                }

/*!
  if (m_currentScale->noteIsDecorated(*nel)) {
  //!!! now in notationvlayout -- look for computed-accidental
  nel->event()->set<Int>("Notation::Accident",
  m_currentScale->useSharps() ? Sharp : Flat);
  m_currentPos += Staff::accidentWidth;
  } else {
  nel->event()->set<Int>("Notation::Accident", NoAccidental);
  }
*/         
            }

            // See if we've completed a bar
            //
            int barDuration = m_timeSignature.getBarDuration();
            if (m_nbTimeUnitsInCurrentBar > barDuration) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : Bar has wrong length" << endl;
                //!!! TODO
            } else if (m_nbTimeUnitsInCurrentBar == barDuration) {
                m_nbTimeUnitsInCurrentBar = 0;
                addNewBar(x + m_noteMargin);
                x += 2 * m_noteMargin + Staff::noteWidth;
            }

            if (nel->absoluteTime() < m_previousAbsoluteTime) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : sanity problem - event absolute time is before previous event's time" << endl;
            }

            m_previousAbsoluteTime = nel->absoluteTime();
        }
    }
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
    m_nbTimeUnitsInCurrentBar = 0;
    m_barPositions.clear();
}


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
