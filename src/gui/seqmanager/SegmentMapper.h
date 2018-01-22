/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTMAPPER_H
#define RG_SEGMENTMAPPER_H

#include <QString>
#include "base/Event.h"
#include "gui/seqmanager/MappedEventBuffer.h"

namespace Rosegarden
{

class Segment;
class RosegardenDocument;

class SegmentMapper : public MappedEventBuffer
{

public:
    virtual ~SegmentMapper();

    /// Create the appropriate mapper for the segment type.  Factory function.
    static SegmentMapper *makeMapperForSegment(RosegardenDocument *, Segment *);

    virtual int getSegmentRepeatCount();
    virtual TrackId getTrackID() const;

    virtual void initSpecial(void);

protected:
    SegmentMapper(RosegardenDocument *, Segment *);

    bool mutedEtc(void);

    //--------------- Data members ---------------------------------
    Segment *m_segment;
};



}

#endif
