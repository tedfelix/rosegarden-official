/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_COMPOSITIONMAPPER_H_
#define _RG_COMPOSITIONMAPPER_H_

#include <map>
#include <QString>

namespace Rosegarden
{

class SegmentMapper;
class MappedEventBuffer;
class Segment;
class RosegardenDocument;


class CompositionMapper
{
    friend class SequenceManager;

public:
    CompositionMapper(RosegardenDocument *doc);
    ~CompositionMapper();

    MappedEventBuffer *getMappedEventBuffer(Segment *);

protected:
    bool segmentModified(Segment *);
    void segmentAdded(Segment *);
    void segmentDeleted(Segment *);

    void mapSegment(Segment *);

    RosegardenDocument *m_doc;

    // I share ownership of the mappers with instances of
    // MappedBufMetaIterator
    typedef std::map<Segment*, SegmentMapper *> segmentmappers;
    segmentmappers m_segmentMappers;
};


}

#endif
