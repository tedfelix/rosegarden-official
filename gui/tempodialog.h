// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _TEMPO_DIALOG_H_
#define _TEMPO_DIALOG_H_

#include <kdialogbase.h>
#include "Composition.h" // tempoT

class RosegardenGUIDoc;
class HSpinBox;
class QRadioButton;
class QLabel;
class RosegardenTimeWidget;
class QCheckBox;
class QWidget;

class TempoDialog : public KDialogBase
{
    Q_OBJECT
public:
    typedef enum{
        AddTempo,
        ReplaceTempo,
        AddTempoAtBarStart,
        GlobalTempo,
        GlobalTempoWithDefault
    } TempoDialogAction;

    TempoDialog(QWidget *parent, RosegardenGUIDoc *doc,
		bool timeEditable = false);
    ~TempoDialog();

    // Set the position at which we're checking the tempo
    //
    void setTempoPosition(Rosegarden::timeT time);

public slots:
    virtual void slotOk();
    void slotActionChanged();
    void slotTempoChanged(const QString &);
    void slotTempoConstantClicked();
    void slotTempoRampToNextClicked();
    void slotTempoRampToTargetClicked();
    void slotTargetChanged(const QString &);

signals:
    // Return results in this signal
    //
    void changeTempo(Rosegarden::timeT,  // tempo change time
                     Rosegarden::tempoT,  // tempo value
		     Rosegarden::tempoT,  // target tempo value
                     TempoDialog::TempoDialogAction); // tempo action

protected:
    void populateTempo();
    void updateBeatLabels(double newTempo);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc     *m_doc;
    Rosegarden::timeT     m_tempoTime;
    HSpinBox  	  	 *m_tempoValueSpinBox;

    QRadioButton         *m_tempoConstant;
    QRadioButton         *m_tempoRampToNext;
    QRadioButton         *m_tempoRampToTarget;
    HSpinBox             *m_tempoTargetSpinBox; 

    QLabel	         *m_tempoBeatLabel;
    QLabel	         *m_tempoBeat;
    QLabel	         *m_tempoBeatsPerMinute;

    RosegardenTimeWidget *m_timeEditor;

    QLabel               *m_tempoTimeLabel;
    QLabel               *m_tempoBarLabel;
    QLabel               *m_tempoStatusLabel;
    
    QRadioButton         *m_tempoChangeHere;
    QRadioButton         *m_tempoChangeBefore;
    QLabel	         *m_tempoChangeBeforeAt;
    QRadioButton         *m_tempoChangeStartOfBar;
    QRadioButton         *m_tempoChangeGlobal;
    QCheckBox            *m_defaultBox;
};

#endif
