// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "compositioncolourcache.h"

#include "colours.h"

using Rosegarden::GUIPalette;

void CompositionColourCache::init()
{
    SegmentCanvas                 = GUIPalette::getColour(GUIPalette::SegmentCanvas);
    SegmentAudioPreview           = GUIPalette::getColour(GUIPalette::SegmentAudioPreview);
    SegmentInternalPreview        = GUIPalette::getColour(GUIPalette::SegmentInternalPreview);
    SegmentLabel                  = GUIPalette::getColour(GUIPalette::SegmentLabel);
    SegmentBorder                 = GUIPalette::getColour(GUIPalette::SegmentBorder);
    RepeatSegmentBorder           = GUIPalette::getColour(GUIPalette::RepeatSegmentBorder);
    RecordingSegmentBorder        = GUIPalette::getColour(GUIPalette::RecordingSegmentBorder);
    RecordingAudioSegmentBlock    = GUIPalette::getColour(GUIPalette::RecordingAudioSegmentBlock);
    RecordingInternalSegmentBlock = GUIPalette::getColour(GUIPalette::RecordingInternalSegmentBlock);
    RotaryFloatForeground         = GUIPalette::getColour(GUIPalette::RotaryFloatForeground);
    
}

CompositionColourCache* CompositionColourCache::getInstance()
{
    if (!m_instance) {
        m_instance = new CompositionColourCache();
    }

    return m_instance;
}

CompositionColourCache* CompositionColourCache::m_instance = 0;
