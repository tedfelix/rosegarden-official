
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

#ifdef NOT_DEFINED // should no longer be needed (now we preparse)
    // right: the elements store their pitches, we need their heights.
    // for this we need to use a NotationDisplayPitch object, passing
    // the relevant clef and key, which we need to obtain from the
    // list first

    Clef clef; // default to defaults
    Key key;

    i = m_elements.findPrevious(Clef::EventPackage, Clef::EventType, from);
    if (i != m_elements.end()) clef = Clef(*(*i)->event());

    i = m_elements.findPrevious(Key::EventPackage, Key::EventType, from);
    if (i != m_elements.end()) key = Key(*(*i)->event());
#endif

    for (i = from; i != to; ++i) {

        NotationElement *el = (*i);
        el->setLayoutY(0);

        if (el->isRest()) {

            // rest pixmaps are sized so that they will be correctly
            // displayed when set to align on the top staff line
            el->setLayoutY(m_staff.yCoordOfHeight(8));

        } else if (el->isNote()) {

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
                    el->event()->set<Bool>(P_STALK_UP, stalkUp);

                    el->event()->set<Bool>
                        (P_DRAW_TAIL,
                         ((stalkUp && j == notes.size()-1) ||
                          (!stalkUp && j == 0)));

                } catch (Event::NoData) {
                    kdDebug(KDEBUG_AREA) <<
                        "NotationVLayout::layout : couldn't get properties for element (has NotationHLayout::preparse run?)" << endl;
                }
            }

            i = inext;
        
        } else {

            if (el->event()->isa(Clef::EventPackage, Clef::EventType)) {

                // clef pixmaps are sized so that they will be
                // correctly displayed when set to align one leger
                // line above the top staff line... well, almost
                el->setLayoutY(m_staff.yCoordOfHeight(10) + 1);
//                clef = Clef(*el->event());

            } else if (el->event()->isa(Key::EventPackage, Key::EventType)) {

                el->setLayoutY(m_staff.yCoordOfHeight(12));
//                key = Key(*el->event());
            }
        }
    }
}

