
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

using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::TimeSignature;

NotationVLayout::NotationVLayout(Staff &staff, NotationElementList &elements) :
    m_staff(staff), m_elements(elements)
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

            Chord chord(m_elements, i);
            if (chord.size() == 0) continue;

            std::vector<int> h;
            for (unsigned int j = 0; j < chord.size(); ++j) {
                h.push_back((*chord[j])->event()->get<Int>(P_HEIGHT_ON_STAFF));
            }
            int top = h.size()-1;

            bool stalkUp = true;
            if (h[top] > 4) {
                if (h[0] > 4) stalkUp = false;
                else stalkUp = (h[top] - 4) < (5 - h[0]);
            }

            for (unsigned int j = 0; j < chord.size(); ++j) {
                el = *chord[j];
                try {

                    el->setLayoutY(m_staff.yCoordOfHeight(h[j]));
                    el->event()->setMaybe<Bool>(P_STALK_UP, stalkUp);

                    el->event()->setMaybe<Bool>
                        (P_DRAW_TAIL,
                         ((stalkUp && j == chord.size()-1) ||
                          (!stalkUp && j == 0)));

                } catch (Event::NoData) {
                    kdDebug(KDEBUG_AREA) <<
                        "NotationVLayout::layout : couldn't get properties for element (has NotationHLayout::preparse run?)" << endl;
                }
            }

            i = chord.getFinalNote();
            
/*!
            NotationElementList::iterator inext(i);
            Event::timeT d; // unwanted
            vector<NotationElementList::iterator> notes =
                m_elements.findSucceedingChordElements(i, inext, d);
            if (notes.size() == 0) continue;

            vector<int> h;
            for (unsigned int j = 0; j < notes.size(); ++j) {
                h.push_back((*notes[j])->event()->get<Int>(P_HEIGHT_ON_STAFF));
            }
            int top = h.size()-1;

            bool stalkUp = true;
            if (h[top] > 4) {
                if (h[0] > 4) stalkUp = false;
                else stalkUp = (h[top] - 4) < (5 - h[0]);
            }

            for (unsigned int j = 0; j < notes.size(); ++j) {
                el = *notes[j];
                try {

                    el->setLayoutY(m_staff.yCoordOfHeight(h[j]));
                    el->event()->setMaybe<Bool>(P_STALK_UP, stalkUp);

                    el->event()->setMaybe<Bool>
                        (P_DRAW_TAIL,
                         ((stalkUp && j == notes.size()-1) ||
                          (!stalkUp && j == 0)));

                } catch (Event::NoData) {
                    kdDebug(KDEBUG_AREA) <<
                        "NotationVLayout::layout : couldn't get properties for element (has NotationHLayout::preparse run?)" << endl;
                }
            }

            i = inext;
*/        
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

