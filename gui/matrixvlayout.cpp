// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "BaseProperties.h"

#include "matrixvlayout.h"
#include "matrixstaff.h"

#include "rosedebug.h"

using Rosegarden::timeT;

MatrixVLayout::MatrixVLayout()
{
}

MatrixVLayout::~MatrixVLayout()
{
}

void MatrixVLayout::reset()
{
}

void MatrixVLayout::resetStaff(StaffType&, timeT, timeT)
{
}

void MatrixVLayout::scanStaff(MatrixVLayout::StaffType& staffBase, timeT, timeT)
{
    MatrixStaff& staff = dynamic_cast<MatrixStaff&>(staffBase);

    using Rosegarden::BaseProperties::PITCH;

    MatrixElementList *notes = staff.getViewElementList();

    MatrixElementList::iterator from = notes->begin();
    MatrixElementList::iterator to = notes->end();
    MatrixElementList::iterator i;

    kdDebug(KDEBUG_AREA) << "MatrixVLayout::scanStaff : id = "
                         << staff.getId() << endl;


    for (i = from; i != to; ++i) {

        MatrixElement *el = (*i);

        if (!el->isNote()) continue; // notes only
        
        int pitch = el->event()->get<Rosegarden::Int>(PITCH);

	int y = staff.getLayoutYForHeight(pitch) - staff.getElementHeight()/2;

        el->setLayoutY(y);
        el->setHeight(staff.getElementHeight());
    }

}

void MatrixVLayout::finishLayout(timeT, timeT)
{
}

const unsigned int MatrixVLayout::maxMIDIPitch = 127;
