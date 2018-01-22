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


#include "CompositionColourCache.h"

#include "gui/general/GUIPalette.h"
#include <QColor>


namespace Rosegarden
{

void CompositionColourCache::init()
{
    SegmentCanvas = GUIPalette::getColour(GUIPalette::SegmentCanvas);
    SegmentAudioPreview = GUIPalette::getColour(GUIPalette::SegmentAudioPreview);
    SegmentInternalPreview = GUIPalette::getColour(GUIPalette::SegmentInternalPreview);
    SegmentLabel = GUIPalette::getColour(GUIPalette::SegmentLabel);
    SegmentBorder = GUIPalette::getColour(GUIPalette::SegmentBorder);
    RepeatSegmentBorder = GUIPalette::getColour(GUIPalette::RepeatSegmentBorder);
    RecordingSegmentBorder = GUIPalette::getColour(GUIPalette::RecordingSegmentBorder);
    RecordingAudioSegmentBlock = GUIPalette::getColour(GUIPalette::RecordingAudioSegmentBlock);
    RecordingInternalSegmentBlock = GUIPalette::getColour(GUIPalette::RecordingInternalSegmentBlock);
    RotaryFloatBackground = GUIPalette::getColour(GUIPalette::RotaryFloatBackground);
    RotaryFloatForeground = GUIPalette::getColour(GUIPalette::RotaryFloatForeground);

}

CompositionColourCache* CompositionColourCache::getInstance()
{
    if (!m_instance) {
        m_instance = new CompositionColourCache();
    }

    return m_instance;
}

CompositionColourCache* CompositionColourCache::m_instance = 0;

}
