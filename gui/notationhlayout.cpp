
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
#include "notationsets.h"

using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::NoAccidental;
using Rosegarden::Note;
using Rosegarden::TimeSignature;


NotationHLayout::NotationHLayout(Staff &staff, //!!! maybe not needed, just trying to build up consistent interfaces for h & v layout
                                 NotationElementList& elements) :
    m_staff(staff),
    m_notationElements(elements),
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
    return m_notationElements.findPrevious(Note::EventType, pos);
}


// To find the "ideal" width of a bar, we need the sum of the minimum
// widths of the elements in the bar, plus as much extra space as
// would be needed if the bar was made up entirely of repetitions of
// its shortest note.  This space is the product of the number of the
// bar's shortest notes that will fit in the duration of the bar and
// the comfortable gap for each.

int NotationHLayout::getIdealBarWidth(int fixedWidth,
                                      NotationElementList::iterator shortest,
                                      const NotePixmapFactory &npf,
                                      int shortCount,
                                      const TimeSignature &timeSignature) const
{
    if (shortest == m_notationElements.end()) return fixedWidth;
    int d = (*shortest)->event()->getDuration();
    if (d == 0) return fixedWidth;

    int smin = getMinWidth(npf, **shortest);
    if (!(*shortest)->event()->get<Bool>(P_NOTE_DOTTED)) {
        smin += npf.getDotWidth()/2;
    }

    /* if there aren't many of the shortest notes, we don't want to
       allow so much space to accommodate them */
    if (shortCount < 3) smin -= 3 - shortCount;

    int gapPer = 
        getComfortableGap(npf, (*shortest)->event()->get<Int>(P_NOTE_TYPE)) +
        smin;

    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: d is "
                         << d << ", shortCount is " << shortCount
                         << ", gapPer is " << gapPer << ", fixedWidth is "
                         << fixedWidth << ", barDuration is "
                         << timeSignature.getBarDuration() << endl;

    int w = fixedWidth + timeSignature.getBarDuration() * gapPer / d;

    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: returning "
                         << w << endl;
    return w;
} 

// Locate the bars, and compute all the cached properties.

void
NotationHLayout::preparse(NotationElementList::iterator from,
                          NotationElementList::iterator to)
{
    Key key;
    Clef clef;
    TimeSignature timeSignature;
    Event::timeT nbTimeUnitsInCurrentBar = 0;
    const NotePixmapFactory &npf(m_staff.getNotePixmapFactory());

    if (from != m_notationElements.begin()) {
        kdDebug(KDEBUG_AREA) << "NotationHLayout::preparse: from > m_notationElements.begin() case not handled yet, resetting from" << endl;
        from = m_notationElements.begin();
    }

    m_barPositions.clear();
    if (from == m_notationElements.end()) return;

    bool startNewBar = true;
    bool barCorrect = true;
    int fixedWidth = m_barMargin;

    NotationElementList::iterator shortest = m_notationElements.end();
    int shortCount = 0;
    NotationElementList::iterator it = from;
    Event::timeT absoluteTime = 0;

    for ( ; it != to; ++it) {
        
        NotationElement *el = (*it);
        absoluteTime = el->getAbsoluteTime();

        if (startNewBar) {
            addNewBar(it, absoluteTime, -1,
                      getIdealBarWidth(fixedWidth, shortest, npf,
                                       shortCount, timeSignature),
                      fixedWidth, true, barCorrect);
            fixedWidth = m_barMargin;
            shortest = m_notationElements.end();
            shortCount = 1;
        }
        startNewBar = false;

        if (el->isNote() || el->isRest()) m_quantizer.quantize(el->event());
        int mw = getMinWidth(npf, *el);
        el->event()->setMaybe<Int>(P_MIN_WIDTH, mw);

        if (el->event()->isa(Clef::EventType)) {

            fixedWidth += mw;
            clef = Clef(*el->event());

        } else if (el->event()->isa(Key::EventType)) {

            fixedWidth += mw;
            key = Key(*el->event());

        } else if (el->event()->isa(TimeSignature::EventType)) {

            if (nbTimeUnitsInCurrentBar > 0) {
                // need to insert the bar line before this event...
                nbTimeUnitsInCurrentBar = 0;

                // ...and also before any preceding clef or key events
                NotationElementList::iterator it0(it), it1(it);
                while (it0 == it ||
                       (*it0)->event()->isa(Clef::EventType) ||
                       (*it0)->event()->isa(Key::EventType)) {
                    it1 = it0;
                    // this shouldn't happen, as we've checked there
                    // are already some time units in the bar, but
                    // it's better to be safe than sorry:
                    if (it0 == m_notationElements.begin()) break;
                    --it0;
                }

                addNewBar(it1, absoluteTime, -1,
                          getIdealBarWidth(fixedWidth, shortest, npf,
                                           shortCount, timeSignature),
                          fixedWidth, true, true);
                fixedWidth = m_barMargin + mw;
                shortest = m_notationElements.end();
                shortCount = 1;
            }

            timeSignature = TimeSignature(*el->event());

        } else if (el->isNote() || el->isRest()) {

            if (el->isNote()) {

                try {
                    int pitch = el->event()->get<Int>("pitch");
                    kdDebug(KDEBUG_AREA) << "pitch : " << pitch << endl;
                    Rosegarden::NotationDisplayPitch p(pitch, clef, key);
                    int h = p.getHeightOnStaff();
                    el->event()->setMaybe<Int>(P_HEIGHT_ON_STAFF, h);
                    el->event()->setMaybe<Int>(P_ACCIDENTAL,
                                               (int)p.getAccidental());
                    el->event()->setMaybe<String>(P_NOTE_NAME, p.getAsString
                                                  (clef, key));
                } catch (Event::NoData) {
                    kdDebug(KDEBUG_AREA) <<
                        "NotationHLayout::preparse: couldn't get pitch for element"
                                         << endl;
                }

                // if we're in a chord, deal appropriately

                Chord chord(m_notationElements, it);
                if (chord.size() < 2 || it == chord.getFinalElement()) {

                    int d = el->event()->get<Int>(P_QUANTIZED_DURATION); 
                    nbTimeUnitsInCurrentBar += d;

                    int sd = 0;
                    if (shortest == m_notationElements.end() ||
                        d <= (sd = (*shortest)->event()->get<Int>
                              (P_QUANTIZED_DURATION))) {
                        if (d == sd) ++shortCount;
                        else {
                            kdDebug(KDEBUG_AREA) << "New shortest! Duration is " << d << " (at " << nbTimeUnitsInCurrentBar << " time units)"<< endl;
                            shortest = it;
                            shortCount = 1;
                        }
                    }
                }
            }

            // See if we've completed a bar

            Event::timeT barDuration = timeSignature.getBarDuration();

            if (nbTimeUnitsInCurrentBar >= barDuration) {
                barCorrect = (nbTimeUnitsInCurrentBar == barDuration);
                nbTimeUnitsInCurrentBar = 0;
                startNewBar = true;
            }
        }
    }

    if (nbTimeUnitsInCurrentBar > 0) {
        addNewBar(it, absoluteTime, -1,
                  getIdealBarWidth(fixedWidth, shortest, npf,
                                   shortCount, timeSignature),
                  fixedWidth, false,
                  (nbTimeUnitsInCurrentBar == timeSignature.getBarDuration()));
    }
}


void
NotationHLayout::layout()
{
    Key key;
    Clef clef;
    double x = 0;
    const NotePixmapFactory &npf(m_staff.getNotePixmapFactory());
    TimeSignature timeSignature;

    int pGroupNo = -1;
    NotationElementList::iterator startOfGroup = m_notationElements.end();

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

        kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): starting a bar, initial x is " << x << " and barWidth is " << bpi->idealWidth << endl;


        bpi->x = (int)(x + m_barMargin / 2);
        x += m_barMargin;

        for (NotationElementList::iterator it = from; it != to; ++it) {
            
            NotationElement *el = (*it);
            el->setLayoutX(x);
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): setting element's x to " << x << endl;

            long delta = 0;
            (void)el->event()->get<Int>(P_MIN_WIDTH, delta);

            if (el->event()->isa(TimeSignature::EventType)) {

                timeSignature = TimeSignature(*el->event());

            } else if (el->event()->isa(Clef::EventType)) {

                clef = Clef(*el->event());

            } else if (el->event()->isa(Key::EventType)) {

                key = Key(*el->event());

            } else if (el->isRest()) {

                // To work out how much space to allot a note (or
                // chord), start with the amount alloted to the
                // whole bar, subtract that reserved for
                // fixed-width items, and take the same proportion
                // of the remainder as our duration is of the
                // whole bar's duration.
                
                delta = ((bpi->idealWidth - bpi->fixedWidth) *
                         el->event()->getDuration()) /
                    //!!! not right for partial bar?
                    timeSignature.getBarDuration();

            } else if (el->isNote()) {
                
                long groupNo = -1;

                if (el->event()->get<Int>(P_GROUP_NO, groupNo) &&
                    groupNo != pGroupNo) {
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: entering group " << groupNo << endl;
                    startOfGroup = it;
                }

                long nextGroupNo = -1;
                NotationElementList::iterator it0(it);
                ++it0;
                if (groupNo > -1 &&
                    (it0 == m_notationElements.end() ||
                     (!(*it0)->event()->get<Int>(P_GROUP_NO, nextGroupNo) ||
                      nextGroupNo != groupNo))) {
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: about to leave group " << groupNo << ", time to do the sums" << endl;
                
                    NotationGroup group(m_notationElements, it, clef, key);
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: group type is " << group.getGroupType() << ", now applying beam" << endl;
                    group.applyBeam(m_staff);
                }

                pGroupNo = groupNo;

                Chord chord(m_notationElements, it);
                if (chord.size() < 2 || it == chord.getFinalElement()) {

		    kdDebug(KDEBUG_AREA) << "This is the final chord element (of " << chord.size() << ")" << endl;

                    // To work out how much space to allot a note (or
                    // chord), start with the amount alloted to the
                    // whole bar, subtract that reserved for
                    // fixed-width items, and take the same proportion
                    // of the remainder as our duration is of the
                    // whole bar's duration.
                    
                    delta = ((bpi->idealWidth - bpi->fixedWidth) *
                             el->event()->getDuration()) /
                        //!!! not right for partial bar?
                        timeSignature.getBarDuration();
                } else {
		    kdDebug(KDEBUG_AREA) << "This is not the final chord element (of " << chord.size() << ")" << endl;
		    delta = 0;
		}
            }

            x += delta;
        }
    }
}

void
NotationHLayout::addNewBar(NotationElementList::iterator start,
                           Event::timeT time, int x, int width, int fwidth,
                           bool fixed, bool correct)
{
    if (m_barPositions.size() > 0) {
        m_barPositions[m_barPositions.size()-1].idealWidth = width;
        m_barPositions[m_barPositions.size()-1].fixedWidth = fwidth;
    }
    m_barPositions.push_back(BarPosition(start, time, x, 150,
                                         150, fixed, correct));
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

int NotationHLayout::getMinWidth(const NotePixmapFactory &npf,
                                 const NotationElement &e) const
{
    int w = m_noteMargin;

    if (e.isNote() || e.isRest()) {

        w += npf.getNoteBodyWidth();
        bool dotted;
        if (e.event()->get<Bool>(P_NOTE_DOTTED, dotted) && dotted) {
            w += npf.getDotWidth();
        }
        long accidental;
        if (e.event()->get<Int>(P_ACCIDENTAL, accidental) &&
            ((Accidental)accidental != NoAccidental)) {
            w += npf.getAccidentalWidth();
        }

    } else if (e.event()->isa(Clef::EventType)) {

        w += npf.getClefWidth();

    } else if (e.event()->isa(Key::EventType)) {

        w += npf.getKeyWidth(Key(*e.event()));

    } else if (e.event()->isa(TimeSignature::EventType)) {

        w += npf.getTimeSigWidth(TimeSignature(*e.event()));

    } else {
        kdDebug(KDEBUG_AREA) << "NotationHLayout::getMinWidth(): no case for event type " << e.event()->getType() << endl;
        w += 24;
    }

    return w;
}

int NotationHLayout::getComfortableGap(const NotePixmapFactory &npf,
                                       Note::Type type) const
{
    int bw = npf.getNoteBodyWidth();
    if (type < Note::Quaver) return 1;
    else if (type == Note::Quaver) return (bw / 2);
    else if (type == Note::Crotchet) return (bw * 3) / 2;
    else if (type == Note::Minim) return (bw * 3);
    else if (type == Note::Semibreve) return (bw * 9) / 2;
    else if (type == Note::Breve) return (bw * 7);
    return 1;
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


