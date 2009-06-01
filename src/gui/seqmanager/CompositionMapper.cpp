/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CompositionMapper.h"
#include "misc/Debug.h"

#include <QDir>
#include "base/Composition.h"
#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"
#include "SegmentMapperFactory.h"
#include "SegmentMapper.h"
#include "sound/MappedSegment.h"
#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <stdint.h>


namespace Rosegarden
{

CompositionMapper::CompositionMapper(RosegardenDocument *doc) :
    m_doc(doc)
{
    SEQMAN_DEBUG << "CompositionMapper() - doc = " << doc << endl;

    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); it++) {

        Track *track = comp.getTrackById((*it)->getTrack());

        // check to see if track actually exists
        //
        if (track == 0) continue;

        mapSegment(*it);
    }
}

CompositionMapper::~CompositionMapper()
{
    SEQMAN_DEBUG << "~CompositionMapper()\n";

    for (segmentmappers::iterator i = m_segmentMappers.begin();
         i != m_segmentMappers.end(); ++i) {
        delete i->second;
    }
}

bool
CompositionMapper::segmentModified(Segment *segment)
{
    if (m_segmentMappers.find(segment) == m_segmentMappers.end()) return false;

    SegmentMapper *mapper = m_segmentMappers[segment];

    if (!mapper) {
        return false; // this can happen with the SegmentSplitCommand,
                      // where the new segment's transpose is set even
                      // though it's not mapped yet
    }

    SEQMAN_DEBUG << "CompositionMapper::segmentModified(" << segment << ") - mapper = "
                 << mapper << endl;

    return mapper->refresh();
}

void
CompositionMapper::segmentAdded(Segment *segment)
{
    SEQMAN_DEBUG << "CompositionMapper::segmentAdded(" << segment << ")\n";

    mapSegment(segment);
}

void
CompositionMapper::segmentDeleted(Segment *segment)
{
    SEQMAN_DEBUG << "CompositionMapper::segmentDeleted(" << segment << ")\n";
    if (m_segmentMappers.find(segment) == m_segmentMappers.end()) return;

    SegmentMapper *mapper = m_segmentMappers[segment];
    m_segmentMappers.erase(segment);

    SEQMAN_DEBUG << "CompositionMapper::segmentDeleted() : deleting SegmentMapper " << mapper << endl;

    delete mapper;
}

void
CompositionMapper::mapSegment(Segment *segment)
{
    SEQMAN_DEBUG << "CompositionMapper::mapSegment(" << segment << ")\n";

    SegmentMapper *mapper =
        SegmentMapperFactory::makeMapperForSegment(m_doc, segment);

    if (mapper) {
        m_segmentMappers[segment] = mapper;
    }
}

MappedSegment *
CompositionMapper::getMappedSegment(Segment *s)
{
    if (m_segmentMappers.find(s) != m_segmentMappers.end()) {
        return m_segmentMappers[s]->getMappedSegment();
    } else {
        return 0;
    }
}

}
