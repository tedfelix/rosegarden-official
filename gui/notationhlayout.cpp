
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
#include "Quantizer.h"

using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::NoAccidental;
using Rosegarden::Sharp;
using Rosegarden::Flat;
using Rosegarden::Natural;
using Rosegarden::Note;
using Rosegarden::Track;
using Rosegarden::TimeSignature;
using Rosegarden::timeT;
using Rosegarden::Quantizer;


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
    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: shortCount is "
                         << shortCount << ", fixedWidth is "
                         << fixedWidth << ", barDuration is "
                         << timeSignature.getBarDuration() << endl;

    if (shortest == m_notationElements.end()) {
        kdDebug(KDEBUG_AREA) << "First trivial return" << endl;
        return fixedWidth;
    }
    int d = (*shortest)->event()->getDuration();
    if (d == 0) {
        kdDebug(KDEBUG_AREA) << "Second trivial return" << endl;
        return fixedWidth;
    }

    int smin = getMinWidth(npf, **shortest);
    if (!(*shortest)->event()->get<Int>(Rosegarden::Note::NoteDots)) { //!!! double-dot?
        smin += npf.getDotWidth()/2;
    }

    /* if there aren't many of the shortest notes, we don't want to
       allow so much space to accommodate them */
    if (shortCount < 3) smin -= 3 - shortCount;

    int gapPer = 
        getComfortableGap(npf, (*shortest)->event()->get<Int>(Rosegarden::Note::NoteType)) +
        smin;

    kdDebug(KDEBUG_AREA) << "d is " << d << ", gapPer is " << gapPer << endl;

    int w = fixedWidth + timeSignature.getBarDuration() * gapPer / d;

    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: returning "
                         << w << endl;
    return w;
} 


// Compute bar data and cached properties.

void
NotationHLayout::preparse(const Track::BarPositionList &barPositions,
			  int firstBar, int lastBar)
{
    Key key;
    Clef clef;
    TimeSignature timeSignature;

    const NotePixmapFactory &npf(m_staff.getNotePixmapFactory());
    AccidentalTable accTable(key, clef), newAccTable(accTable);

    m_barData.clear();
    int fixedWidth = m_barMargin;
    
    for (int barNo = firstBar; barNo <= lastBar; ++barNo) {

	kdDebug(KDEBUG_AREA) << "preparse: Looking at bar " << barNo << endl;

	NotationElementList::iterator shortest = m_notationElements.end();
	int shortCount = 0;

	NotationElementList::iterator from =
	    m_notationElements.findTime(barPositions[barNo].start);

	NotationElementList::iterator to;
        if (barNo < (int)barPositions.size() - 1) {
	    to = m_notationElements.findTime(barPositions[barNo+1].start);
	} else {
	    to = m_notationElements.end();
	}

	for (NotationElementList::iterator it = from; it != to; ++it) {
        
	    NotationElement *el = (*it);
	    int mw = getMinWidth(npf, *el);

	    if (el->event()->isa(Clef::EventType)) {

		fixedWidth += mw;
		clef = Clef(*el->event());

		//!!! Probably not strictly the right thing to do
		// here, but I hope it'll do well enough in practice
		accTable = AccidentalTable(key, clef);
		newAccTable = accTable;

	    } else if (el->event()->isa(Key::EventType)) {

		fixedWidth += mw;
		key = Key(*el->event());

		accTable = AccidentalTable(key, clef);
		newAccTable = accTable;

	    } else if (el->event()->isa(TimeSignature::EventType)) {

		fixedWidth += mw;
		timeSignature = TimeSignature(*el->event());

	    } else if (el->isNote() || el->isRest()) {

		bool hasDuration = true;

		if (el->isNote()) {

		    try {
			int pitch = el->event()->get<Int>("pitch");
			kdDebug(KDEBUG_AREA) << "pitch : " << pitch << endl;
			Rosegarden::NotationDisplayPitch p(pitch, clef, key);
			int h = p.getHeightOnStaff();
			Accidental acc = p.getAccidental();

			el->event()->setMaybe<Int>(P_HEIGHT_ON_STAFF, h);
			el->event()->setMaybe<Int>(P_ACCIDENTAL, acc);
			el->event()->setMaybe<String>
			    (P_NOTE_NAME, p.getAsString(clef, key));

			// update display acc for note according to the
			// accTable (accidentals in force when the last
			// chord ended) and update newAccTable with
			// accidentals from this note.  (We don't update
			// accTable because there may be other notes in
			// this chord that need accTable to be the same as
			// it is for this one)
		    
			Accidental dacc = accTable.getDisplayAccidental(acc, h);

			//		kdDebug(KDEBUG_AREA) << "display accidental = " << dacc << endl;
		    
			el->event()->setMaybe<Int>(P_DISPLAY_ACCIDENTAL, dacc);
			newAccTable.update(acc, h);

			if (dacc != acc) {
			    // recalculate min width, as the accidental is
			    // not what we first thought
			    mw = getMinWidth(npf, *el);
			}

		    } catch (Event::NoData) {
			kdDebug(KDEBUG_AREA) <<
			    "NotationHLayout::preparse: couldn't get pitch for element"
					     << endl;
		    }

		    Chord chord(m_notationElements, it);
		    if (chord.size() >= 2 && it != chord.getFinalElement()) {
			// we're in a chord, but not at the end of it yet
			hasDuration = false;
		    } else {
			accTable = newAccTable;
		    }
		}

		if (hasDuration) {
		
		    // either we're not in a chord or the chord is about
		    // to end: update shortest data accordingly

		    int d = el->event()->get<Int>
			(Quantizer::NoteDurationProperty);

		    int sd = 0;
		    if (shortest == m_notationElements.end() ||
			d <= (sd = (*shortest)->event()->get<Int>
			      (Quantizer::NoteDurationProperty))) {
			if (d == sd) ++shortCount;
			else {
			    kdDebug(KDEBUG_AREA) << "New shortest! Duration is " << d << " (at " << el->getAbsoluteTime() << " time units)"<< endl;
			    shortest = it;
			    shortCount = 1;
			}
		    }
		}
	    }

	    el->event()->setMaybe<Int>(P_MIN_WIDTH, mw);
	}
	
        addNewBar(barNo, from,
                  getIdealBarWidth(fixedWidth, shortest, npf,
                                   shortCount, timeSignature),
                  fixedWidth);
    }
}


#ifdef NOT_DEFINED
// Locate the bars, and compute all the cached properties.

void
NotationHLayout::preparse(NotationElementList::iterator from,
                          NotationElementList::iterator to,
			  const Track::BarPositionList &barPositions)
{
    Key key;
    Clef clef;
    TimeSignature timeSignature;
    timeT nbTimeUnitsInCurrentBar = 0;
    const NotePixmapFactory &npf(m_staff.getNotePixmapFactory());
    AccidentalTable accTable(key, clef), newAccTable(accTable);

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
    timeT absoluteTime = 0;
    Quantizer quantizer;

    for ( ; it != to; ++it) {
        
        NotationElement *el = (*it);
        absoluteTime = el->getAbsoluteTime();

//         kdDebug(KDEBUG_AREA) << "NotationHLayout::preparse() : "
//                              << el->event()->getType() << " of duration "
//                              << el->event()->getDuration() << " at time "
//                              << absoluteTime << endl;

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

        if (el->isNote() || el->isRest())
            quantizer.quantizeByNote(el->event());
        int mw = getMinWidth(npf, *el);

        if (el->event()->isa(Clef::EventType)) {

            fixedWidth += mw;
            clef = Clef(*el->event());

	    //!!! Probably not strictly the right thing to do
	    // here, but I hope it'll do well enough in practice
	    accTable = AccidentalTable(key, clef);
	    newAccTable = accTable;

        } else if (el->event()->isa(Key::EventType)) {

            fixedWidth += mw;
            key = Key(*el->event());

	    accTable = AccidentalTable(key, clef);
	    newAccTable = accTable;

        } else if (el->event()->isa(TimeSignature::EventType)) {

            if (nbTimeUnitsInCurrentBar > 0) {
                // need to insert the bar line before this event...
                nbTimeUnitsInCurrentBar = 0;
		int savedWidth = 0;

                // ...and also before any preceding clef or key events
                NotationElementList::iterator it0(it), it1(it);
                while (it0 == it ||
                       (*it0)->event()->isa(Clef::EventType) ||
                       (*it0)->event()->isa(Key::EventType)) {
                    it1 = it0;
		    if (it0 != it) savedWidth += getMinWidth(npf, *(*it0));
                    // this shouldn't happen, as we've checked there
                    // are already some time units in the bar, but
                    // it's better to be safe than sorry:
                    if (it0 == m_notationElements.begin()) break;
                    --it0;
                }

		fixedWidth -= savedWidth;
                addNewBar(it1, absoluteTime, -1,
                          getIdealBarWidth(fixedWidth, shortest, npf,
                                           shortCount, timeSignature),
                          fixedWidth, true, true);
                fixedWidth = m_barMargin + mw + savedWidth;
                shortest = m_notationElements.end();
                shortCount = 1;
            }

            timeSignature = TimeSignature(*el->event());

        } else if (el->isNote() || el->isRest()) {

            bool hasDuration = true;

            if (el->isNote()) {

//		Accidental acc((Accidental)el->event()->get<Int>(P_ACCIDENTAL));
//		int height(el->event()->get<Int>(P_HEIGHT_ON_STAFF));
		
                //		kdDebug(KDEBUG_AREA) << "accidental = " << acc <<
                //		    ", height = " << height << endl;


                try {
                    int pitch = el->event()->get<Int>("pitch");
                    kdDebug(KDEBUG_AREA) << "pitch : " << pitch << endl;
                    Rosegarden::NotationDisplayPitch p(pitch, clef, key);
                    int h = p.getHeightOnStaff();
		    Accidental acc = p.getAccidental();

                    el->event()->setMaybe<Int>(P_HEIGHT_ON_STAFF, h);
                    el->event()->setMaybe<Int>(P_ACCIDENTAL, acc);
                    el->event()->setMaybe<String>
			(P_NOTE_NAME, p.getAsString(clef, key));

		    // update display acc for note according to the
		    // accTable (accidentals in force when the last
		    // chord ended) and update newAccTable with
		    // accidentals from this note.  (We don't update
		    // accTable because there may be other notes in
		    // this chord that need accTable to be the same as
		    // it is for this one)
		    
		    Accidental dacc = accTable.getDisplayAccidental(acc, h);

                //		kdDebug(KDEBUG_AREA) << "display accidental = " << dacc << endl;
		    
		    el->event()->setMaybe<Int>(P_DISPLAY_ACCIDENTAL, dacc);
		    newAccTable.update(acc, h);

		    if (dacc != acc) {
			// recalculate min width, as the accidental is
			// not what we first thought
			mw = getMinWidth(npf, *el);
		    }

                } catch (Event::NoData) {
                    kdDebug(KDEBUG_AREA) <<
                        "NotationHLayout::preparse: couldn't get pitch for element"
                                         << endl;
                }

                Chord chord(m_notationElements, it);
                if (chord.size() >= 2 && it != chord.getFinalElement()) {
                    // we're in a chord, but not at the end of it yet
                    hasDuration = false;
                } else {
		    accTable = newAccTable;
		}
            }

            if (hasDuration) {

                // either we're not in a chord or the chord is about
                // to end: update the time accordingly

//                int d = el->event()->get<Int>(P_QUANTIZED_DURATION); 
                int d = el->event()->get<Int>(Quantizer::NoteDurationProperty);
                nbTimeUnitsInCurrentBar += d;

                kdDebug(KDEBUG_AREA) << "Quantized duration is " << d
                                     << ", current bar now "
                                     << nbTimeUnitsInCurrentBar << endl;

                int sd = 0;
                if (shortest == m_notationElements.end() ||
                    d <= (sd = (*shortest)->event()->get<Int>
//                          (P_QUANTIZED_DURATION))) {
                          (Quantizer::NoteDurationProperty))) {
                    if (d == sd) ++shortCount;
                    else {
                        kdDebug(KDEBUG_AREA) << "New shortest! Duration is " << d << " (at " << nbTimeUnitsInCurrentBar << " time units)"<< endl;
                        shortest = it;
                        shortCount = 1;
                    }
                }
            }

            // See if we've completed a bar

            timeT barDuration = timeSignature.getBarDuration();

            if (nbTimeUnitsInCurrentBar >= barDuration) {
                barCorrect = (nbTimeUnitsInCurrentBar == barDuration);
                nbTimeUnitsInCurrentBar = 0;
                startNewBar = true;
            }
        }

        el->event()->setMaybe<Int>(P_MIN_WIDTH, mw);
    }

    if (startNewBar || nbTimeUnitsInCurrentBar > 0) {
        addNewBar(it, absoluteTime, -1,
                  getIdealBarWidth(fixedWidth, shortest, npf,
                                   shortCount, timeSignature),
                  fixedWidth, false,
                  (nbTimeUnitsInCurrentBar == timeSignature.getBarDuration()));
    }
}
#endif

NotationHLayout::AccidentalTable::AccidentalTable(Key key, Clef clef) :
    m_key(key), m_clef(clef)
{
    std::vector<int> heights(key.getAccidentalHeights(clef));
    unsigned int i;
    for (i = 0; i < 7; ++i) push_back(NoAccidental);
    for (i = 0; i < heights.size(); ++i) {
	(*this)[Key::canonicalHeight(heights[i])] =
	    (key.isSharp() ? Sharp : Flat);
    }
}

Accidental
NotationHLayout::AccidentalTable::getDisplayAccidental(Accidental accidental,
						       int height) const
{
    height = Key::canonicalHeight(height);

    if (accidental == NoAccidental) {
	accidental = m_key.getAccidentalAtHeight(height, m_clef);
    }

//    kdDebug(KDEBUG_AREA) << "accidental = " << accidental << ", stored accidental at height " << height << " is " << (*this)[height] << endl;

    if ((*this)[height] != NoAccidental) {

	if (accidental == NoAccidental || accidental == Natural) {
	    return Natural;
	} else if (accidental == (*this)[height]) {
	    return NoAccidental;
	} else {
	    //!!! aargh.  What we really want to do now is have two
	    //accidentals shown: first a natural, then the one
	    //required for the note.  But there's no scope for that in
	    //our accidental structure (RG2.1 is superior here)
	    return accidental;
	}
    } else {
	return accidental;
    }
}

void
NotationHLayout::AccidentalTable::update(Accidental accidental, int height)
{
    height = Key::canonicalHeight(height);

    if (accidental == NoAccidental) {
	accidental = m_key.getAccidentalAtHeight(height, m_clef);
    }

//    kdDebug(KDEBUG_AREA) << "updating height" << height << " from " << (*this)[height] << " to " << accidental << endl;


    //!!! again, we can't properly deal with the difficult case where
    //we already have an accidental at height but it's not the same
    //accidental

    (*this)[height] = accidental;

}

void
NotationHLayout::layout()
{
    Key key;
    Clef clef;
    TimeSignature timeSignature;

    double x = 0;
    const NotePixmapFactory &npf(m_staff.getNotePixmapFactory());

    int pGroupNo = -1;
    NotationElementList::iterator startOfGroup = m_notationElements.end();

    for (BarDataList::iterator bdi = m_barData.begin();
         bdi != m_barData.end(); ++bdi) {

        NotationElementList::iterator from = bdi->start;
        NotationElementList::iterator to;
        BarDataList::iterator nbdi(bdi);
        if (++nbdi == m_barData.end()) {
            to = m_notationElements.end();
        } else {
            to = nbdi->start;
        }

        kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): starting a bar, initial x is " << x << " and barWidth is " << bdi->idealWidth << endl;


        bdi->x = (int)(x + m_barMargin / 2);
        x += m_barMargin;

	bool haveAccidentalInThisChord = false;

        for (NotationElementList::iterator it = from; it != to; ++it) {
            
            NotationElement *el = (*it);
            el->setLayoutX(x);
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): setting element's x to " << x << endl;

            long delta = el->event()->get<Int>(P_MIN_WIDTH);

            if (el->event()->isa(TimeSignature::EventType)) {

                timeSignature = TimeSignature(*el->event());

            } else if (el->event()->isa(Clef::EventType)) {

		// nasty hack: if there's a time signature or key
		// before this, swap places with it...

                /*!! no, this really isn't the right thing
                  if (it != m_notationElements.begin()) {
                  NotationElementList::iterator it0(it);
                  for (;;) {
                  --it0;
                  int x0;
                  if (((*it0)->event()->isa(TimeSignature::EventType) ||
                  (*it0)->event()->isa(Key::EventType)) &&
                  (x0 = (*it0)->getLayoutX()) < x) {
                  el->setLayoutX(x0);
                  (*it0)->setLayoutX
                  ((x + delta) -
                  (*it0)->event()->get<Int>(P_MIN_WIDTH));
                  } else break;
                  if (it0 == m_notationElements.begin()) break;
                  }
                  }
                */

                clef = Clef(*el->event());

            } else if (el->event()->isa(Key::EventType)) {

		// nasty hack: if there's a time signature before
		// this, move it to after...
                /*!! no, not right
                  if (it != m_notationElements.begin()) {
                  NotationElementList::iterator it0(it);
                  for (;;) {
                  --it0;
                  int x0;
                  if ((*it0)->event()->isa(TimeSignature::EventType) &&
                  (x0 = (*it0)->getLayoutX()) < x) {
                  el->setLayoutX(x0);
                  (*it0)->setLayoutX
                  ((x + delta) -
                  (*it0)->event()->get<Int>(P_MIN_WIDTH));
                  } else break;
                  if (it0 == m_notationElements.begin()) break;
                  }
                  }
                */
                key = Key(*el->event());

            } else if (el->isRest()) {

                // To work out how much space to allot a note (or
                // chord), start with the amount alloted to the
                // whole bar, subtract that reserved for
                // fixed-width items, and take the same proportion
                // of the remainder as our duration is of the
                // whole bar's duration.
 
                delta = ((bdi->idealWidth - bdi->fixedWidth) *
                         el->event()->getDuration()) /
                    //!!! not right for partial bar?
                    timeSignature.getBarDuration();

                 kdDebug(KDEBUG_AREA) << "Rest idealWidth : "
                                      << bdi->idealWidth
                                      << " - fixedWidth : "
                                      << bdi->fixedWidth << endl;


            } else if (el->isNote()) {
                
		// Retrieve the record the presence of any display
		// accidental.  We'll need to shift the x-coord
		// slightly if there is one, because the
		// notepixmapfactory quite reasonably places the hot
		// spot at the start of the note head, not at the
		// start of the whole pixmap.  But we can't do that
		// here because it needs to be done for all notes in a
		// chord, when at least one of those notes has an
		// accidental.

		Accidental acc(NoAccidental);
		{
		    long acc0;
		    if (el->event()->get<Int>(P_DISPLAY_ACCIDENTAL, acc0)) {
			acc = (Accidental)acc0;
		    }
		}
		if (acc != NoAccidental) haveAccidentalInThisChord = true;
		
                Chord chord(m_notationElements, it);
                if (chord.size() < 2 || it == chord.getFinalElement()) {

		    // either we're not in a chord, or the chord is
		    // about to end: update the delta now, and add any
		    // additional accidental spacing

		    if (haveAccidentalInThisChord) {
			for (int i = 0; i < (int)chord.size(); ++i) {
			    (*chord[i])->setLayoutX
				((*chord[i])->getLayoutX() +
				 npf.getAccidentalWidth());
			}
		    }

                    //		    kdDebug(KDEBUG_AREA) << "This is the final chord element (of " << chord.size() << ")" << endl;

                    // To work out how much space to allot a note (or
                    // chord), start with the amount alloted to the
                    // whole bar, subtract that reserved for
                    // fixed-width items, and take the same proportion
                    // of the remainder as our duration is of the
                    // whole bar's duration.
                    
                    delta = ((bdi->idealWidth - bdi->fixedWidth) *
                             el->event()->getDuration()) /
                        //!!! not right for partial bar?
                        timeSignature.getBarDuration();

//                     kdDebug(KDEBUG_AREA) << "Note idealWidth : "
//                                          << bdi->idealWidth
//                                          << " - fixedWidth : "
//                                          << bdi->fixedWidth << endl;

                } else {
		    kdDebug(KDEBUG_AREA) << "This is not the final chord element (of " << chord.size() << ")" << endl;
		    delta = 0;
		}

		// See if we're in a group, and add the beam if so.
		// This needs to happen after the chord-related
		// manipulations abpve because the beam's position
		// depends on the x-coord of the note, which depends
		// on the presence of accidentals somewhere in the
		// chord.  (All notes in a chord should be in the same
		// group, so the leaving-group calculation will only
		// happen after all the notes in the final chord of
		// the group have been processed.)

                long groupNo = -1;

                if (el->event()->get<Int>(Track::BeamedGroupIdPropertyName,
                                          groupNo) &&
                    groupNo != pGroupNo) {
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: entering group " << groupNo << endl;
                    startOfGroup = it;
                }

                long nextGroupNo = -1;
                NotationElementList::iterator it0(it);
                ++it0;
                if (groupNo > -1 &&
                    (it0 == m_notationElements.end() ||
                     (!(*it0)->event()->get<Int>
                      (Track::BeamedGroupIdPropertyName, nextGroupNo) ||
                      nextGroupNo != groupNo))) {
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: about to leave group " << groupNo << ", time to do the sums" << endl;
                
                    NotationGroup group(m_notationElements, it, clef, key);
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: group type is " << group.getGroupType() << ", now applying beam" << endl;
                    group.applyBeam(m_staff);
                }

                pGroupNo = groupNo;
            }

            x += delta;
            kdDebug(KDEBUG_AREA) << "x = " << x << endl;
        }
    }
}


#ifdef NOT_DEFINED
void
NotationHLayout::addNewBar(NotationElementList::iterator start,
                           timeT time, int x, int width, int fwidth,
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

#endif


void
NotationHLayout::addNewBar(int barNo,  NotationElementList::iterator start,
                           int width, int fwidth)
{
    m_barData.push_back(BarData(barNo, start, -1, width, fwidth));
}


int NotationHLayout::getMinWidth(const NotePixmapFactory &npf,
                                 const NotationElement &e) const
{
    int w = m_noteMargin;

    if (e.isNote() || e.isRest()) {

        w += npf.getNoteBodyWidth();
        long dots;
        if (e.event()->get<Int>(Rosegarden::Note::NoteDots, dots)) {
            w += npf.getDotWidth() * dots;
        }
        long accidental;
        if (e.event()->get<Int>(P_DISPLAY_ACCIDENTAL, accidental) &&
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
    m_barData.clear();
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


