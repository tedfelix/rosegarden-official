
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _COLOURS_H_
#define _COLOURS_H_

#include <qcolor.h>

/**
 * Definitions of colours to be used throughout the Rosegarden GUI.
 * 
 * They're in this file not so much to allow them to be easily
 * changed, as to ensure a certain amount of consistency between
 * colours for different functions that might end up being seen
 * at the same time.
 */

namespace RosegardenGUIColours
{
    extern const QColor ActiveRecordTrack;

    extern const QColor SegmentCanvas;
    extern const QColor SegmentBorder;
    extern const QColor SegmentBlock;
    extern const QColor SegmentHighlightBlock;
    extern const QColor SegmentIntersectBlock;

    extern const QColor RecordingSegmentBlock;
    extern const QColor RecordingSegmentBorder;

    extern const QColor RepeatSegmentBlock;
    extern const QColor RepeatSegmentBorder;

    extern const QColor SegmentAudioPreview;
    extern const QColor SegmentInternalPreview;
    extern const QColor SegmentLabel;
    extern const QColor SegmentSplitLine;

    extern const QColor MatrixElementBorder;
    extern const QColor MatrixElementBlock;

    extern const QColor LoopRulerBackground;
    extern const QColor LoopRulerForeground;
    extern const QColor LoopHighlight;

    extern const QColor TextRulerBackground;
    extern const QColor TextRulerForeground;
  
    extern const QColor ChordNameRulerBackground;
    extern const QColor ChordNameRulerForeground;

    extern const QColor RawNoteRulerBackground;
    extern const QColor RawNoteRulerForeground;
  
    extern const QColor LevelMeterGreen;
    extern const QColor LevelMeterOrange;
    extern const QColor LevelMeterRed;

    extern const QColor BarLine;
    extern const QColor BarLineIncorrect;
    extern const QColor BeatLine;
    extern const QColor StaffConnectingLine;
    extern const QColor StaffConnectingTerminatingLine;
    extern const QColor StaffRulerBackground;
    
    extern const QColor Pointer;
    extern const QColor PointerRuler;

    extern const QColor InsertCursor;
    extern const QColor InsertCursorRuler;

    extern const QColor MovementGuide;
    extern const QColor SelectionRectangle;
    extern const QColor SelectedElement;
    extern const int SelectedElementHue;
    extern const int SelectedElementMinValue;
    extern const int HighlightedElementHue;
    extern const int HighlightedElementMinValue;
    extern const int QuantizedNoteHue;
    extern const int QuantizedNoteMinValue;

    extern const QColor TextAnnotationBackground;

    extern const QColor AudioCountdownBackground;
    extern const QColor AudioCountdownForeground;

    extern const QColor RotaryFloatBackground;
    extern const QColor RotaryFloatForeground;

    extern const QColor RotaryPastelBlue;
    extern const QColor RotaryPastelRed;
    extern const QColor RotaryPastelGreen;
    extern const QColor RotaryPastelOrange;
    extern const QColor RotaryPastelYellow;
}

#endif

