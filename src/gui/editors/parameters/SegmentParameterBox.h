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
class SegmentParameterBox : public RosegardenParameterBox,
                            public CompositionObserver
{

    Q_OBJECT

public:
    // ??? Get document directly.
    SegmentParameterBox(RosegardenDocument *doc,
                        QWidget *parent);
    ~SegmentParameterBox() override;

    // ??? Get document directly.
    void setDocument(RosegardenDocument *doc);

    // ??? Get rid of this.  Use getSelectedSegments() (in the .cpp) instead.
    void useSegments(const SegmentSelection &segments);

    // CompositionObserver interface
    // ??? Get rid of this.
    void segmentRemoved(const Composition *,
                        Segment *) override;

public slots:
    // ??? It would be nice if there were an ActionFileClient::createAction()
    //     that takes a function pointer instead of a QString/SLOT().
    void slotRepeatPressed();

signals:
    /// RosegardenMainWindow connects to this.
    void documentModified();

private slots:
    void slotNewDocument(RosegardenDocument *doc);

    void slotUpdate();

    void slotEditSegmentLabel();

    void slotTransposeSelected(int);
    void slotTransposeTextChanged(const QString &);

    void slotQuantizeSelected(int);

    void slotDelaySelected(int);
    void slotDelayTimeChanged(timeT delayValue);
    void slotDelayTextChanged(const QString &);

    void slotColourChanged(int);
    void slotDocColoursChanged();

    void slotChangeLinkTranspose();
    void slotResetLinkTranspose();

private:
    RosegardenDocument *m_doc;

    // ??? Can we access the selection directly instead of keeping a
    //     copy?  Of pointers?  That would make this code significantly
    //     safer.  Yes.  See getSelectedSegments() in the .cpp.  Switch
    //     everyone over to that.
    typedef std::vector<Segment *> SegmentVector;
    SegmentVector m_segments;

    void updateWidgets();

    Label *m_label;
    void updateLabel();
    QPushButton *m_edit;

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
    void setDelay(timeT);
    void updateDelay();

    ColorCombo *m_color;
    void updateColor();

};



}

#endif
