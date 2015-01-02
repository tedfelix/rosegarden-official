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


#include "SegmentMapperFactory.h"

#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "misc/Debug.h"
#include "gui/seqmanager/AudioSegmentMapper.h"
#include "gui/seqmanager/InternalSegmentMapper.h"
#include "gui/seqmanager/MarkerMapper.h"
#include "gui/seqmanager/MappedEventBuffer.h"
#include "gui/seqmanager/MetronomeMapper.h"
#include "gui/seqmanager/SegmentMapper.h"
#include "gui/seqmanager/TempoSegmentMapper.h"
#include "gui/seqmanager/TimeSigSegmentMapper.h"
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
    
    switch (segment->getType()) {
    case Segment::Internal :
        mapper = new InternalSegmentMapper(doc, segment);
        break;
    case Segment::Audio :
        mapper = new AudioSegmentMapper(doc, segment);
        break;
    default:
        SEQMAN_DEBUG << "SegmentMapperFactory::makeMapperForSegment("
                     << segment
                     << ") : can't map, unknown segment type "
                     << segment->getType() << endl;
        mapper = 0;
    }

    if (mapper) { mapper->init(); }

    return mapper;
}

MetronomeMapper *
SegmentMapperFactory::makeMetronome(RosegardenDocument *doc)
{
    MetronomeMapper *mapper = new MetronomeMapper(doc);
    mapper->init();
    return mapper;
}

TimeSigSegmentMapper *
SegmentMapperFactory::makeTimeSig(RosegardenDocument *doc)
{
    TimeSigSegmentMapper *mapper = new TimeSigSegmentMapper(doc);
    mapper->init();
    return mapper;
}

TempoSegmentMapper *
SegmentMapperFactory::makeTempo(RosegardenDocument *doc)
{
    TempoSegmentMapper* mapper = new TempoSegmentMapper(doc);
    mapper->init();
    return mapper;
}

MarkerMapper *
SegmentMapperFactory::makeMarker(RosegardenDocument *doc)
{
    MarkerMapper* mapper = new MarkerMapper(doc);
    mapper->init();
    return mapper;
}

}
