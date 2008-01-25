/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentMmapperFactory.h"

#include "base/Segment.h"
#include "document/RosegardenGUIDoc.h"
#include "misc/Debug.h"
#include "MetronomeMmapper.h"
#include "SegmentMmapper.h"
#include "AudioSegmentMmapper.h"
#include "TempoSegmentMmapper.h"
#include "TimeSigSegmentMmapper.h"
#include <qstring.h>


namespace Rosegarden
{
    
SegmentMmapper* SegmentMmapperFactory::makeMmapperForSegment(RosegardenGUIDoc* doc,
                                                             Rosegarden::Segment* segment,
                                                             const QString& fileName)
{
    SegmentMmapper* mmapper = 0;

    if (segment == 0) {
        SEQMAN_DEBUG << "SegmentMmapperFactory::makeMmapperForSegment() segment == 0\n";
        return 0;
    }
    
    switch (segment->getType()) {
    case Segment::Internal :
        mmapper = new SegmentMmapper(doc, segment, fileName);
        break;
    case Segment::Audio :
        mmapper = new AudioSegmentMmapper(doc, segment, fileName);
        break;
    default:
        SEQMAN_DEBUG << "SegmentMmapperFactory::makeMmapperForSegment(" << segment
                     << ") : can't map, unknown segment type " << segment->getType() << endl;
        mmapper = 0;
    }
    
    if (mmapper)
        mmapper->init();

    return mmapper;
}

MetronomeMmapper* SegmentMmapperFactory::makeMetronome(RosegardenGUIDoc* doc)
{
    MetronomeMmapper* mmapper = new MetronomeMmapper(doc);
    mmapper->init();
    
    return mmapper;
}

TimeSigSegmentMmapper* SegmentMmapperFactory::makeTimeSig(RosegardenGUIDoc* doc)
{
    TimeSigSegmentMmapper* mmapper = new TimeSigSegmentMmapper(doc, "rosegarden_timesig");
    
    mmapper->init();
    return mmapper;
}

TempoSegmentMmapper* SegmentMmapperFactory::makeTempo(RosegardenGUIDoc* doc)
{
    TempoSegmentMmapper* mmapper = new TempoSegmentMmapper(doc, "rosegarden_tempo");
    
    mmapper->init();
    return mmapper;
}
    
}
