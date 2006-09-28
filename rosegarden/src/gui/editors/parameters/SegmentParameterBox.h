
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SEGMENTPARAMETERBOX_H_
#define _RG_SEGMENTPARAMETERBOX_H_

#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "gui/widgets/ColourTable.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"
#include <qstring.h>
#include <vector>
#include "base/Event.h"


class QWidget;
class QSpinBox;
class QPushButton;
class QLabel;
class QCheckBox;
class KCommand;
class KComboBox;


namespace Rosegarden
{

class TristateCheckBox;
class SegmentSelection;
class Segment;
class RosegardenGUIDoc;
class MultiViewCommandHistory;
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

    SegmentParameterBox(RosegardenGUIDoc *doc,
                        QWidget *parent=0);
    ~SegmentParameterBox();

    // Use Segments to update GUI parameters
    //
    void useSegment(Segment *segment);
    void useSegments(const SegmentSelection &segments);

    // Command history stuff
    MultiViewCommandHistory* getCommandHistory();
    void addCommandToHistory(KCommand *command);

    void setDocument(RosegardenGUIDoc*);

    // CompositionObserver interface
    //
    virtual void segmentRemoved(const Composition *,
				Segment *);

    virtual void showAdditionalControls(bool showThem);

    virtual QString getPreviousBox(RosegardenParameterArea::Arrangement) const;

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

    virtual void update();

signals:
    void documentModified();
    void canvasModified();

protected:
    void initBox();
    void populateBoxFromSegments();
    void updateHighLow();

    QLabel                     *m_label;
    QLabel                     *m_rangeLabel;
    QPushButton                *m_labelButton;
    QPushButton		       *m_highButton;
    QPushButton		       *m_lowButton;
    TristateCheckBox *m_repeatValue;
    KComboBox                  *m_quantizeValue;
    KComboBox                  *m_transposeValue;
    KComboBox                  *m_delayValue;
    KComboBox                  *m_colourValue;

    // Audio autofade
    //
    QLabel                     *m_autoFadeLabel;
    QCheckBox                  *m_autoFadeBox;
    QLabel                     *m_fadeInLabel;
    QSpinBox                   *m_fadeInSpin;
    QLabel                     *m_fadeOutLabel;
    QSpinBox                   *m_fadeOutSpin;

    int                        m_addColourPos;

    // used to keep track of highest/lowest as there is no associated spinbox
    // to query for its value
    int			       m_highestPlayable;
    int			       m_lowestPlayable;

    std::vector<Segment*> m_segments;
    std::vector<timeT> m_standardQuantizations;
    std::vector<timeT> m_delays;
    std::vector<int> m_realTimeDelays;
    ColourTable::ColourList  m_colourList;

    RosegardenGUIDoc           *m_doc;

    MidiByte        m_transposeRange;
};



}

#endif
