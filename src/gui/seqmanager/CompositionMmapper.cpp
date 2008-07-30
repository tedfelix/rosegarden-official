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


#include "CompositionMmapper.h"
#include "misc/Debug.h"

#include <kstddirs.h>
#include "base/Composition.h"
#include "base/Segment.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/application/RosegardenApplication.h"
#include "SegmentMmapperFactory.h"
#include "SegmentMmapper.h"
#include <kglobal.h>
#include <qdir.h>
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <stdint.h>


namespace Rosegarden
{

CompositionMmapper::CompositionMmapper(RosegardenGUIDoc *doc)
        : m_doc(doc)
{
    cleanup();

    SEQMAN_DEBUG << "CompositionMmapper() - doc = " << doc << endl;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); it++) {

        Track* track = comp.getTrackById((*it)->getTrack());

        // check to see if track actually exists
        //
        if (track == 0)
            continue;

        mmapSegment(*it);
    }
}

CompositionMmapper::~CompositionMmapper()
{
    SEQMAN_DEBUG << "~CompositionMmapper()\n";

    //
    // Clean up possible left-overs
    //
    cleanup();

    for (segmentmmapers::iterator i = m_segmentMmappers.begin();
            i != m_segmentMmappers.end(); ++i)
        delete i->second;
}

void CompositionMmapper::cleanup()
{
    // In case the sequencer is still running, mapping some segments
    //
    rgapp->sequencerSend("closeAllSegments()");

    // Erase all 'segment_*' files
    //
    QString tmpPath = KGlobal::dirs()->resourceDirs("tmp").last();

    QDir segmentsDir(tmpPath, "segment_*");
    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        QString segmentName = tmpPath + '/' + segmentsDir[i];
        SEQMAN_DEBUG << "CompositionMmapper : cleaning up " << segmentName << endl;
        QFile::remove
            (segmentName);
    }

}

bool CompositionMmapper::segmentModified(Segment* segment)
{
    SegmentMmapper* mmapper = m_segmentMmappers[segment];

    if (!mmapper)
        return false; // this can happen with the SegmentSplitCommand, where the new segment's transpose is set
    // even though it's not mapped yet

    SEQMAN_DEBUG << "CompositionMmapper::segmentModified(" << segment << ") - mmapper = "
    << mmapper << endl;

    return mmapper->refresh();
}

void CompositionMmapper::segmentAdded(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::segmentAdded(" << segment << ")\n";

    mmapSegment(segment);
}

void CompositionMmapper::segmentDeleted(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::segmentDeleted(" << segment << ")\n";
    SegmentMmapper* mmapper = m_segmentMmappers[segment];
    m_segmentMmappers.erase(segment);
    SEQMAN_DEBUG << "CompositionMmapper::segmentDeleted() : deleting SegmentMmapper " << mmapper << endl;

    delete mmapper;
}

void CompositionMmapper::mmapSegment(Segment* segment)
{
    SEQMAN_DEBUG << "CompositionMmapper::mmapSegment(" << segment << ")\n";

    SegmentMmapper* mmapper = SegmentMmapperFactory::makeMmapperForSegment(m_doc,
                              segment,
                              makeFileName(segment));

    if (mmapper)
        m_segmentMmappers[segment] = mmapper;
}

QString CompositionMmapper::makeFileName(Segment* segment)
{
    QStringList tmpDirs = KGlobal::dirs()->resourceDirs("tmp");

    return QString("%1/segment_%2")
           .arg(tmpDirs.last())
           .arg((uintptr_t)segment, 0, 16);
}

QString CompositionMmapper::getSegmentFileName(Segment* s)
{
    SegmentMmapper* mmapper = m_segmentMmappers[s];

    if (mmapper)
        return mmapper->getFileName();
    else
        return QString::null;
}

size_t CompositionMmapper::getSegmentFileSize(Segment* s)
{
    SegmentMmapper* mmapper = m_segmentMmappers[s];
    
    if (mmapper)
        return mmapper->getFileSize();
    else
        return 0;
}

}
