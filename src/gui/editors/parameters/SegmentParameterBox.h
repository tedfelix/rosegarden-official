
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

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

#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "base/Selection.h"
#include "gui/widgets/ColourTable.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"
#include <QString>
#include <vector>
#include "base/Event.h"


class QWidget;
class QSpinBox;
class QPushButton;
class QLabel;
class QCheckBox;
class QComboBox;


namespace Rosegarden
{

class Command;
class TristateCheckBox;
class Label;
class Segment;
class RosegardenDocument;
class Composition;


class SegmentParameterBox : public RosegardenParameterBox,
                            public CompositionObserver
{
Q_OBJECT

public:

    typedef enum
    {
        None,
        Some,
        All,
        NotApplicable // no applicable segments selected
    } Tristate;

    SegmentParameterBox(RosegardenDocument *doc,
                        QWidget *parent=0);
    ~SegmentParameterBox();

    // Use Segments to update GUI parameters
    //
    void useSegment(Segment *segment);
    void useSegments(const SegmentSelection &segments);

    void addCommandToHistory(Command *command);

    void setDocument(RosegardenDocument*);

    // CompositionObserver interface
    //
    virtual void segmentRemoved(const Composition *,
                                Segment *);

public slots:
    void slotRepeatPressed();
    void slotQuantizeSelected(int);

    void slotTransposeSelected(int);
    void slotTransposeTextChanged(const QString &);

    void slotDelaySelected(int);
    void slotDelayTimeChanged(timeT delayValue);
    void slotDelayTextChanged(const QString &);

    void slotEditSegmentLabel();

    void slotColourSelected(int);
    void slotDocColoursChanged();

    void slotAudioFadeChanged(int);
    void slotFadeInChanged(int);
    void slotFadeOutChanged(int);

    void slotHighestPressed();
    void slotLowestPressed();

    void slotChangeLinkTranspose();
    void slotResetLinkTranspose();

    virtual void update();

signals:
    void documentModified();
    void canvasModified();

protected:
    void initBox();
    void populateBoxFromSegments();
    void updateHighLow();

    Label                      *m_label;
//    QLabel                     *m_rangeLabel;
    QPushButton                *m_editButton;
//    QPushButton                *m_highButton;
//    QPushButton                *m_lowButton;
    TristateCheckBox *m_repeatCheckBox;
    QComboBox                  *m_quantizeComboBox;
    QComboBox                  *m_transposeComboBox;
    QComboBox                  *m_delayComboBox;
    QComboBox                  *m_colourComboBox;

    // Audio autofade
    //
//    QLabel                     *m_autoFadeLabel;
//    QCheckBox                  *m_autoFadeBox;
//    QLabel                     *m_fadeInLabel;
//    QSpinBox                   *m_fadeInSpin;
//    QLabel                     *m_fadeOutLabel;
//    QSpinBox                   *m_fadeOutSpin;

    int                        m_addColourPos;

    // used to keep track of highest/lowest as there is no associated spinbox
    // to query for its value
    int                        m_highestPlayable;
    int                        m_lowestPlayable;

    std::vector<Segment*> m_segments;
    std::vector<timeT> m_standardQuantizations;
    std::vector<timeT> m_delays;
    std::vector<int> m_realTimeDelays;
    ColourTable::ColourList  m_colourList;

    RosegardenDocument           *m_doc;

    MidiByte        m_transposeRange;
};



}

#endif
