// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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


#include "colours.h"


namespace RosegardenGUIColours
{
    const QColor ActiveRecordTrack = Qt::red;

    const QColor SegmentCanvas = QColor(230, 230, 230);
    const QColor SegmentBlock = QColor(197, 211, 125);
    const QColor SegmentBorder = Qt::black;
    const QColor SegmentHighlightBlock = QColor(98, 102, 78);
    const QColor SegmentIntersectBlock = SegmentBlock.dark(150);
    const QColor RecordingSegmentBlock = QColor(255, 182, 193);
    const QColor RecordingSegmentBorder = Qt::black;

    const QColor RepeatSegmentBlock = QColor(238, 238, 205);
    const QColor RepeatSegmentBorder = QColor(130, 133, 170);

    const QColor SegmentAudioPreview = QColor(39, 71, 22);
    const QColor SegmentInternalPreview = Qt::white;
    const QColor SegmentLabel = Qt::black;
    const QColor SegmentSplitLine = Qt::black;

    const QColor MatrixElementBorder = Qt::black;
    const QColor MatrixElementBlock = QColor(98, 128, 232);

    const QColor LoopRulerBackground = QColor(120, 120, 120);
    const QColor LoopRulerForeground = Qt::white;
    const QColor LoopHighlight = Qt::white;
  
    //const QColor TextRulerBackground = QColor(60, 205, 230, QColor::Hsv);
//    const QColor TextRulerBackground = QColor(120, 90, 238, QColor::Hsv);
//    const QColor TextRulerBackground = QColor(210, 220, 140);
    const QColor TextRulerBackground = QColor(226, 232, 187);
    const QColor TextRulerForeground = Qt::white;

    const QColor ChordNameRulerBackground = QColor(230, 230, 230);
    const QColor ChordNameRulerForeground = Qt::black;

    const QColor LevelMeterGreen = QColor(0, 200, 0);
    const QColor LevelMeterOrange = QColor(255, 165, 0);
    const QColor LevelMeterRed = QColor(200, 0, 0);

    const QColor BarLine = Qt::black;
    const QColor BarLineIncorrect = QColor(211, 0, 31);
    const QColor BeatLine = QColor(212, 212, 212);
    const QColor StaffConnectingLine = QColor(192, 192, 192);
    const QColor StaffConnectingTerminatingLine = QColor(128, 128, 128);

    const QColor Pointer = Qt::darkBlue;
    const QColor PointerRuler = QColor(100, 100, 100);

    const QColor InsertCursor = QColor(160, 104, 186);
    const QColor InsertCursorRuler = QColor(160, 136, 170);

    const QColor SelectionRectangle = QColor(103, 128, 211);
    const QColor SelectedElement = QColor(0, 54, 232);

    const int SelectedElementHue = 225;
    const int SelectedElementMinValue = 220;
    const int HighlightedElementHue = 25;
    const int HighlightedElementMinValue = 220;
    const int QuantizedNoteHue = 69;
    const int QuantizedNoteMinValue = 140;

    const QColor TextAnnotationBackground = QColor(255, 255, 180);
}


