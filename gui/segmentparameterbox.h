// -*- c-basic-offset: 4 -*-

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

#include <vector>


#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qpushbutton.h>

#include "Quantizer.h"
#include "Selection.h"
#include "colourwidgets.h"
#include "widgets.h"


// Provides a mechanism for viewing and modifying the parameters
// associated with a Rosegarden Segment.
//
//

namespace Rosegarden { class Segment; }
class RosegardenGUIView;
class MultiViewCommandHistory;
class KCommand;

#ifndef _SEGMENTPARAMETERBOX_H_
#define _SEGMENTPARAMETERBOX_H_

class SegmentParameterBox : public RosegardenParameterBox
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

    SegmentParameterBox(RosegardenGUIView *view,
                        QWidget *parent=0);
    ~SegmentParameterBox();

    // Use Segments to update GUI parameters
    //
    void useSegment(Rosegarden::Segment *segment);
    void useSegments(const Rosegarden::SegmentSelection &segments);

    // Command history stuff
    MultiViewCommandHistory* getCommandHistory();
    void addCommandToHistory(KCommand *command);

public slots:
    void slotRepeatPressed();
    void slotQuantizeSelected(int);

    void slotTransposeSelected(int);
    void slotTransposeTextChanged(const QString &);

    void slotDelaySelected(int);
    void slotDelayTimeChanged(Rosegarden::timeT delayValue);
    void slotDelayTextChanged(const QString &);

    void slotEditSegmentLabel();

    void slotColourSelected(int);
    void slotDocColoursChanged();

    virtual void update();

signals:
    void documentModified();
    void canvasModified();

protected:
    void initBox();
    void populateBoxFromSegments();

    QLabel                     *m_label;
    QPushButton                *m_labelButton;
    RosegardenTristateCheckBox *m_repeatValue;
    KComboBox                  *m_quantizeValue;
    KComboBox                  *m_transposeValue;
    KComboBox                  *m_delayValue;
    KComboBox                  *m_colourValue;

    int                        m_addColourPos;

    std::vector<Rosegarden::Segment*> m_segments;
    std::vector<Rosegarden::timeT> m_standardQuantizations;
    std::vector<Rosegarden::timeT> m_delays;
    std::vector<int> m_realTimeDelays;
    RosegardenColourTable::ColourList  m_colourList;

    RosegardenGUIView          *m_view;

    Rosegarden::MidiByte        m_tranposeRange;
};


#endif // _SEGMENTPARAMETERBOX_H_
