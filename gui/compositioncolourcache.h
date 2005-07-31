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

#ifndef COMPOSITIONCOLOURCACHE_H
#define COMPOSITIONCOLOURCACHE_H

#include <qcolor.h>

class CompositionColourCache 
{
public:
    static CompositionColourCache* getInstance();

    void init();

    QColor SegmentCanvas;
    QColor SegmentAudioPreview;
    QColor SegmentLabel;
    QColor SegmentBorder;
    QColor RepeatSegmentBorder;
    QColor RecordingSegmentBorder;
    QColor RecordingAudioSegmentBlock;
    QColor RecordingInternalSegmentBlock;
    QColor Pointer;
    QColor MovementGuide;
    QColor RotaryFloatForeground;

protected:
    CompositionColourCache() { init(); }
    static CompositionColourCache* m_instance;
};


#endif
