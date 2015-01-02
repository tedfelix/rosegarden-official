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

#ifndef RG_TEMPOSEGMENTMAPPER_H
#define RG_TEMPOSEGMENTMAPPER_H

#include "SpecialSegmentMapper.h"
#include "base/Composition.h"

namespace Rosegarden
{

class RosegardenDocument;
class RealTime;

class TempoSegmentMapper : public SpecialSegmentMapper
{
    friend class SegmentMapperFactory;

protected:
    TempoSegmentMapper(RosegardenDocument *doc) :
        SpecialSegmentMapper(doc) {
    }

    // overrides from SegmentMapper
    virtual int calculateSize();

    // override from SegmentMapper
    virtual void fillBuffer();

    virtual bool shouldPlay(MappedEvent *evt, RealTime /*startTime*/);

    // Map one tempo event
    void mapATempo(RealTime eventTime, tempoT tempo, bool ramping);
    // Map the tempo event at time 0.
    void mapTempoAtZero(Composition& comp);
};

}

#endif
