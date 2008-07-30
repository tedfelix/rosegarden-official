
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_COMPOSITIONMMAPPER_H_
#define _RG_COMPOSITIONMMAPPER_H_

#include <map>
#include <qstring.h>




namespace Rosegarden
{

class SegmentMmapper;
class Segment;
class RosegardenGUIDoc;


class CompositionMmapper
{
    friend class SequenceManager;

public:
    CompositionMmapper(RosegardenGUIDoc *doc);
    ~CompositionMmapper();

    QString getSegmentFileName(Segment*);
    size_t  getSegmentFileSize(Segment*);

    void cleanup();

protected:
    bool segmentModified(Segment*);
    void segmentAdded(Segment*);
    void segmentDeleted(Segment*);

    void mmapSegment(Segment*);
    QString makeFileName(Segment*);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc* m_doc;
    typedef std::map<Segment*, SegmentMmapper*> segmentmmapers;

    segmentmmapers m_segmentMmappers;
};


}

#endif
