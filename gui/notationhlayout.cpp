// -*- c-basic-offset: 4 -*-

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

#include "notationhlayout.h"
#include "notationstaff.h"
#include "rosedebug.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "notepixmapfactory.h"
#include "notationproperties.h"
#include "notationsets.h"
#include "Quantizer.h"
#include "Composition.h"
#include "SegmentNotationHelper.h"

using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Indication;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::TimeSignature;
using Rosegarden::timeT;
using Rosegarden::Quantizer;

using Rosegarden::Accidental;
using namespace Rosegarden::Accidentals;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

std::vector<double> NotationHLayout::m_availableSpacings;


NotationHLayout::NotationHLayout(NotePixmapFactory &npf) :
    m_totalWidth(0.),
    m_spacing(1.0),
    m_npf(npf)
{
    kdDebug(KDEBUG_AREA) << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    // empty
}

std::vector<double>
NotationHLayout::getAvailableSpacings()
{
    if (m_availableSpacings.size() == 0) {
        m_availableSpacings.push_back(0.3);
        m_availableSpacings.push_back(0.6);
        m_availableSpacings.push_back(0.85);
        m_availableSpacings.push_back(1.0);
        m_availableSpacings.push_back(1.3);
        m_availableSpacings.push_back(1.7);
        m_availableSpacings.push_back(2.2);
    }
    return m_availableSpacings;
}


NotationHLayout::BarDataList &
NotationHLayout::getBarData(StaffType &staff)
{
    BarDataMap::iterator i = m_barData.find(&staff);
    if (i == m_barData.end()) {
	m_barData[&staff] = BarDataList();
    }

    return m_barData[&staff];
}


const NotationHLayout::BarDataList &
NotationHLayout::getBarData(StaffType &staff) const
{
    return ((NotationHLayout *)this)->getBarData(staff);
}


// To find the "ideal" width of a bar, we need the sum of the minimum
// widths of the elements in the bar, plus as much extra space as
// would be needed if the bar was made up entirely of repetitions of
// its shortest note.  This space is the product of the number of the
// bar's shortest notes that will fit in the duration of the bar and
// the comfortable gap for each.

// Some ground rules for "ideal" layout:
// 
// -- The shortest notes in the bar need to have a certain amount of
//    space, but if they're _very_ short compared to the total bar
//    duration then we can probably afford to squash them somewhat
// 
// -- If there are lots of the shortest note duration, then we
//    should try not to squash them quite so much
// 
// -- If there are not very many notes in the bar altogether, we can
//    squash things up a bit more perhaps
// 
// -- Similarly if they're dotted, we need space for the dots; we
//    can't risk making the dots invisible
// 
// -- In theory we don't necessarily want the whole bar width to be
//    the product of the shortest-note width and the number of shortest
//    notes in the bar.  But it's difficult to plan the spacing
//    otherwise.  One possibility is to augment the fixedWidth with a
//    certain proportion of the width of each note, and make that a
//    higher proportion for very short notes than for long notes.

//!!! The algorithm below does not implement most of these rules; it
// can probably be improved dramatically without too much work

int NotationHLayout::getIdealBarWidth(StaffType &staff,
                                      int fixedWidth,
                                      int baseWidth,
                                      NotationElementList::iterator shortest,
                                      int shortCount,
                                      int totalCount,
                                      const TimeSignature &timeSignature) const
{
    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: shortCount is "
                         << shortCount << ", fixedWidth is "
                         << fixedWidth << ", barDuration is "
                         << timeSignature.getBarDuration() << endl;

    if (shortest == staff.getViewElementList()->end()) {
        kdDebug(KDEBUG_AREA) << "First trivial return" << endl;
        return fixedWidth;
    }

    int d = 0;

    try {
	d = (*shortest)->event()->get<Int>(Quantizer::LegatoDurationProperty);
    } catch (Event::NoData e) {
	kdDebug(KDEBUG_AREA) << "getIdealBarWidth: No quantized duration in shortest! event is " << *((*shortest)->event()) << endl;
    }

    if (d == 0) {
        kdDebug(KDEBUG_AREA) << "Second trivial return" << endl;
        return fixedWidth;
    }

    int smin = getMinWidth(**shortest);
    if (!(*shortest)->event()->get<Int>(NOTE_DOTS)) {
        smin += m_npf.getDotWidth()/2;
    }

    /* if there aren't many of the shortest notes, we don't want to
       allow so much space to accommodate them */
    if (shortCount < 3) smin -= 3 - shortCount;

    int gapPer = 
        getComfortableGap((*shortest)->event()->get<Int>(NOTE_TYPE)) +
        smin;

    kdDebug(KDEBUG_AREA) << "d is " << d << ", gapPer is " << gapPer << endl;

    int w = fixedWidth + timeSignature.getBarDuration() * gapPer / d;

    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: returning "
                         << w << endl;

    w = (int)(w * m_spacing);
    if (w < (fixedWidth + baseWidth)) w = fixedWidth + baseWidth;
    return w;
} 


void
NotationHLayout::scanStaff(StaffType &staff)
{
    START_TIMING;

    Segment &t(staff.getSegment());
    const Segment *timeRef = t.getComposition()->getReferenceSegment();

    if (timeRef == 0) {
	kdDebug(KDEBUG_AREA) << "ERROR: NotationHLayout::scanStaff: reference segment required (at least until code\nis written to render a segment without bar lines)" << endl;
	return;
    }

    NotationElementList *notes = staff.getViewElementList();

    BarDataList &barList(getBarData(staff));

    Key key;
    Clef clef;
    TimeSignature timeSignature;

    barList.clear();

    Segment::iterator refStart = timeRef->findTime(t.getStartIndex());
    Segment::iterator refEnd = timeRef->findTime(t.getEndIndex());
    if (refEnd != timeRef->end()) ++refEnd;
    
    int barNo = 0;

    SegmentNotationHelper nh(t);
    nh.quantize();

    addNewBar(staff, barNo, notes->begin(), 0, 0, true, 0); 
    ++barNo;

    for (Segment::iterator refi = refStart; refi != refEnd; ++refi) {

	timeT barStartTime = (*refi)->getAbsoluteTime();
	timeT   barEndTime;

	Segment::iterator refi0(refi);
	if (++refi0 != refEnd) {
	    barEndTime = (*refi0)->getAbsoluteTime();
	} else {
	    barEndTime = t.getEndIndex();
	}

        NotationElementList::iterator from = notes->findTime(barStartTime);
        NotationElementList::iterator to = notes->findTime(barEndTime);

	kdDebug(KDEBUG_AREA) << "NotationHLayout::scanStaff: bar " << barNo << ", from " << barStartTime << ", to " << barEndTime << " (end " << t.getEndIndex() << ")" << endl;

        NotationElementList::iterator shortest = notes->end();
        int shortCount = 0;
        int totalCount = 0;

        // fixedWidth includes clefs, keys &c, but also accidentals
	int fixedWidth = getBarMargin();

        // baseWidth is absolute minimum width of non-fixedWidth elements
        int baseWidth = 0;

        timeT apparentBarDuration = 0;

	AccidentalTable accTable(key, clef), newAccTable(accTable);

	Event *timeSigEvent = 0;

	if ((*refi)->isa(TimeSignature::EventType)) {
	    kdDebug(KDEBUG_AREA) << "Found timesig" << endl;
	    timeSigEvent = *refi;
	    timeSignature = TimeSignature(*timeSigEvent);
	    fixedWidth += getFixedItemSpacing() +
		m_npf.getTimeSigWidth(timeSignature);
	}

        for (NotationElementList::iterator it = from; it != to; ++it) {
        
            NotationElement *el = (*it);
            int mw = getMinWidth(*el);

            if (el->event()->isa(Clef::EventType)) {

		kdDebug(KDEBUG_AREA) << "Found clef" << endl;

                fixedWidth += mw;
                clef = Clef(*el->event());

                //!!! Probably not strictly the right thing to do
                // here, but I hope it'll do well enough in practice
                accTable = AccidentalTable(key, clef);
                newAccTable = accTable;

            } else if (el->event()->isa(Key::EventType)) {

		kdDebug(KDEBUG_AREA) << "Found key" << endl;

                fixedWidth += mw;
                key = Key(*el->event());

                accTable = AccidentalTable(key, clef);
                newAccTable = accTable;

            } else if (el->isNote() || el->isRest()) {

                bool hasDuration = true;

                if (el->isNote()) {

                    long pitch = 64;
                    if (!el->event()->get<Int>(PITCH, pitch)) {
                        kdDebug(KDEBUG_AREA) <<
                            "WARNING: NotationHLayout::scanStaff: couldn't get pitch for element, using default pitch of " << pitch << endl;
                    }

                    Accidental explicitAccidental = NoAccidental;
		    (void)el->event()->get<String>(ACCIDENTAL,
						   explicitAccidental);

                    Rosegarden::NotationDisplayPitch p
                        (pitch, clef, key, explicitAccidental);
                    int h = p.getHeightOnStaff();
                    Accidental acc = p.getAccidental();

                    el->event()->setMaybe<Int>(HEIGHT_ON_STAFF, h);
                    el->event()->setMaybe<String>(CALCULATED_ACCIDENTAL, acc);

                    // update display acc for note according to the
                    // accTable (accidentals in force when the last
                    // chord ended) and update newAccTable with
                    // accidentals from this note.  (We don't update
                    // accTable because there may be other notes in
                    // this chord that need accTable to be the same as
                    // it is for this one)
                    
                    Accidental dacc = accTable.getDisplayAccidental(acc, h);
                    el->event()->setMaybe<String>(DISPLAY_ACCIDENTAL, dacc);

                    newAccTable.update(acc, h);

                    if (dacc != NoAccidental) {
                        //!!! wrong for chords with >1 accidental
                        fixedWidth += m_npf.getAccidentalWidth(dacc);
                    }

                    Chord chord(*notes, it);
                    if (chord.size() >= 2 && it != chord.getFinalElement()) {
                        // we're in a chord, but not at the end of it yet
                        hasDuration = false;
                    } else {
                        accTable = newAccTable;
                        if (chord.hasNoteHeadShifted()) {
                            fixedWidth += m_npf.getNoteBodyWidth();
                        }
                    }
                }

                if (hasDuration) {
                
                    // either we're not in a chord or the chord is about
                    // to end: update shortest data accordingly

                    int d = 0;
                    try {
                        d = el->event()->get<Int>(Quantizer::LegatoDurationProperty);
                    } catch (Event::NoData e) {
                        kdDebug(KDEBUG_AREA) << "No quantized duration in note/rest! event is " << *(el->event()) << endl;
                    }

                    ++totalCount;
                    apparentBarDuration += d;

                    int sd = 0;
                    try {
//    kdDebug(KDEBUG_AREA) << "NotationHLayout::scanStaff: about to query legato duration property" << endl;
			if (d > 0 &&
			    (shortest == notes->end() ||
			     d <= (sd = (*shortest)->event()->get<Int>
				   (Quantizer::LegatoDurationProperty)))) {
			    if (d == sd) {

                                // assumption: rests are wider than notes
                                if ((*shortest)->isNote() &&
                                    (*it)->isRest()) shortest = it;

                                ++shortCount;
                            } else {
				shortest = it;
				shortCount = 1;
			    }
			}
//kdDebug(KDEBUG_AREA) << "done" <<endl;
                    } catch (Event::NoData e) {
                        kdDebug(KDEBUG_AREA) << "No quantized duration in shortest! event is " << *((*shortest)->event()) << endl;
                    }
                }

                baseWidth += mw;

	    } else if (el->event()->isa(Indication::EventType)) {

		kdDebug(KDEBUG_AREA) << "Found indication" << endl;

		mw = 0;

	    } else {
		
		kdDebug(KDEBUG_AREA) << "Found something I don't know about" << endl;
		fixedWidth += mw;
	    }

            el->event()->setMaybe<Int>(MIN_WIDTH, mw);
	}


        addNewBar(staff, barNo, to,
                  getIdealBarWidth(staff, fixedWidth, baseWidth, shortest, 
                                   shortCount, totalCount, timeSignature),
                  fixedWidth,
                  apparentBarDuration == timeSignature.getBarDuration(),
		  timeSigEvent);

	++barNo;
    }

    PRINT_ELAPSED("NotationHLayout::scanStaff");
}


void
NotationHLayout::addNewBar(StaffType &staff,
			   int barNo, NotationElementList::iterator i,
                           int width, int fwidth, bool correct, Event *timeSig)
{
    BarDataList &bdl(m_barData[&staff]);

    int s = bdl.size() - 1;
    if (s >= 0) {
        bdl[s].idealWidth = width;
        bdl[s].fixedWidth = fwidth;
	bdl[s].timeSignature = timeSig;
    }

    if (timeSig) kdDebug(KDEBUG_AREA) << "Adding bar with timesig" << endl;
    else kdDebug(KDEBUG_AREA) << "Adding bar without timesig" << endl;

    bdl.push_back(BarData(barNo, i, -1, 0, 0, correct, 0));
}


void
NotationHLayout::reconcileBars()
{
    START_TIMING;

    // Scoot through the staffs, prepending fake bars to the start of
    // the bar list for any staff that doesn't begin right at the
    // start of the composition

    BarDataMap::iterator i;

    for (i = m_barData.begin(); i != m_barData.end(); ++i) {

        StaffType *staff = i->first;
        BarDataList &list = i->second;

        if (list.size() > 0 && list[0].barNo < 0) continue; // done it already

        Segment &segment = staff->getSegment();
        const Segment &refSegment = *(segment.getComposition()->getReferenceSegment());

        for (Segment::const_iterator j = refSegment.begin();
             j != refSegment.end(); ++j) {

            if ((*j)->getAbsoluteTime() >= segment.getStartIndex()) break;
            list.push_front
                (BarData
                 (-1, staff->getViewElementList()->end(), -1, 0, 0, true, 0));
        }
    }

    // Ensure that concurrent bars on all staffs have the same width,
    // which for now we make the maximum width required for this bar
    // on any staff

    unsigned int barNo = 0;
    bool reachedEnd = false;
    bool aWidthChanged = false;

    while (!reachedEnd) {

	int maxWidth = -1;
	reachedEnd = true;

	for (i = m_barData.begin(); i != m_barData.end(); ++i) {

	    BarDataList &list = i->second;

	    if ((int)list.size() > barNo) {

		reachedEnd = false;

		if (list[barNo].idealWidth > maxWidth) {
		    maxWidth = i->second[barNo].idealWidth;
		}
	    }
	}

	for (i = m_barData.begin(); i != m_barData.end(); ++i) {

	    BarDataList &list = i->second;

	    if ((int)list.size() > barNo) {

		BarData &bd(list[barNo]);

		if (bd.idealWidth != maxWidth) {
		    if (bd.idealWidth > 0) {
			float ratio = (float)maxWidth / (float)bd.idealWidth;
			bd.fixedWidth += bd.fixedWidth * int((ratio - 1.0)/2.0);
		    }
		    bd.idealWidth = maxWidth;
                    aWidthChanged = true;
		}

                if (aWidthChanged) bd.needsLayout = true;
	    }
	}

	++barNo;
    }

    PRINT_ELAPSED("NotationHLayout::reconcileBars");
}	


// and for once I swear things will still be good tomorrow

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

        if (accidental == (*this)[height]) {
            return NoAccidental;
        } else if (accidental == NoAccidental || accidental == Natural) {
            return Natural;
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
NotationHLayout::finishLayout()
{
    reconcileBars();
    
    for (BarDataMap::iterator i(m_barData.begin()); i != m_barData.end(); ++i)
	layout(i);
}

void
NotationHLayout::layout(BarDataMap::iterator i)
{
    START_TIMING;

    StaffType &staff = *(i->first);
    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));

    Key key;
    Clef clef;
    TimeSignature timeSignature;

    int x = 0, barX = 0;
    TieMap tieMap;

    for (BarDataList::iterator bdi = barList.begin();
         bdi != barList.end(); ++bdi) {

        NotationElementList::iterator from = bdi->start;
        NotationElementList::iterator to;
        BarDataList::iterator nbdi(bdi);
        if (++nbdi == barList.end()) {
            to = notes->end();
        } else {
            to = nbdi->start;
        }

        kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): starting barNo " << bdi->barNo << ", x = " << barX << ", width = " << bdi->idealWidth << ", time = " << (from == notes->end() ? -1 : (*from)->getAbsoluteTime()) << endl;

        if (from == notes->end()) {
            kdDebug(KDEBUG_AREA) << "Start is end" << endl;
        }
        if (from == to) {
            kdDebug(KDEBUG_AREA) << "Start is to" << endl;
        }

	x = barX;
        bdi->x = x + getBarMargin() / 2;
        x += getBarMargin();
	barX += bdi->idealWidth;

	bool timeSigToPlace = false;
	if (bdi->timeSignature) {
	    timeSignature = TimeSignature(*(bdi->timeSignature));
	    timeSigToPlace = true;
	}

        if (bdi->barNo < 0) { // fake bar
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): fake bar " << bdi->barNo << endl;
            continue;
        }
        if (!bdi->needsLayout) {
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): bar " << bdi->barNo << " has needsLayout false" << endl;
            continue;
        }

        if (timeSigToPlace) {
	    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): there's a time sig in this bar" << endl;
	}

//!!!        Accidental accidentalInThisChord = NoAccidental;

        for (NotationElementList::iterator it = from; it != to; ++it) {
            
            NotationElement *el = (*it);
            el->setLayoutX(x);
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): setting element's x to " << x << endl;

            long delta = el->event()->get<Int>(MIN_WIDTH);

	    if (timeSigToPlace && !el->event()->isa(Clef::EventType)) {
		kdDebug(KDEBUG_AREA) << "Placing timesig at " << x << endl;
		bdi->timeSigX = x;
		x += getFixedItemSpacing() +
		    m_npf.getTimeSigWidth(timeSignature);
		kdDebug(KDEBUG_AREA) << "and moving next elt to " << x << endl;
		el->setLayoutX(x);
		timeSigToPlace = false;
	    }

	    if (el->isNote()) {

		// This modifies "it" and "tieMap"
		delta = positionChord
		    (staff, it, bdi, timeSignature, clef, key, tieMap);

	    } else if (el->isRest()) {

		delta = positionRest(staff, it, bdi, timeSignature);

	    } else if (el->event()->isa(Clef::EventType)) {
		
		kdDebug(KDEBUG_AREA) << "Found clef" << endl;
		clef = Clef(*el->event());

	    } else if (el->event()->isa(Key::EventType)) {

		kdDebug(KDEBUG_AREA) << "Found key" << endl;
		key = Key(*el->event());

	    }

            x += delta;
        }

        bdi->needsLayout = false;
    }

    if (x > m_totalWidth) m_totalWidth = x;

    PRINT_ELAPSED("NotationHLayout::layout");
}


long
NotationHLayout::positionRest(StaffType &,
                              const NotationElementList::iterator &itr,
                              const BarDataList::iterator &bdi,
                              const TimeSignature &timeSignature)
{
    NotationElement *rest = *itr;

    // To work out how much space to allot a rest, as for a note,
    // start with the amount alloted to the whole bar, subtract that
    // reserved for fixed-width items, and take the same proportion of
    // the remainder as our duration is of the whole bar's duration.
 
    long delta = ((bdi->idealWidth - bdi->fixedWidth) *
                  rest->event()->getDuration()) /
        //!!! not right for partial bar?
        timeSignature.getBarDuration();

    // Situate the rest somewhat further into its allotted space.  Not
    // convinced this is the right thing to do

    int baseWidth = m_npf.getRestWidth
	(Note(rest->event()->get<Int>(NOTE_TYPE),
	      rest->event()->get<Int>(NOTE_DOTS)));

    if (delta > 2 * baseWidth) {
        int shift = (delta - 2 * baseWidth) / 4;
        shift = std::min(shift, (baseWidth * 3));
        rest->setLayoutX(rest->getLayoutX() + shift);
    }
                
    return delta;
}


long
NotationHLayout::positionChord(StaffType &staff,
                               NotationElementList::iterator &itr,
			       const BarDataList::iterator &bdi,
			       const TimeSignature &timeSignature,
			       const Clef &clef, const Key &key,
			       TieMap &tieMap)
{
    Chord chord(*staff.getViewElementList(), itr, clef, key);
    double baseX = (*itr)->getLayoutX();

    // To work out how much space to allot a note (or chord), start
    // with the amount alloted to the whole bar, subtract that
    // reserved for fixed-width items, and take the same proportion of
    // the remainder as our duration is of the whole bar's duration.
    // We use the shortest note in the chord, should the durations vary
                    
    long delta = ((bdi->idealWidth - bdi->fixedWidth) *
                  (*chord.getShortestElement())->event()->getDuration()) /
        //!!! not right for partial bar?
        timeSignature.getBarDuration();

    int noteWidth = m_npf.getNoteBodyWidth();

    // If the chord's allowed a lot of space, situate it somewhat
    // further into its allotted space.  Not convinced this is always
    // the right thing to do.

    if (delta > 2 * noteWidth) {
        int shift = (delta - 2 * noteWidth) / 5;
	baseX += std::min(shift, (m_npf.getNoteBodyWidth() * 3 / 2));
    }

    // Find out whether the chord contains any accidentals, and if so,
    // make space, and also shift the notes' positions right somewhat.
    // (notepixmapfactory quite reasonably places the hot spot at the
    // start of the note head, not at the start of the whole pixmap.)
    // Also use this loop to check for beamed-group information.

    unsigned int i;
    int accWidth = 0;
    long groupId = -1;

    for (i = 0; i < chord.size(); ++i) {

	NotationElement *note = *(chord[i]);
	if (!note->isNote()) continue;

	Accidental acc = NoAccidental;
	if (note->event()->get<String>(DISPLAY_ACCIDENTAL, acc) &&
	    acc != NoAccidental) {
            accWidth = std::max(accWidth, m_npf.getAccidentalWidth(acc));
	}

	if (groupId != -2) {
	    long myGroupId = -1;
	    if (note->event()->get<Int>(BEAMED_GROUP_ID, myGroupId) &&
		(groupId == -1 || myGroupId == groupId)) {
		groupId = myGroupId;
	    } else {
		groupId = -2; // not all note-heads think they're in the group
	    }
	}
    }

    baseX += accWidth;
    delta += accWidth;

    // Cope with the presence of shifted note-heads

    bool shifted = chord.hasNoteHeadShifted();

    if (shifted) {
	if (delta < noteWidth * 2) delta = noteWidth * 2;
	if (!chord.hasStemUp()) baseX += noteWidth;
    }

    // Check for any ties going back, and if so work out how long it
    // must have been and assign accordingly.

    for (i = 0; i < chord.size(); ++i) {

	NotationElement *note = *(chord[i]);
	if (!note->isNote()) continue;

	bool tiedForwards = false;
	bool tiedBack = false;

	note->event()->get<Bool>(TIED_FORWARD,  tiedForwards);
	note->event()->get<Bool>(TIED_BACKWARD, tiedBack);

	int pitch = note->event()->get<Int>(PITCH);
	if (tiedForwards) {
	    note->event()->setMaybe<Int>(TIE_LENGTH, 0);
	    tieMap[pitch] = itr;
	} else {
	    note->event()->unset(TIE_LENGTH);
	}

	if (tiedBack) {
	    TieMap::iterator ti(tieMap.find(pitch));

	    if (ti != tieMap.end()) {
		NotationElementList::iterator otherItr(ti->second);
		
		if ((*otherItr)->getAbsoluteTime() +
		    (*otherItr)->getDuration() ==
		    note->getAbsoluteTime()) {
		    
		    (*otherItr)->event()->setMaybe<Int>
			(TIE_LENGTH,
			 (int)(baseX - (*otherItr)->getLayoutX()));
		    
		} else {
		    tieMap.erase(pitch);
		}
	    }
	}
    }

    // Now set the positions of all the notes in the chord.  We don't
    // need to shift the positions of shifted notes, because that will
    // be taken into account when making their pixmaps later (in
    // NotationStaff::makeNoteSprite / NotePixmapFactory::makeNotePixmap).

    for (i = 0; i < chord.size(); ++i) {
	(*(chord[i]))->setLayoutX(baseX);
    }

    itr = chord.getFinalElement();
    if (groupId < 0) return delta;

    // Finally set the beam data

    long nextGroupId = -1;
    NotationElementList::iterator scooter(itr);
    ++scooter;

    if (scooter == staff.getViewElementList()->end() ||
	!(*scooter)->event()->get<Int>(BEAMED_GROUP_ID, nextGroupId) ||
	nextGroupId != groupId) {
	
	//!!! not very nice
	NotationStaff &notationStaff = dynamic_cast<NotationStaff &>(staff);
	NotationGroup group(*staff.getViewElementList(), itr, clef, key);
	group.applyBeam(notationStaff);
	group.applyTuplingLine(notationStaff);
    }

    return delta;
}


int NotationHLayout::getMinWidth(NotationElement &e) const
{
    int w = 0;

    if (e.isNote()) {

        long noteType = e.event()->get<Int>(NOTE_TYPE, noteType);
        w += m_npf.getNoteBodyWidth(noteType);

        long dots;
        if (e.event()->get<Int>(NOTE_DOTS, dots)) {
            w += m_npf.getDotWidth() * dots;
        }

        return w;

    } else if (e.isRest()) {

        w += m_npf.getRestWidth(Note(e.event()->get<Int>(NOTE_TYPE),
                                     e.event()->get<Int>(NOTE_DOTS)));

        return w;
    }

    w = getFixedItemSpacing();

    if (e.event()->isa(Clef::EventType)) {

        w += m_npf.getClefWidth(Clef(*e.event()));

    } else if (e.event()->isa(Key::EventType)) {

        w += m_npf.getKeyWidth(Key(*e.event()));

    } else {
        kdDebug(KDEBUG_AREA) << "NotationHLayout::getMinWidth(): no case for event type " << e.event()->getType() << endl;
        w += 24;
    }

    return w;
}

int NotationHLayout::getComfortableGap(Note::Type type) const
{
    int bw = m_npf.getNoteBodyWidth();
    if (type < Note::Quaver) return 1;
    else if (type == Note::Quaver) return (bw / 2);
    else if (type == Note::Crotchet) return (bw * 3) / 2;
    else if (type == Note::Minim) return (bw * 3);
    else if (type == Note::Semibreve) return (bw * 9) / 2;
    else if (type == Note::Breve) return (bw * 7);
    return 1;
}        

int NotationHLayout::getBarMargin() const
{
    return (int)(m_npf.getBarMargin() * m_spacing);
}

void
NotationHLayout::reset()
{
    m_barData.clear();
    m_totalWidth = 0;
}

void
NotationHLayout::resetStaff(StaffType &staff)
{
    getBarData(staff).clear();
    m_totalWidth = 0;
}

unsigned int
NotationHLayout::getBarLineCount(StaffType &staff)
{
    int c = getBarData(staff).size();
    return c;
}

double
NotationHLayout::getBarLineX(StaffType &staff, unsigned int i)
{
    return getBarData(staff)[i].x;
}

int
NotationHLayout::getBarLineDisplayNumber(StaffType &staff, unsigned int i)
{
    return getBarData(staff)[i].barNo;
}

bool
NotationHLayout::isBarLineCorrect(StaffType &staff, unsigned int i)
{
    return getBarData(staff)[i].correct;
}

Event *NotationHLayout::getTimeSignatureInBar(StaffType &staff,
					      unsigned int i, int &timeSigX)
{
    BarData &bd(getBarData(staff)[i]);
    timeSigX = bd.timeSigX;
    return bd.timeSignature;
}

