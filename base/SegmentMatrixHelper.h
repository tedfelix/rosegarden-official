// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _SEGMENT_MATRIX_HELPER_H_
#define _SEGMENT_MATRIX_HELPER_H_

#include "SegmentNotationHelper.h"

namespace Rosegarden 
{

class SegmentMatrixHelper : protected SegmentNotationHelper
{
public:
    SegmentMatrixHelper(Segment &t) : SegmentNotationHelper(t) { }

    iterator insertNote(Event *);

    using SegmentHelper::segment;
    using SegmentNotationHelper::deleteEvent;
    using SegmentNotationHelper::deleteNote;
};


}

#endif
