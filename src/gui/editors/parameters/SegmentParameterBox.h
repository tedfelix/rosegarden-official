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

#ifndef RG_SEGMENTPARAMETERBOX_H
#define RG_SEGMENTPARAMETERBOX_H

#include "gui/widgets/ColourTable.h"
#include "base/Composition.h"
#include "base/MidiProgram.h"  // For MidiByte
#include "RosegardenParameterBox.h"
#include "base/Selection.h"  // For SegmentSelection

class QComboBox;
class QPushButton;
class QString;
class QWidget;

#include <vector>


namespace Rosegarden
{


class ColorCombo;
class Command;
class TristateCheckBox;
class Label;
class Segment;
class RosegardenDocument;


/// Top pane to the left of the segment canvas (CompositionView).
/**
 * In the past, this box had fields for highest and lowest note.  Those can
 * now be set in the notation editor via Segment > Convert notation for....
 * This box also had settings for audio fade (auto, fade in, and fade out).
 * These were removed at some point, possibly near r8100.  I've decided to
 * completely remove all remaining traces of these features.
 */
class SegmentParameterBox : public RosegardenParameterBox
{

    Q_OBJECT

public:
    SegmentParameterBox(QWidget *parent);

public slots:
    /// Segment > Toggle Repeat
    /**
     * It would be nice if there were an ActionFileClient::createAction()
     * that takes a function pointer instead of a QString/SLOT().
     */
    void slotToggleRepeat();

private slots:
    void slotNewDocument(RosegardenDocument *doc);

    void slotDocumentModified(bool);

    /// Connected to RosegardenMainViewWidget::segmentsSelected().
    void slotSelectionChanged(const SegmentSelection &segments);

    void slotEditSegmentLabel();

    void slotRepeatClicked(bool checked);

    void slotTransposeSelected(int);

    void slotQuantizeSelected(int qIndex);

    void slotDelaySelected(int);
    //void slotDelayTextChanged(const QString &);

    void slotColourChanged(int);
    void slotDocColoursChanged();

    void slotChangeLinkTranspose();
    void slotResetLinkTranspose();

private:
    void updateWidgets();

    Label *m_label;
    void updateLabel();

    TristateCheckBox *m_repeat;
    void updateRepeat();

    QComboBox *m_transpose;
    void updateTranspose();

    QComboBox *m_quantize;
    std::vector<timeT> m_standardQuantizations;
    /// Compute the UI (m_quantize) index for a specific quantization time.
    int quantizeIndex(timeT t);
    void updateQuantize();

    QComboBox *m_delay;
    std::vector<timeT> m_delays;
    std::vector<int> m_realTimeDelays;
    /// Set the delay combobox to delayValue.
    /**
     * See setSegmentDelay() for details of positive vs. negative delays.
     */
    void setDelay(long delayValue);
    /// Update the delay combobox from the selected Segments.
    void updateDelay();
    /// Set the delay in the selected Segments.
    /**
     * Positive delay values are assumed to be of note-duration (timeT/ppq)
     * and are set using Segment::setDelay().  Negative delay values are
     * assumed to be positive millisecond delays and are set using
     * Segment::setRealTimeDelay().
     *
     * ??? Less than ideal.  We need a better approach.  Perhaps a std::map
     *     from combobox index to a timeT/RealTime delay record.  That would
     *     then allow negative delays.
     */
    void setSegmentDelay(long delayValue);

    ColorCombo *m_color;
    void updateColor();

};


}

#endif
