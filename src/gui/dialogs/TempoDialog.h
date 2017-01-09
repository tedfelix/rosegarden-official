
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

#ifndef RG_TEMPODIALOG_H
#define RG_TEMPODIALOG_H

#include <QDialog>
#include <QTime>
#include "base/Composition.h" // for tempoT (TODO: should move to a small header like basetypes.h)
#include "base/Event.h" // for timeT (TODO: same)

class QWidget;
class QString;
class QRadioButton;
class QLabel;
class QCheckBox;
class QDoubleSpinBox;
class QPushButton;

namespace Rosegarden
{

class TimeWidget;
class RosegardenDocument;


class TempoDialog : public QDialog
{
    Q_OBJECT
public:
    enum TempoDialogAction {
        AddTempo,
        ReplaceTempo,
        AddTempoAtBarStart,
        GlobalTempo,
        GlobalTempoWithDefault
    };

    TempoDialog(QWidget *parent, RosegardenDocument *doc,
                bool timeEditable = false);
    ~TempoDialog();

    // Set the position at which we're checking the tempo
    //
    void setTempoPosition(timeT time);

public slots:
    virtual void accept();
    void slotActionChanged();
    void slotTempoChanged(double);
    void slotTempoConstantClicked();
    void slotTempoRampToNextClicked();
    void slotTempoRampToTargetClicked();
    void slotTargetChanged(double);
    void slotTapClicked();
    void slotHelpRequested();

signals:
    // Return results in this signal
    //
    void changeTempo(timeT,  // tempo change time
                     tempoT,  // tempo value
                     tempoT,  // target tempo value
                     TempoDialog::TempoDialogAction); // tempo action

protected:
    void populateTempo();
    void updateBeatLabels(double newTempo);

    //--------------- Data members ---------------------------------

    RosegardenDocument     *m_doc;
    timeT                 m_tempoTime;
    QDoubleSpinBox       *m_tempoValueSpinBox;
    QPushButton          *m_tempoTap;
    QTime                 m_tapMinusTwo;
    QTime                 m_tapMinusOne;

    QRadioButton         *m_tempoConstant;
    QRadioButton         *m_tempoRampToNext;
    QRadioButton         *m_tempoRampToTarget;
    QDoubleSpinBox       *m_tempoTargetSpinBox; 

    QLabel               *m_tempoBeatLabel;
    QLabel               *m_tempoBeat;
    QLabel               *m_tempoBeatsPerMinute;

    TimeWidget           *m_timeEditor;

    QLabel               *m_tempoTimeLabel;
    QLabel               *m_tempoBarLabel;
    QLabel               *m_tempoStatusLabel;
    
    QRadioButton         *m_tempoChangeHere;
    QRadioButton         *m_tempoChangeBefore;
    QLabel               *m_tempoChangeBeforeAt;
    QRadioButton         *m_tempoChangeStartOfBar;
    QRadioButton         *m_tempoChangeGlobal;
    QCheckBox            *m_defaultBox;
};


}

#endif
