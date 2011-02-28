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


#include "SegmentMapperFactory.h"

#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "misc/Debug.h"
#include "MetronomeMapper.h"
#include "SegmentMapper.h"
#include "AudioSegmentMapper.h"
#include "TempoSegmentMapper.h"
#include "TimeSigSegmentMapper.h"
#include "sound/MappedSegment.h"
#include <QString>


namespace Rosegarden
{
    
SegmentMapper *
SegmentMapperFactory::makeMapperForSegment(RosegardenDocument *doc,
                                           Segment *segment)
{
    SegmentMapper *mapper = 0;

    if (segment == 0) {
        SEQMAN_DEBUG << "SegmentMapperFactory::makeMapperForSegment() segment == 0\n";
        return 0;
    }
    
    MappedSegment *mapped = new MappedSegment;

    switch (segment->getType()) {
    case Segment::Internal :
        mapper = new SegmentMapper(doc, segment, mapped);
        break;
    case Segment::Audio :
        mapper = new AudioSegmentMapper(doc, segment, mapped);
        break;
    default:
        SEQMAN_DEBUG << "SegmentMapperFactory::makeMapperForSegment("
                     << segment
                     << ") : can't map, unknown segment type "
                     << segment->getType() << endl;
        mapper = 0;
    }
    
    if (mapper) mapper->init();
    else delete mapped;

    return mapper;
}

MetronomeMapper *
SegmentMapperFactory::makeMetronome(RosegardenDocument *doc)
{
    MappedSegment *mapped = new MappedSegment;
    MetronomeMapper *mapper = new MetronomeMapper(doc, mapped);
    mapper->init();
    return mapper;
}

TimeSigSegmentMapper *
SegmentMapperFactory::makeTimeSig(RosegardenDocument *doc)
{
    MappedSegment *mapped = new MappedSegment;
    TimeSigSegmentMapper *mapper = new TimeSigSegmentMapper(doc, mapped);
    mapper->init();
    return mapper;
}

TempoSegmentMapper *
SegmentMapperFactory::makeTempo(RosegardenDocument *doc)
{
    MappedSegment *mapped = new MappedSegment;
    TempoSegmentMapper* mapper = new TempoSegmentMapper(doc, mapped);
    mapper->init();
    return mapper;
}
    
}
