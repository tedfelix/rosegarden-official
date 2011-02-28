
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_COMPOSITIONITEMHELPER_H_
#define _RG_COMPOSITIONITEMHELPER_H_

#include "CompositionModel.h"
#include "base/Event.h"




namespace Rosegarden
{

class SnapGrid;
class Segment;


class CompositionItemHelper {
public:
    static timeT getStartTime(const CompositionItem&, const SnapGrid&);
    static timeT getEndTime(const CompositionItem&, const SnapGrid&);
    static int getTrackPos(const CompositionItem&, const SnapGrid&);
    static void setStartTime(CompositionItem&, timeT, const SnapGrid&);
    static void setEndTime(CompositionItem&, timeT, const SnapGrid&);
    static Segment* getSegment(CompositionItem);
    static CompositionItem makeCompositionItem(Segment*);
    /**
     * return the CompositionItem in the model which references the same segment as referenceItem
     */
    static CompositionItem findSiblingCompositionItem(const CompositionModel::itemcontainer& items, const CompositionItem& referenceItem);

};


}

#endif
