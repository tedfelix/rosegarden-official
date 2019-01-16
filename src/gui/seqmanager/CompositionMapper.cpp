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

#define RG_MODULE_STRING "[CompositionMapper]"

#include "CompositionMapper.h"
#include "misc/Debug.h"

#include "base/Composition.h"
#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "gui/seqmanager/MappedEventBuffer.h"
#include "gui/seqmanager/SegmentMapper.h"


namespace Rosegarden
{

CompositionMapper::CompositionMapper(RosegardenDocument *doc) :
    m_doc(doc)
{
    RG_DEBUG << "ctor: doc = " << doc;

    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {

        Track *track = comp.getTrackById((*it)->getTrack());

        // check to see if track actually exists
        //
        if (track == nullptr) continue;

        mapSegment(*it);
    }
}

CompositionMapper::~CompositionMapper()
{
    RG_DEBUG << "dtor";
}

bool
CompositionMapper::segmentModified(Segment *segment)
{
    if (m_segmentMappers.find(segment) == m_segmentMappers.end()) return false;

    QSharedPointer<SegmentMapper> mapper = m_segmentMappers[segment];

    if (!mapper) {
        return false; // this can happen with the SegmentSplitCommand,
                      // where the new segment's transpose is set even
                      // though it's not mapped yet
    }

    RG_DEBUG << "segmentModified(" << segment << ") - mapper = " << mapper;

    return mapper->refresh();
}

void
CompositionMapper::segmentAdded(Segment *segment)
{
    RG_DEBUG << "segmentAdded(" << segment << ")";

    mapSegment(segment);
}

void
CompositionMapper::segmentDeleted(Segment *segment)
{
    RG_DEBUG << "segmentDeleted()";

    // !!! WARNING !!!
    // The segment pointer that is coming in to this routine has already
    // been deleted.  This is a POINTER TO DELETED MEMORY.  It cannot be
    // dereferenced in any way.  Each of the following lines of code will be
    // explained to make it clear that the pointer is not being dereferenced.

    // "segment" is used here as an index into m_segmentMappers.  It is not
    // dereferenced.
    if (m_segmentMappers.find(segment) == m_segmentMappers.end()) return;

    // "segment" is used here as an index into m_segmentMappers.  It is not
    // dereferenced.
    m_segmentMappers.erase(segment);

    // Given that mapper has a pointer to the deleted segment, this line is
    // suspect.  However, I believe there is no operator<< for SegmentMapper.
    // In that case, this should do nothing more than write out the pointer
    // value.  Uncomment this line of code at your own risk.
//    RG_DEBUG << "segmentDeleted() : releasing SegmentMapper " << mapper;
}

void
CompositionMapper::mapSegment(Segment *segment)
{
    RG_DEBUG << "mapSegment(" << segment << ")";
    RG_DEBUG << "  We have" << m_segmentMappers.size() << "segment(s)";

    SegmentMappers::iterator itMapper = m_segmentMappers.find(segment);

    // If it already exists, don't add it but do refresh it.
    if (itMapper != m_segmentMappers.end()) {
        itMapper->second->refresh();
        return;
    }
    QSharedPointer<SegmentMapper> mapper =
        SegmentMapper::makeMapperForSegment(m_doc, segment);

    if (mapper) {
        m_segmentMappers[segment] = mapper;
    }
}

QSharedPointer<MappedEventBuffer>
CompositionMapper::getMappedEventBuffer(Segment *s)
{
    // !!! WARNING !!!
    // The "s" segment pointer that is coming in to this routine may have
    // already been deleted.  This may be a POINTER TO DELETED MEMORY.
    // DO NOT DEREFERENCE IN ANY WAY!

    if (m_segmentMappers.find(s) != m_segmentMappers.end()) {
        return m_segmentMappers[s];
    } else {
        return QSharedPointer<MappedEventBuffer>();
    }
}

}
