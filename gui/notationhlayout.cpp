
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
#include "notepixmapfactory.h"
#include "notationproperties.h"


NotationHLayout::NotationHLayout(Staff &staff, //!!! maybe not needed, just trying to build up consistent interfaces for h & v layout
                                 NotationElementList& elements) :
    m_staff(staff),
    m_notationElements(elements),
    m_barWidth(100), // for now
    m_barMargin(staff.getBarMargin()),
    m_noteMargin(staff.getNoteMargin())
{
    kdDebug(KDEBUG_AREA) << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    // empty
}

NotationElementList::iterator
NotationHLayout::getPreviousNote(NotationElementList::iterator pos)
{
    return m_notationElements.findPrevious
        (Note::EventPackage, Note::EventType, pos);
}


// locate the bars, and compute all the cached properties

void
NotationHLayout::preparse(NotationElementList::iterator from,
                          NotationElementList::iterator to)
{
    Key key;
    Clef clef;
    TimeSignature timeSignature;
    Event::timeT nbTimeUnitsInCurrentBar = 0;
    Event::timeT previousAbsoluteTime = 0;

    if (from != m_notationElements.begin()) {
        kdDebug(KDEBUG_AREA) << "NotationHLayout::preparse: from > m_notationElements.begin() case not handled yet, resetting from" << endl;
        from = m_notationElements.begin();
    }

    m_barPositions.clear();

    bool startOfBar = true;
    bool barCorrect = true;

    for (NotationElementList::iterator it = from; it != to; ++it) {
        
        NotationElement *el = (*it);
        if (startOfBar) {
            addNewBar(it, el->getAbsoluteTime(), -1, -1, true, barCorrect);
        }
        startOfBar = false;

        if (el->event()->isa(Clef::EventPackage, Clef::EventType)) {

            try {
                clef = Clef(*el->event());
            } catch (Event::NoData) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::preparse() : got a clef event with no clef property"
                                     << endl;
            }

        } else if (el->event()->isa(Key::EventPackage, Key::EventType)) {

            try {
                key = Key(*el->event());
            } catch (Event::NoData) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::preparse() : got a keychange event with no key property"
                                     << endl;
            }

        } else if (el->event()->isa(TimeSignature::EventPackage,
                                    TimeSignature::EventType)) {

            if (nbTimeUnitsInCurrentBar > 0) {
                // need to insert the bar line _before_ this event
                nbTimeUnitsInCurrentBar = 0;
                addNewBar(it, el->getAbsoluteTime(), -1, -1, true, true);
            }

            timeSignature = TimeSignature(*el->event());

        } else if (el->isNote() || el->isRest()) {

            m_quantizer.quantize(el->event());

            if (el->isNote()) {

                try {
                    int pitch = el->event()->get<Int>("pitch");
                    kdDebug(KDEBUG_AREA) << "pitch : " << pitch << endl;
                    NotationDisplayPitch p(pitch, clef, key);
                    int h = p.getHeightOnStaff();
                    el->event()->set<Int>(P_HEIGHT_ON_STAFF, h);
                    el->event()->set<Int>(P_ACCIDENTAL,(int)p.getAccidental());
                    el->event()->set<Bool>(P_STALK_UP, h <= 4); 
                } catch (Event::NoData) {
                    kdDebug(KDEBUG_AREA) <<
                        "NotationHLayout::preparse: couldn't get pitch for element"
                                         << endl;
                }
            }
        
            // find out if we have a chord; if not, move on

            NotationElementList::iterator ni(it);

            if (it == to || !el->isNote() ||
                (++ni) == to || !(*ni)->isNote() ||
                (*ni)->getAbsoluteTime() != el->getAbsoluteTime()) {

                // okay, we aren't a note being followed by a note with the
                // same absolute time... so don't hang back

                // Add note to current bar
                //!!! may be wrong for chords whose notes differ in duration
                nbTimeUnitsInCurrentBar +=
                    el->event()->get<Int>("Cache::QuantizedDuration");
            }

            // See if we've completed a bar

            Event::timeT barDuration = timeSignature.getBarDuration();

            if (nbTimeUnitsInCurrentBar >= barDuration) {
                barCorrect = (nbTimeUnitsInCurrentBar == barDuration);
                nbTimeUnitsInCurrentBar = 0;
                startOfBar = true;
            }

            if (el->getAbsoluteTime() < previousAbsoluteTime) {
                kdDebug(KDEBUG_AREA) << "NotationHLayout::preparse() : sanity problem - event absolute time is before previous event's time" << endl;
            }
            
            previousAbsoluteTime = el->getAbsoluteTime();
        }
    }
}


void
NotationHLayout::layout()
{
    double x = 0;
    const NotePixmapFactory &npf(m_staff.getNotePixmapFactory());

    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): starting, initial x is " << x << endl;

    for (BarPositions::iterator bpi = m_barPositions.begin();
         bpi != m_barPositions.end(); ++bpi) {

        NotationElementList::iterator from = bpi->start;
        NotationElementList::iterator to;
        BarPositions::iterator nbpi(bpi);
        if (++nbpi == m_barPositions.end()) {
            to = m_notationElements.end();
        } else {
            to = nbpi->start;
        }

        bpi->x = (int)(x + m_barMargin / 2);
        x += m_barMargin;

        for (NotationElementList::iterator it = from; it != to; ++it) {
            
            NotationElement *nel = (*it);
            nel->setX(x);

            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): setting element's x to " << x << endl;

            if (nel->event()->isa(Key::EventPackage, Key::EventType)) {

                try {
                    Key key(*nel->event());
                    x += (npf.getAccidentalWidth() - (key.isSharp() ? 1 : 2)) *
                        key.getAccidentalCount() + m_noteMargin;
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

            } else if (nel->isNote() || nel->isRest()) {

                //!!! need a method for locating and collating chords
                // find out if we have a chord; if not, move on

                NotationElementList::iterator ni(it);

                if (it == to || !nel->isNote() ||
                    (++ni) == to || !(*ni)->isNote() ||
                    (*ni)->getAbsoluteTime() != nel->getAbsoluteTime()) {

                    // okay, we aren't a note being followed by a note with the
                    // same absolute time... so don't hang back

                    // check the property is here ?
                    Note::Type note = nel->event()->get<Int>(P_NOTE_TYPE);
                    bool dotted = nel->event()->get<Bool>(P_NOTE_DOTTED);

                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout() : moving from "
                                         << x << "..." << endl;

                    x += getNoteWidth(note, dotted) +
                        npf.getNoteBodyWidth() + m_noteMargin;

                    kdDebug(KDEBUG_AREA) << " to " << x << endl;

                    Accidental a = NoAccidental;
                    long tmp;
                    
                    if (nel->event()->get<Int>(P_ACCIDENTAL, tmp)) {
                        a = Accidental(tmp);
                        if (a != NoAccidental) x += npf.getAccidentalWidth();
                    }

                }
            }
        }
    }
}

void
NotationHLayout::addNewBar(NotationElementList::iterator start,
                           Event::timeT time, int x, int width,
                           bool fixed, bool correct)
{
    m_barPositions.push_back(BarPosition(start, time, x, width, fixed, correct));
    kdDebug(KDEBUG_AREA) << "NotationHLayout::addNewBar(" << x << ") - size : "
                         << m_barPositions.size() << "\n";
}

NotationHLayout::BarPositions&
NotationHLayout::getBarPositions()
{
    return m_barPositions;
}

const NotationHLayout::BarPositions&
NotationHLayout::getBarPositions() const
{
    return m_barPositions;
}

void
NotationHLayout::reset()
{
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


