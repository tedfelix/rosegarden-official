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


#include <kapp.h>
#include <kconfig.h>

#include "colours.h"

namespace Rosegarden {

QColor GUIPalette::getColour(const char* const colourName)
{
    KConfig* config = kapp->config();
    config->setGroup(ColoursConfigGroup);

    QColor res = getInstance()->m_defaultsMap[colourName];
    config->readColorEntry(colourName, &res);
    return res;
}

Rosegarden::Colour GUIPalette::convertColour(const QColor &input)
{
    int r,g,b;
    input.rgb(&r, &g, &b);
    return Rosegarden::Colour(r,g,b);
}

QColor GUIPalette::convertColour(const Rosegarden::Colour& input)
{
    return QColor(input.getRed(), input.getGreen(), input.getBlue());
}


GUIPalette::GUIPalette()
{
    m_defaultsMap[ActiveRecordTrack] = Qt::red;

    m_defaultsMap[SegmentCanvas] = QColor(230, 230, 230);
    m_defaultsMap[SegmentBorder] = Qt::black;

    m_defaultsMap[RecordingInternalSegmentBlock] = QColor(255, 182, 193);
    m_defaultsMap[RecordingAudioSegmentBlock] = QColor(182, 222, 255);
    m_defaultsMap[RecordingSegmentBorder] = Qt::black;

    m_defaultsMap[RepeatSegmentBorder] = QColor(130, 133, 170);

    m_defaultsMap[SegmentAudioPreview] = QColor(39, 71, 22);
    m_defaultsMap[SegmentInternalPreview] = Qt::white;
    m_defaultsMap[SegmentLabel] = Qt::black;
    m_defaultsMap[SegmentSplitLine] = Qt::black;

    m_defaultsMap[MatrixElementBorder] = Qt::black;
    m_defaultsMap[MatrixElementBlock] = QColor(98, 128, 232);

    m_defaultsMap[LoopRulerBackground] = QColor(120, 120, 120);
    m_defaultsMap[LoopRulerForeground] = Qt::white;
    m_defaultsMap[LoopHighlight] = Qt::white;

    m_defaultsMap[TempoBase] = QColor(197, 211, 125);

    //m_defaultsMap[TextRulerBackground] = QColor(60, 205, 230, QColor::Hsv);
//    m_defaultsMap[TextRulerBackground] = QColor(120, 90, 238, QColor::Hsv);
//    m_defaultsMap[TextRulerBackground] = QColor(210, 220, 140);
    m_defaultsMap[TextRulerBackground] = QColor(226, 232, 187);
    m_defaultsMap[TextRulerForeground] = Qt::white;

    m_defaultsMap[ChordNameRulerBackground] = QColor(230, 230, 230);
    m_defaultsMap[ChordNameRulerForeground] = Qt::black;

    m_defaultsMap[RawNoteRulerBackground] = QColor(240, 240, 240);
    m_defaultsMap[RawNoteRulerForeground] = Qt::black;

    m_defaultsMap[LevelMeterGreen] = QColor(0, 200, 0);
    m_defaultsMap[LevelMeterOrange] = QColor(255, 165, 0);
    m_defaultsMap[LevelMeterRed] = QColor(200, 0, 0);

//    m_defaultsMap[LevelMeterSolidGreen] = QColor(0, 140, 0);
    m_defaultsMap[LevelMeterSolidGreen] = QColor(84, 177, 248); // blue!
    m_defaultsMap[LevelMeterSolidOrange] = QColor(220, 120, 0);
    m_defaultsMap[LevelMeterSolidRed] = QColor(255, 50, 50);

    m_defaultsMap[BarLine] = Qt::black;
    m_defaultsMap[BarLineIncorrect] = QColor(211, 0, 31);
    m_defaultsMap[BeatLine] = QColor(100, 100, 100);
    m_defaultsMap[SubBeatLine] = QColor(212, 212, 212);
    m_defaultsMap[StaffConnectingLine] = QColor(192, 192, 192);
    m_defaultsMap[StaffConnectingTerminatingLine] = QColor(128, 128, 128);

    m_defaultsMap[Pointer] = Qt::darkBlue;
    m_defaultsMap[PointerRuler] = QColor(100, 100, 100);

    m_defaultsMap[InsertCursor] = QColor(160, 104, 186);
    m_defaultsMap[InsertCursorRuler] = QColor(160, 136, 170);

    m_defaultsMap[MovementGuide] = QColor(172, 230, 139);
    //m_defaultsMap[MovementGuide] = QColor(62, 161, 194);
    //m_defaultsMap[MovementGuide] = QColor(255, 189, 89);
    m_defaultsMap[SelectionRectangle] = QColor(103, 128, 211);
    m_defaultsMap[SelectedElement] = QColor(0, 54, 232);

    const int SelectedElementHue = 225;
    const int SelectedElementMinValue = 220;
    const int HighlightedElementHue = 25;
    const int HighlightedElementMinValue = 220;
    const int QuantizedNoteHue = 69;
    const int QuantizedNoteMinValue = 140;
    const int TriggerNoteHue = 4;
    const int TriggerNoteMinValue = 140;

    m_defaultsMap[TextAnnotationBackground] = QColor(255, 255, 180);

    m_defaultsMap[AudioCountdownBackground] = Qt::darkGray;
    m_defaultsMap[AudioCountdownForeground] = Qt::red;

    m_defaultsMap[RotaryFloatBackground] = Qt::cyan;
    m_defaultsMap[RotaryFloatForeground] = Qt::black;

    m_defaultsMap[RotaryPastelBlue] = QColor(205, 212, 255);
    m_defaultsMap[RotaryPastelRed] = QColor(255, 168, 169);
    m_defaultsMap[RotaryPastelGreen] = QColor(231, 255, 223);
    m_defaultsMap[RotaryPastelOrange] = QColor(255, 233, 208);
    m_defaultsMap[RotaryPastelYellow] = QColor(249, 255, 208);

    m_defaultsMap[MatrixKeyboardFocus] = QColor(224, 112, 8);

//    m_defaultsMap[RotaryPlugin] = QColor(185, 255, 248);
    m_defaultsMap[RotaryPlugin] = QColor(185, 200, 248);
//    m_defaultsMap[RotaryPlugin] = QColor(185, 185, 185);

    m_defaultsMap[RotaryMeter] = QColor(255, 100, 0);

    m_defaultsMap[MarkerBackground] = QColor(185, 255, 248);

//    m_defaultsMap[MuteTrackLED] = QColor(218, 190, 230, QColor::Hsv);
    m_defaultsMap[MuteTrackLED] = QColor(211, 194, 238, QColor::Hsv);
    m_defaultsMap[RecordTrackLED] = QColor(0, 250, 225, QColor::Hsv);

    m_defaultsMap[PlaybackFaderOutline] = QColor(211, 194, 238, QColor::Hsv);
    m_defaultsMap[RecordFaderOutline] = QColor(0, 250, 225, QColor::Hsv);


}

GUIPalette* GUIPalette::getInstance() 
{
    if (!m_instance) m_instance = new GUIPalette();
    return m_instance;
}

const char* const GUIPalette::ColoursConfigGroup = "coloursconfiggroup";

const char* const GUIPalette::ActiveRecordTrack = "activerecordtrack";


const char* const GUIPalette::SegmentCanvas = "segmentcanvas";
const char* const GUIPalette::SegmentBorder = "segmentborder";
const char* const GUIPalette::RecordingInternalSegmentBlock = "recordinginternalsegmentblock";
const char* const GUIPalette::RecordingAudioSegmentBlock = "recordingaudiosegmentblock";
const char* const GUIPalette::RecordingSegmentBorder = "recordingsegmentborder";

const char* const GUIPalette::RepeatSegmentBorder = "repeatsegmentborder";

const char* const GUIPalette::SegmentAudioPreview = "segmentaudiopreview";
const char* const GUIPalette::SegmentInternalPreview = "segmentinternalpreview";
const char* const GUIPalette::SegmentLabel = "segmentlabel";
const char* const GUIPalette::SegmentSplitLine = "segmentsplitline";

const char* const GUIPalette::MatrixElementBorder = "matrixelementborder";
const char* const GUIPalette::MatrixElementBlock = "matrixelementblock";

const char* const GUIPalette::LoopRulerBackground = "looprulerbackground";
const char* const GUIPalette::LoopRulerForeground = "looprulerforeground";
const char* const GUIPalette::LoopHighlight = "loophighlight";

const char* const GUIPalette::TempoBase = "tempobase";

const char* const GUIPalette::TextRulerBackground = "textrulerbackground";
const char* const GUIPalette::TextRulerForeground = "textrulerforeground";

const char* const GUIPalette::ChordNameRulerBackground = "chordnamerulerbackground";
const char* const GUIPalette::ChordNameRulerForeground = "chordnamerulerforeground";

const char* const GUIPalette::RawNoteRulerBackground = "rawnoterulerbackground";
const char* const GUIPalette::RawNoteRulerForeground = "rawnoterulerforeground";

const char* const GUIPalette::LevelMeterGreen = "levelmetergreen";
const char* const GUIPalette::LevelMeterOrange = "levelmeterorange";
const char* const GUIPalette::LevelMeterRed = "levelmeterred";

const char* const GUIPalette::LevelMeterSolidGreen = "levelmetersolidgreen";
const char* const GUIPalette::LevelMeterSolidOrange = "levelmetersolidorange";
const char* const GUIPalette::LevelMeterSolidRed = "levelmetersolidred";

const char* const GUIPalette::BarLine = "barline";
const char* const GUIPalette::BarLineIncorrect = "barlineincorrect";
const char* const GUIPalette::BeatLine = "beatline";
const char* const GUIPalette::SubBeatLine = "subbeatline";
const char* const GUIPalette::StaffConnectingLine = "staffconnectingline";
const char* const GUIPalette::StaffConnectingTerminatingLine = "staffconnectingterminatingline";

const char* const GUIPalette::Pointer = "pointer";
const char* const GUIPalette::PointerRuler = "pointerruler";

const char* const GUIPalette::InsertCursor = "insertcursor";
const char* const GUIPalette::InsertCursorRuler = "insertcursorruler";

const char* const GUIPalette::MovementGuide = "movementguide";
const char* const GUIPalette::SelectionRectangle = "selectionrectangle";
const char* const GUIPalette::SelectedElement = "selectedelement";

const int GUIPalette::SelectedElementHue = 225;
const int GUIPalette::SelectedElementMinValue = 220;
const int GUIPalette::HighlightedElementHue = 25;
const int GUIPalette::HighlightedElementMinValue = 220;
const int GUIPalette::QuantizedNoteHue = 69;
const int GUIPalette::QuantizedNoteMinValue = 140;
const int GUIPalette::TriggerNoteHue = 4;
const int GUIPalette::TriggerNoteMinValue = 140;

const char* const GUIPalette::TextAnnotationBackground = "textannotationbackground";

const char* const GUIPalette::AudioCountdownBackground = "audiocountdownbackground";
const char* const GUIPalette::AudioCountdownForeground = "audiocountdownforeground";

const char* const GUIPalette::RotaryFloatBackground = "rotaryfloatbackground";
const char* const GUIPalette::RotaryFloatForeground = "rotaryfloatforeground";

const char* const GUIPalette::RotaryPastelBlue = "rotarypastelblue";
const char* const GUIPalette::RotaryPastelRed = "rotarypastelred";
const char* const GUIPalette::RotaryPastelGreen = "rotarypastelgreen";
const char* const GUIPalette::RotaryPastelOrange = "rotarypastelorange";
const char* const GUIPalette::RotaryPastelYellow = "rotarypastelyellow";

const char* const GUIPalette::MatrixKeyboardFocus = "matrixkeyboardfocus";

const char* const GUIPalette::RotaryPlugin = "rotaryplugin";

const char* const GUIPalette::RotaryMeter = "rotarymeter";

const char* const GUIPalette::MarkerBackground = "markerbackground";

const char* const GUIPalette::MuteTrackLED = "mutetrackled";
const char* const GUIPalette::RecordTrackLED = "recordtrackled";

const char* const GUIPalette::PlaybackFaderOutline = "playbackfaderoutline";
const char* const GUIPalette::RecordFaderOutline = "recordfaderoutline";

 
GUIPalette* GUIPalette::m_instance = 0;

}
