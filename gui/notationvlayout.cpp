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

#include "notationvlayout.h"
#include "notationstaff.h"
#include "rosedebug.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "notepixmapfactory.h"
#include "notationproperties.h"
#include "notationsets.h"

// I woke up on the last day of the year
// with the sudden realisation
// that people have brought terrible ills upon themselves by
// trying to cover the earth with fields in shapes such as squares
// which don't tesselate when mapped onto curved surfaces.
// War and famine would cease, if only we could all
// move at once onto a system of triangles.

using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::TimeSignature;
using Rosegarden::Note;
using Rosegarden::Mark;
using Rosegarden::timeT;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

NotationVLayout::NotationVLayout()
{
    // empty
}

NotationVLayout::~NotationVLayout()
{
    // empty
}

NotationVLayout::SlurList &
NotationVLayout::getSlurList(StaffType &staff)
{
    SlurListMap::iterator i = m_slurs.find(&staff);
    if (i == m_slurs.end()) {
	m_slurs[&staff] = SlurList();
    }

    return m_slurs[&staff];
}

void
NotationVLayout::reset()
{
    m_slurs.clear();
}

void
NotationVLayout::resetStaff(StaffType &staff)
{
    getSlurList(staff).clear();
}

void
NotationVLayout::scanStaff(StaffType &staffBase)
{ 
    NotationStaff &staff = dynamic_cast<NotationStaff &>(staffBase);
    NotationElementList *notes = staff.getViewElementList();

    NotationElementList::iterator from = notes->begin();
    NotationElementList::iterator to = notes->end();
    NotationElementList::iterator i;

    for (i = from; i != to; ++i) {

        NotationElement *el = (*i);
        el->setLayoutY(0);

        if (el->isRest()) {

            // rests for notes longer than the minim have hotspots
            // aligned with the line above the middle line; the rest
            // are aligned with the middle line

            int noteType = el->event()->get<Int>(NOTE_TYPE);
            if (noteType > Note::Minim) {
                el->setLayoutY(staff.yCoordOfHeight(6));
            } else {
                el->setLayoutY(staff.yCoordOfHeight(4));
            }

        } else if (el->isNote()) {

            Chord chord(*notes, i);
            if (chord.size() == 0) continue;

            std::vector<int> h;
            for (unsigned int j = 0; j < chord.size(); ++j) {
                h.push_back((*chord[j])->event()->get<Int>(HEIGHT_ON_STAFF));
            }
            bool stemUp = chord.hasStemUp();
            bool hasNoteHeadShifted = chord.hasNoteHeadShifted();

            unsigned int flaggedNote = (stemUp ? chord.size() - 1 : 0);

	    for (unsigned int j = 0; j < chord.size(); ++j) {
		el = *chord[j];
		el->setLayoutY(staff.yCoordOfHeight(h[j]));

		// we can't only set this if it hasn't already been
		// set, because we may have inserted more notes on the
		// chord since it was last set.  we have to make sure
		// the bit that sets this for beamed groups is called
		// after this bit (i.e. that notationview calls
		// notationhlayout after notationvlayout)... or else
		// introduce two separate properties (beamed stem up
		// and non-beamed stem up)
                el->event()->setMaybe<Bool>(STEM_UP, stemUp);

		bool primary = (stemUp ? (j == 0) : (j == chord.size()-1));
                el->event()->setMaybe<Bool>(CHORD_PRIMARY_NOTE, primary);

                el->event()->setMaybe<Bool>(NOTE_HEAD_SHIFTED,
                                            chord.isNoteHeadShifted(chord[j]));

                el->event()->setMaybe<Bool>(NEEDS_EXTRA_SHIFT_SPACE,
                                            hasNoteHeadShifted && !stemUp);

                el->event()->setMaybe<Bool>(DRAW_FLAG,
                                            j == flaggedNote);

                int stemLength = -1;
                if (j != flaggedNote) {
                    stemLength = staff.yCoordOfHeight(h[flaggedNote]) -
                        staff.yCoordOfHeight(h[j]);
                    if (stemLength < 0) stemLength = -stemLength;
                    kdDebug(KDEBUG_AREA) << "Setting stem length to "
                                         << stemLength << endl;
                }
                el->event()->setMaybe<Int>(UNBEAMED_STEM_LENGTH,
                                           stemLength);
            }

            i = chord.getFinalElement();
            
        } else {

            if (el->event()->isa(Clef::EventType)) {

                // clef pixmaps have the hotspot placed to coincide
                // with the pitch of the clef -- so the alto clef
                // should be "on" the middle line, the treble clef
                // "on" the line below the middle, etc

                int height;
                Clef clef(*el->event());

                if (clef.getClefType() == Clef::Treble) {
                    height = 2;
                } else if (clef.getClefType() == Clef::Bass) {
                    height = 6;
                } else if (clef.getClefType() == Clef::Tenor) {
                    height = 6;
                } else {
                    height = 4;
                }

                el->setLayoutY(staff.yCoordOfHeight(height));

            } else if (el->event()->isa(Key::EventType)) {

                el->setLayoutY(staff.yCoordOfHeight(12));

            } else if (el->event()->isa(Mark::EventType)) {

		std::string markType =
		    el->event()->get<String>(Mark::MarkTypePropertyName);

		if (markType == Mark::Slur) {
		    getSlurList(staff).push_back(i);
		}

		el->setLayoutY(staff.yCoordOfHeight(-9));
	    }
        }
    }
}

void
NotationVLayout::finishLayout()
{
    for (SlurListMap::iterator mi = m_slurs.begin();
	 mi != m_slurs.end(); ++mi) {

	for (SlurList::iterator si = mi->second.begin();
	     si != mi->second.end(); ++si) {

	    NotationElementList::iterator i = *si;
	    NotationElementList::iterator scooter = i;
	    NotationStaff &staff = dynamic_cast<NotationStaff &>(*(mi->first));

	    // We always position the slur based on the notes' positional
	    // properties, never the other way around.  So we need to know
	    // which direction the tails are going in, particularly if
	    // there are beams involved.  (It'd be nice to be flexible
	    // enough to be able to change the tail directions if they
	    // were not particularly important and there was a slur to
	    // accommodate in a difficult position, but I think that's an
	    // uncommon case.)

	    timeT slurDuration =
		(*i)->event()->get<Int>(Mark::MarkDurationPropertyName);
	    timeT endTime = (*i)->getAbsoluteTime() + slurDuration;

	    bool haveStart = false;
	    int startHeight = 4, endHeight = startHeight;
	    int startX = (*i)->getLayoutX(), endX = startX + 10;
	    bool startStemUp = false, endStemUp = false;
	    bool beamAbove = false, beamBelow = false;

	    while (scooter != staff.getViewElementList()->end()) {

		if ((*scooter)->getAbsoluteTime() >= endTime) break;

		if ((*scooter)->isNote()) {

		    bool primary = false;

		    if ((*scooter)->event()->get<Bool>(CHORD_PRIMARY_NOTE, primary)
			&& primary) {

			int h = (*scooter)->event()->get<Int>(HEIGHT_ON_STAFF);

			bool stemUp = (h <= 4);
			(*scooter)->event()->get<Bool>(STEM_UP, stemUp);

			bool beamed = false;
			(*scooter)->event()->get<Bool>(BEAMED, beamed);

			if (beamed) {
			    if (stemUp) beamAbove = true;
			    else beamBelow = true;
			}

			if (!haveStart) {
			    startX = (*scooter)->getLayoutX();
			    startHeight = h;
			    startStemUp = stemUp;
			    haveStart = true;
			}

			endX = (*scooter)->getLayoutX();
			endHeight = h;
			endStemUp = stemUp;
		    }
		}

		++scooter;
	    }

	    bool above = true;
	    if (startStemUp == endStemUp) {
		above = !startStemUp;
	    } else if (beamBelow) {
		above = true;
	    } else if (beamAbove) {
		above = false;
	    } else {
		above = (startHeight + endHeight <= 8);
	    }

	    if (above) {
		startHeight += 3;
		endHeight += 3;
	    } else {
		startHeight -= 3;
		endHeight -= 3;
	    }

	    int y0 = staff.yCoordOfHeight(startHeight),
		y1 = staff.yCoordOfHeight(endHeight);

	    int dy = y1 - y0;
	    if (above) {
		if (dy > 0) dy = dy * 3 / 4;
	    } else {
		if (dy < 0) dy = dy * 3 / 4;
	    }

	    (*i)->event()->set<Bool>(SLUR_ABOVE, above);
	    (*i)->event()->set<Int>(SLUR_Y_DELTA, (y1 - y0) * 3 / 4);
	    (*i)->event()->set<Int>(SLUR_LENGTH, (endX - startX));
	    (*i)->setLayoutX(startX);
	    (*i)->setLayoutY(y0);
	}
    }
}


