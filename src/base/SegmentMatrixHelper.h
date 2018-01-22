/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENT_MATRIX_HELPER_H
#define RG_SEGMENT_MATRIX_HELPER_H

#include "SegmentNotationHelper.h"

namespace Rosegarden 
{

class SegmentMatrixHelper : protected SegmentNotationHelper
{
public:
    SegmentMatrixHelper(Segment &t) : SegmentNotationHelper(t) { }

    iterator insertNote(Event *);

    /**
     * Returns true if event is colliding another note in percussion
     * matrix (ie event is a note and has the same start time and the
     * same pitch as another note).
     */
    bool isDrumColliding(Event *);

    using SegmentHelper::segment;
    using SegmentNotationHelper::deleteEvent;
    using SegmentNotationHelper::deleteNote;

};


}

#endif
