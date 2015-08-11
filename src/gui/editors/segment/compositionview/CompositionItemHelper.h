
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMPOSITIONITEMHELPER_H
#define RG_COMPOSITIONITEMHELPER_H

#include "CompositionModelImpl.h"
#include "base/Event.h"

namespace Rosegarden
{


class SnapGrid;
class Segment;

/// ??? Get rid of this class.  Move all functions into CompositionItem.
class CompositionItemHelper {
public:
    static int getTrackPos(CompositionItemPtr, const SnapGrid&);

    // ??? This returns an object that must be deleted by the caller.
    //     Audit all callers to make sure they delete this object.
    static CompositionItemPtr makeCompositionItem(Segment*);
    /// return the CompositionItemPtr in the model which references the same segment as referenceItem
    static CompositionItemPtr findSiblingCompositionItem(
            const CompositionModelImpl::ChangingSegmentSet &items,
            CompositionItemPtr referenceItem);
};


}

#endif
