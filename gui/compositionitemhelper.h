// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef COMPOSITIONITEMHELPER_H
#define COMPOSITIONITEMHELPER_H

#include "compositionitem.h"
#include "compositionview.h"

class CompositionItemHelper {
public:
    static Rosegarden::timeT getStartTime(const CompositionItem&, const Rosegarden::SnapGrid&);
    static Rosegarden::timeT getEndTime(const CompositionItem&, const Rosegarden::SnapGrid&);
    static int getTrackPos(const CompositionItem&, const Rosegarden::SnapGrid&);
    static void setStartTime(CompositionItem&, Rosegarden::timeT, const Rosegarden::SnapGrid&);
    static void setEndTime(CompositionItem&, Rosegarden::timeT, const Rosegarden::SnapGrid&);
    static Rosegarden::Segment* getSegment(CompositionItem);
    static CompositionItem makeCompositionItem(Rosegarden::Segment*);
    /**
     * return the CompositionItem in the model which references the same segment as referenceItem
     */
    static CompositionItem findSiblingCompositionItem(const CompositionModel::itemcontainer& items, const CompositionItem& referenceItem);
};

#endif
