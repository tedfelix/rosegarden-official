/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MatrixVLayout.h"
#include "misc/Debug.h"

#include "base/BaseProperties.h"
#include "base/LayoutEngine.h"
#include "base/Staff.h"
#include "MatrixElement.h"
#include "MatrixStaff.h"


namespace Rosegarden
{

MatrixVLayout::MatrixVLayout()
{}

MatrixVLayout::~MatrixVLayout()
{}

void MatrixVLayout::reset()
{}

void MatrixVLayout::resetStaff(Staff&, timeT, timeT)
{}

void MatrixVLayout::scanStaff(Staff& staffBase,
                              timeT startTime, timeT endTime)
{
    MatrixStaff& staff = dynamic_cast<MatrixStaff&>(staffBase);

    using BaseProperties::PITCH;

    MatrixElementList *notes = staff.getViewElementList();

    MatrixElementList::iterator from = notes->begin();
    MatrixElementList::iterator to = notes->end();
    MatrixElementList::iterator i;

    if (startTime != endTime) {
        from = notes->findNearestTime(startTime);
        if (from == notes->end())
            from = notes->begin();
        to = notes->findTime(endTime);
    }

    MATRIX_DEBUG << "MatrixVLayout::scanStaff : id = "
    << staff.getId() << endl;


    for (i = from; i != to; ++i) {

        MatrixElement *el = dynamic_cast<MatrixElement*>((*i));

        if (!el->isNote())
            continue; // notes only

        long pitch = 60;
        el->event()->get
        <Int>(PITCH, pitch);

        int y = staff.getLayoutYForHeight(pitch) - staff.getElementHeight() / 2;

        el->setLayoutY(y);
        el->setHeight(staff.getElementHeight());
    }

}

void MatrixVLayout::finishLayout(timeT, timeT)
{}

const int MatrixVLayout::minMIDIPitch = 0;
const int MatrixVLayout::maxMIDIPitch = 127;

}
