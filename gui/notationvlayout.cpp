/***************************************************************************
                          notationvlayout.cpp  -  description
                             -------------------
    begin                : Thu Aug 3 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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

    int lookedAt = 0;//!!!

    for (i = from; i != to; ++i) {

        NotationElement *el = (*i);

	kdDebug(KDEBUG_AREA) << "looking at an element" << endl;
	++lookedAt;

        if (el->isRest()) {

            // all rest pixmaps are sized so that they will be correctly
            // displayed when set to align on the top staff line (height 8)
            el->setY(m_staff.yCoordOfHeight(8));

        } else if (el->isNote()) {

            try {
                int pitch = el->event()->get<Int>("pitch");
                kdDebug(KDEBUG_AREA) << "pitch : " << pitch << endl;
                NotationDisplayPitch p(pitch, clef, key);
                el->setY(m_staff.yCoordOfHeight(p.getHeightOnStaff()));
                el->event()->set<Int>("computed-accidental",
                                      (int)p.getAccidental());
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

    
    kdDebug(KDEBUG_AREA) << "vlayout: looked at " << lookedAt << " elements" << endl;
}

