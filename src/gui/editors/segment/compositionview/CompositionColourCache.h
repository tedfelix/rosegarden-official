
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMPOSITIONCOLOURCACHE_H
#define RG_COMPOSITIONCOLOURCACHE_H

#include <QColor>




namespace Rosegarden
{



class CompositionColourCache 
{
public:
    static CompositionColourCache* getInstance();

    void init();

    QColor SegmentCanvas;
    QColor SegmentAudioPreview;
    QColor SegmentInternalPreview;
    QColor SegmentLabel;
    QColor SegmentBorder;
    QColor RepeatSegmentBorder;
    QColor RecordingSegmentBorder;
    QColor RecordingAudioSegmentBlock;
    QColor RecordingInternalSegmentBlock;
    QColor Pointer;
    QColor MovementGuide;
    QColor RotaryFloatBackground;
    QColor RotaryFloatForeground;

protected:
    CompositionColourCache() { init(); }
    static CompositionColourCache* m_instance;
};



}

#endif
