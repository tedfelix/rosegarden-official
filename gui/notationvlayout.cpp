
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
    // right: the elements store their pitches, we need their heights.
    // for this we need to use a NotationDisplayPitch object, passing
    // the relevant clef and key, which we need to obtain from the
    // list first

    Clef clef; // default to defaults
    Key key;

    NotationElementList::iterator i;

    i = m_elements.findPrevious(Clef::EventPackage, Clef::EventType, from);
    if (i != m_elements.end()) clef = Clef(*(*i)->event());

    i = m_elements.findPrevious(Key::EventPackage, Key::EventType, from);
    if (i != m_elements.end()) key = Key(*(*i)->event());

    for (i = from; i != to; ++i) {

        NotationElement *el = (*i);

        if (el->isRest()) {

            // all rest pixmaps are sized so that they will be correctly
            // displayed when set to align on the top staff line (height 8)
            el->setY(m_staff.yCoordOfHeight(8));

        } else if (el->isNote()) {

            try {
                int pitch = el->event()->get<Int>("pitch");
                kdDebug(KDEBUG_AREA) << "pitch : " << pitch << endl;
                NotationDisplayPitch p(pitch, clef, key);
                int h = p.getHeightOnStaff();
                el->setY(m_staff.yCoordOfHeight(h));
                el->event()->set<Int>("Notation::Accidental",
                                      (int)p.getAccidental());
                el->event()->set<Bool>("computed-stalk-up", h <= 4); 
                kdDebug(KDEBUG_AREA) << "NotationVLayout::layout : pitch : "
                                     << pitch << " - y : " << el->y() << endl;
            } catch (Event::NoData) {
                kdDebug(KDEBUG_AREA) <<
                    "NotationVLayout::layout : couldn't get pitch for element"
                                     << endl;
                el->setY(0);
            }
        
        } else {

            el->setY(0);
            
            if (el->event()->isa(Clef::EventPackage, Clef::EventType)) {
                clef = Clef(*el->event());
            } else if (el->event()->isa(Key::EventPackage, Key::EventType)) {
                key = Key(*el->event());
            }
        }
    }
}

