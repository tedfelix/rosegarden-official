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


#include "SpecialSegmentMmapper.h"

#include <kstandarddirs.h>
#include "base/Event.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenGUIDoc.h"
#include "SegmentMmapper.h"
#include <kglobal.h>
#include <QString>


namespace Rosegarden
{

SpecialSegmentMmapper::SpecialSegmentMmapper(RosegardenGUIDoc* doc,
        QString baseFileName)
        : SegmentMmapper(doc, 0, createFileName(baseFileName))
{}

QString SpecialSegmentMmapper::createFileName(QString baseFileName)
{
    return KGlobal::dirs()->resourceDirs("tmp").last() + "/" + baseFileName;
}

unsigned int SpecialSegmentMmapper::getSegmentRepeatCount()
{
    return 1;
}

}
