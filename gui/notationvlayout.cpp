
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
#include "rosedebug.h"
#include "NotationTypes.h"
#include "notepixmapfactory.h"
#include "notationproperties.h"
#include "notationsets.h"

using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::TimeSignature;

NotationVLayout::NotationVLayout(Staff &staff, NotationElementList &elements) :
    m_staff(staff), m_notationElements(elements)
{
    // empty
}

NotationVLayout::~NotationVLayout() { }

void NotationVLayout::reset() { }

void
NotationVLayout::layout(NotationElementList::iterator from,
                        NotationElementList::iterator to)
{ 
    NotationElementList::iterator i;
    const NotePixmapFactory &npf(m_staff.getNotePixmapFactory());

    for (i = from; i != to; ++i) {

        NotationElement *el = (*i);
        el->setLayoutY(0);

        if (el->isRest()) {

            // rest pixmaps are sized so that they will be correctly
            // displayed when set to align on the top staff line
            el->setLayoutY(m_staff.yCoordOfHeight(8));

        } else if (el->isNote()) {

            Chord chord(m_notationElements, i);
            if (chord.size() == 0) continue;

            std::vector<int> h;
            for (unsigned int j = 0; j < chord.size(); ++j) {
                h.push_back((*chord[j])->event()->get<Int>(Properties::HEIGHT_ON_STAFF));
            }
            bool stalkUp = chord.hasStalkUp();

            int tailedNote = (stalkUp ? chord.size() - 1 : 0);

	    for (unsigned int j = 0; j < chord.size(); ++j) {
		el = *chord[j];
		el->setLayoutY(m_staff.yCoordOfHeight(h[j]));

		// we can't only set this if it hasn't already been
		// set, because we may have inserted more notes on the
		// chord since it was last set.  we have to make sure
		// the bit that sets this for beamed groups is called
		// after this bit (i.e. that notationview calls
		// notationhlayout after notationvlayout)... or else
		// introduce two separate properties (beamed stalk up
		// and non-beamed stalk up)
                el->event()->setMaybe<Bool>(Properties::STEM_UP, stalkUp);

                el->event()->setMaybe<Bool>(Properties::NOTE_HEAD_SHIFTED,
                                            chord.isNoteHeadShifted(chord[j]));

                el->event()->setMaybe<Bool>(Properties::DRAW_TAIL,
                                            j == tailedNote);

                int stemLength = -1;
                if (j != tailedNote) {
                    stemLength = m_staff.yCoordOfHeight(h[tailedNote]) -
                        m_staff.yCoordOfHeight(h[j]);
                    if (stemLength < 0) stemLength = -stemLength;
                    kdDebug(KDEBUG_AREA) << "Setting stem length to "
                                         << stemLength << endl;
                }
                el->event()->setMaybe<Int>(Properties::UNBEAMED_STEM_LENGTH,
                                           stemLength);
            }

            i = chord.getFinalElement();
            
        } else {

            if (el->event()->isa(Clef::EventType)) {

                // clef pixmaps are sized so that they will be
                // correctly displayed when set to align one leger
                // line above the top staff line... well, almost
                el->setLayoutY(m_staff.yCoordOfHeight(10) + 1);

            } else if (el->event()->isa(Key::EventType)) {

                el->setLayoutY(m_staff.yCoordOfHeight(12));

            } else if (el->event()->isa(TimeSignature::EventType)) {

                el->setLayoutY(m_staff.yCoordOfHeight(8) + 2);
            }
        }
    }
}

