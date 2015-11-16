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

#define RG_MODULE_STRING "[SegmentMapperFactory]"

#include "SegmentMapperFactory.h"

#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "misc/Debug.h"
#include "gui/seqmanager/AudioSegmentMapper.h"
#include "gui/seqmanager/InternalSegmentMapper.h"
#include "gui/seqmanager/MappedEventBuffer.h"
#include "gui/seqmanager/SegmentMapper.h"

#include <QString>


namespace Rosegarden
{
    
SegmentMapper *
SegmentMapperFactory::makeMapperForSegment(RosegardenDocument *doc,
                                           Segment *segment)
{
    SegmentMapper *mapper = 0;

    if (segment == 0) {
        RG_DEBUG << "makeMapperForSegment() segment == 0";
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
        RG_DEBUG << "makeMapperForSegment(" << segment << ") : can't map, unknown segment type " << segment->getType();
        mapper = 0;
    }

    // ??? InternalSegmentMapper and AudioSegmentMapper's ctors should
    //     call init().
    if (mapper)
        mapper->init();

    return mapper;
}

}
