
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
#include "TrackNotationHelper.h"

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
using Rosegarden::TrackNotationHelper;
using Rosegarden::TimeSignature;
using Rosegarden::timeT;
using Rosegarden::Quantizer;


NotationHLayout::NotationHLayout() :
    m_totalWidth(0.)
{
    kdDebug(KDEBUG_AREA) << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    // empty
}


NotationHLayout::BarDataList &
NotationHLayout::getBarData(Staff &staff)
{
    BarDataMap::iterator i = m_barData.find(&staff);
    if (i == m_barData.end()) {
	m_barData[&staff] = BarDataList();
    }

    return m_barData[&staff];
}


const NotationHLayout::BarDataList &
NotationHLayout::getBarData(Staff &staff) const
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

int NotationHLayout::getIdealBarWidth(Staff &staff,
                                      int fixedWidth,
                                      NotationElementList::iterator shortest,
                                      const NotePixmapFactory &npf,
                                      int shortCount,
                                      int totalCount,
                                      const TimeSignature &timeSignature) const
{
    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: shortCount is "
                         << shortCount << ", fixedWidth is "
                         << fixedWidth << ", barDuration is "
                         << timeSignature.getBarDuration() << endl;

    if (shortest == staff.getNotationElementList()->end()) {
        kdDebug(KDEBUG_AREA) << "First trivial return" << endl;
        return fixedWidth;
    }

    int d = (*shortest)->event()->get<Int>(Quantizer::NoteDurationProperty);
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


void
NotationHLayout::preparse(Staff &staff)
{
    Track &t(staff.getViewElementsManager()->getTrack());
    const Track *timeRef = t.getReferenceTrack();
    if (timeRef == 0) {
	kdDebug(KDEBUG_AREA) << "ERROR: NotationHLayout::preparse: reference track required (at least until code\nis written to render a track without bar lines)" << endl;
	return;
    }

//!!!    const Track::BarPositionList &barPositions(t.getBarPositions());
    NotationElementList *notes = staff.getNotationElementList();

    BarDataList &barList(getBarData(staff));

    Key key;
    Clef clef;
    TimeSignature timeSignature;

    const NotePixmapFactory &npf(staff.getNotePixmapFactory());
    AccidentalTable accTable(key, clef), newAccTable(accTable);

    barList.clear();
    int fixedWidth = staff.getBarMargin();

    Track::iterator refStart = timeRef->findTime(t.getStartIndex());
    Track::iterator refEnd = timeRef->findTime(t.getEndIndex() + 1); //!!!

    int barNo = 0;

    for (Track::iterator refi = refStart; refi != refEnd; ++refi) {

	timeT barStartTime = (*refi)->getAbsoluteTime();
	timeT   barEndTime;

	Track::iterator refi0(refi);
	if (++refi0 != refEnd) {
	    barEndTime = (*refi0)->getAbsoluteTime();
	} else {
//	    break; //!!!
	    barEndTime = t.getEndIndex();
	}

        NotationElementList::iterator from = notes->findTime(barStartTime);
        NotationElementList::iterator to = notes->findTime(barEndTime);

        NotationElementList::iterator shortest = notes->end();
        int shortCount = 0;
        int totalCount = 0;

        for (NotationElementList::iterator it = from; it != to; ++it) {
        
            NotationElement *el = (*it);
            int mw = getMinWidth(npf, *el);

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

            } else if (el->event()->isa(TimeSignature::EventType)) {

		kdDebug(KDEBUG_AREA) << "Found timesig" << endl;

                fixedWidth += mw;
                timeSignature = TimeSignature(*el->event());

            } else if (el->isNote() || el->isRest()) {

                bool hasDuration = true;

                if (el->isNote()) {

                    long pitch = 64;
                    if (!el->event()->get<Int>("pitch", pitch)) {
                        kdDebug(KDEBUG_AREA) <<
                            "WARNING: NotationHLayout::preparse: couldn't get pitch for element, using default pitch of " << pitch << endl;
                    }

                    Accidental explicitAccidental = NoAccidental;
                    if (el->event()->has("accidental")) {
                        explicitAccidental =
                            Rosegarden::NotationDisplayPitch::getAccidentalByName
                            (el->event()->get<String>("accidental"));
                    }

                    Rosegarden::NotationDisplayPitch p
                        (pitch, clef, key, explicitAccidental);
                    int h = p.getHeightOnStaff();
                    Accidental acc = p.getAccidental();

                    el->event()->setMaybe<Int>
                        (Properties::HEIGHT_ON_STAFF, h);

                    el->event()->setMaybe<Int>
                        (Properties::CALCULATED_ACCIDENTAL, acc);

                    el->event()->setMaybe<String>
                        (Properties::NOTE_NAME, p.getAsString(clef, key));

                    // update display acc for note according to the
                    // accTable (accidentals in force when the last
                    // chord ended) and update newAccTable with
                    // accidentals from this note.  (We don't update
                    // accTable because there may be other notes in
                    // this chord that need accTable to be the same as
                    // it is for this one)
                    
                    Accidental dacc = accTable.getDisplayAccidental(acc, h);

                    //                kdDebug(KDEBUG_AREA) << "display accidental = " << dacc << endl;
                    
                    el->event()->setMaybe<Int>
                        (Properties::DISPLAY_ACCIDENTAL, dacc);

                    newAccTable.update(acc, h);

                    if (dacc != acc) {
                        // recalculate min width, as the accidental is
                        // not what we first thought
                        mw = getMinWidth(npf, *el);
                    }

                    Chord chord(*notes, it);
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

                    ++totalCount;
                    
                    int d = 0;
                    try {
                        d = el->event()->get<Int>
                            (Quantizer::NoteDurationProperty);
                    } catch (Event::NoData e) {
                        kdDebug(KDEBUG_AREA) << "No quantized duration in note/rest! event is " << *(el->event()) << endl;
                    }

                    int sd = 0;
                    try {
			if (shortest == notes->end() ||
			    d <= (sd = (*shortest)->event()->get<Int>
				  (Quantizer::NoteDurationProperty))) {
			    if (d == sd) ++shortCount;
			    else {
				kdDebug(KDEBUG_AREA) << "New shortest! Duration is " << d << " (at " << el->getAbsoluteTime() << " time units)"<< endl;
				shortest = it;
				shortCount = 1;
			    }
			}
                    } catch (Event::NoData e) {
                        kdDebug(KDEBUG_AREA) << "No quantized duration in shortest! event is " << *((*shortest)->event()) << endl;
                    }
                }
            }

            el->event()->setMaybe<Int>(Properties::MIN_WIDTH, mw);
        }
        
	timeT actualStart =
	    (from == notes->end() ? t.getEndIndex() :
	                            (*from)->getAbsoluteTime());

        addNewBar(staff, barNo, from,
                  getIdealBarWidth(staff, fixedWidth, shortest, npf,
                                   shortCount, totalCount, timeSignature),
                  fixedWidth, actualStart == barStartTime);

	++barNo;
    }
}


void
NotationHLayout::addNewBar(Staff &staff,
			   int barNo,  NotationElementList::iterator start,
                           int width, int fwidth, bool correct)
{
   m_barData[&staff].push_back
       (BarData(barNo, start, -1, width, fwidth, correct));
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
NotationHLayout::reconcileBars()
{
    // Scoot through the staffs, prepending fake bars to the start of
    // the bar list for any staff that doesn't begin right at the
    // start of the composition

    BarDataMap::iterator i;

    for (i = m_barData.begin(); i != m_barData.end(); ++i) {

        Staff *staff = i->first;
        BarDataList &list = i->second;

        Track &track = staff->getViewElementsManager()->getTrack();
        const Track &refTrack = *(track.getReferenceTrack());

        for (Track::const_iterator j = refTrack.begin();
             j != refTrack.end(); ++j) {

            if ((*j)->getAbsoluteTime() >= track.getStartIndex()) break;
            list.push_front
                (BarData
                 (-1, staff->getNotationElementList()->end(), -1, 0, 0, true));
        }
    }

    // Ensure that concurrent bars on all staffs have the same width,
    // which for now we make the maximum width required for this bar
    // on any staff

    unsigned int barNo = 0;
    bool reachedEnd = false;

    while (!reachedEnd) {

	int maxWidth = -1;
	reachedEnd = true;

	for (i = m_barData.begin(); i != m_barData.end(); ++i) {

	    BarDataList &list = i->second;

	    if (list.size() > barNo) {

		reachedEnd = false;

		if (list[barNo].idealWidth > maxWidth) {
		    maxWidth = i->second[barNo].idealWidth;
		}
	    }
	}

	for (i = m_barData.begin(); i != m_barData.end(); ++i) {

	    BarDataList &list = i->second;

	    if (list.size() > barNo) {

		BarData &bd(list[barNo]);

		if (bd.idealWidth < maxWidth) {
		    if (bd.idealWidth > 0) {
			float ratio = (float)maxWidth / (float)bd.idealWidth;
			bd.fixedWidth += bd.fixedWidth * int((ratio - 1.0)/2.0);
		    }
		    bd.idealWidth = maxWidth;
		}
	    }
	}

	++barNo;
    }
}	

void
NotationHLayout::layout()
{
    for (BarDataMap::iterator i = m_barData.begin(); i != m_barData.end(); ++i)
	layout(i);
}

void
NotationHLayout::layout(BarDataMap::iterator i)
{
    Staff &staff = *(i->first);
    NotationElementList *notes = staff.getNotationElementList();
    BarDataList &barList(getBarData(staff));

    Key key;
    Clef clef;
    TimeSignature timeSignature;

    int x = 0, barX = 0;
    const NotePixmapFactory &npf(staff.getNotePixmapFactory());
    int noteBodyWidth = npf.getNoteBodyWidth();

    int pGroupNo = -1;
    NotationElementList::iterator startOfGroup = notes->end();

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

        kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): starting a bar, barNo is " << bdi->barNo << ", initial x is "
                             << x << ", barX is " << barX << ", and bar width is " << bdi->idealWidth << endl;
        if (from == notes->end()) {
            kdDebug(KDEBUG_AREA) << "Start is end" << endl;
        }
        if (from == to) {
            kdDebug(KDEBUG_AREA) << "Start is to" << endl;
        }

	x = barX;
        bdi->x = x + staff.getBarMargin() / 2;
        x += staff.getBarMargin();
	barX += bdi->idealWidth;

        if (bdi->barNo < 0) continue; // fake bar

        bool haveAccidentalInThisChord = false;

        for (NotationElementList::iterator it = from; it != to; ++it) {
            
            NotationElement *el = (*it);
            el->setLayoutX(x);
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): setting element's x to " << x << endl;

            long delta = el->event()->get<Int>(Properties::MIN_WIDTH);

            if (el->event()->isa(TimeSignature::EventType)) {

		kdDebug(KDEBUG_AREA) << "Found timesig" << endl;

                timeSignature = TimeSignature(*el->event());

            } else if (el->event()->isa(Clef::EventType)) {

		kdDebug(KDEBUG_AREA) << "Found clef" << endl;

                clef = Clef(*el->event());

            } else if (el->event()->isa(Key::EventType)) {

		kdDebug(KDEBUG_AREA) << "Found key" << endl;

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

                // Situate the rest somewhat further into its allotted
                // space.

                if (delta > noteBodyWidth) {
                    int shift = (delta - noteBodyWidth) / 4;
                    shift = std::min(shift, (noteBodyWidth * 4));
                    el->setLayoutX(el->getLayoutX() + shift);
                }
                
                 kdDebug(KDEBUG_AREA) << "Rest idealWidth : "
                                      << bdi->idealWidth
                                      << " - fixedWidth : "
                                      << bdi->fixedWidth << endl;


            } else if (el->isNote()) {


                // To work out how much space to allot a note (or
                // chord), start with the amount alloted to the whole
                // bar, subtract that reserved for fixed-width items,
                // and take the same proportion of the remainder as
                // our duration is of the whole bar's duration.
                    
                delta = ((bdi->idealWidth - bdi->fixedWidth) *
                         el->event()->getDuration()) /
                    //!!! not right for partial bar?
                    timeSignature.getBarDuration();

                // Situate the note somewhat further into its allotted
                // space.

                if (delta > noteBodyWidth) {
                    int shift = (delta - noteBodyWidth) / 5;
                    shift = std::min(shift, (noteBodyWidth * 3));
                    el->setLayoutX(el->getLayoutX() + shift);
                }
                
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
                    if (el->event()->get<Int>
                        (Properties::DISPLAY_ACCIDENTAL, acc0)) {
                        acc = (Accidental)acc0;
                    }
                }
                if (acc != NoAccidental) haveAccidentalInThisChord = true;
                
                Chord chord(*notes, it);
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

                    //                    kdDebug(KDEBUG_AREA) << "This is the final chord element (of " << chord.size() << ")" << endl;

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

                if (el->event()->get<Int>(TrackNotationHelper::BeamedGroupIdPropertyName,
                                          groupNo) &&
                    groupNo != pGroupNo) {
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: entering group " << groupNo << endl;
                    startOfGroup = it;
                }

                long nextGroupNo = -1;
                NotationElementList::iterator it0(it);
                ++it0;
                if (groupNo > -1 &&
                    (it0 == notes->end() ||
                     (!(*it0)->event()->get<Int>
                      (TrackNotationHelper::BeamedGroupIdPropertyName, nextGroupNo) ||
                      nextGroupNo != groupNo))) {
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: about to leave group " << groupNo << ", time to do the sums" << endl;
                
                    NotationGroup group(*notes, it, clef, key);
                    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: group type is " << group.getGroupType() << ", now applying beam" << endl;
                    group.applyBeam(staff);
                }

                pGroupNo = groupNo;
            }

            x += delta;
            kdDebug(KDEBUG_AREA) << "x = " << x << endl;
        }
    }

    if (x > m_totalWidth) m_totalWidth = x;
}


int NotationHLayout::getMinWidth(const NotePixmapFactory &npf,
                                 const NotationElement &e) const
{
    int w = 0;

    if (e.isNote() || e.isRest()) {

        w += npf.getNoteBodyWidth();
        long dots;
        if (e.event()->get<Int>(Rosegarden::Note::NoteDots, dots)) {
            w += npf.getDotWidth() * dots;
        }
        long accidental;
        if (e.event()->get<Int>(Properties::DISPLAY_ACCIDENTAL, accidental) &&
            ((Accidental)accidental != NoAccidental)) {
            w += npf.getAccidentalWidth();
        }
        return w;
    }

    // rather misleadingly, noteMargin is now only used for non-note events

    w = npf.getNoteBodyWidth() / 5;

    if (e.event()->isa(Clef::EventType)) {

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
    m_totalWidth = 0;
}


