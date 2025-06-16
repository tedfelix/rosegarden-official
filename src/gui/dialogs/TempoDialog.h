
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

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

#include "base/Composition.h"  // for tempoT

#include <QDialog>
#include <QTime>

class QCheckBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QRadioButton;
class QWidget;


namespace Rosegarden
{


class TimeWidget2;
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
                bool timeEditable);

    /// Set the position at which we're checking the tempo.
    void setTempoPosition(timeT time);

    // For those who would prefer to avoid the changeTempo() signal overhead.
    timeT getTime() const  { return m_tempoTime; }
    tempoT getTempo() const  { return m_tempo; }
    tempoT getTargetTempo() const  { return m_targetTempo; }

signals:

    /// Results are returned via this signal.
    /**
     * Or use the getters above.
     */
    void changeTempo(timeT time,
                     tempoT tempo,
                     tempoT target,
                     TempoDialog::TempoDialogAction action);

private slots:

    void accept() override;

    void slotActionChanged();
    void slotTempoChanged(double);
    void slotTempoConstantClicked();
    void slotTempoRampToNextClicked();
    void slotTempoRampToTargetClicked();
    //void slotTargetChanged(double);
    void slotTapClicked();
    void slotHelpRequested();

    void slotIsValid(bool valid);

private:

    RosegardenDocument *m_doc;

    timeT m_tempoTime{0};

    // Widgets

    // Tempo Group

    QDoubleSpinBox *m_tempoValueSpinBox;
    QPushButton *m_tempoTap;
    QTime m_tapMinusTwo;
    QTime m_tapMinusOne;
    QLabel *m_tempoBeatLabel;
    QLabel *m_tempoBeat;
    QLabel *m_tempoBeatsPerMinute;
    void updateBeatLabels(double newTempo);

    QRadioButton *m_tempoConstant;
    QRadioButton *m_tempoRampToNext;
    QRadioButton *m_tempoRampToTarget;
    QDoubleSpinBox *m_tempoTargetSpinBox;

    // Time of Tempo Change Group

    TimeWidget2 *m_timeWidget{nullptr};

    // Scope Group

    QLabel *m_tempoTimeLabel;  // x.y s
    QLabel *m_tempoBarLabel;  // "at the start of ..." or "in the middle of ..."
    QLabel *m_tempoStatusLabel;  // "there are no..." status message
    
    // Apply this tempo from here onwards
    QRadioButton *m_tempoChangeHere;
    // Replace the last tempo change
    QRadioButton *m_tempoChangeBefore;
    QLabel *m_tempoChangeBeforeAt;
    // Apply this tempo from the start of this bar
    QRadioButton *m_tempoChangeStartOfBar;
    // Apply this tempo to the whole composition
    QRadioButton *m_tempoChangeGlobal;
    // Also make this the default tempo
    QCheckBox *m_defaultBox;

    QDialogButtonBox *m_buttonBox;

    void populateTempo();


    // User input is stored here on accept().
    tempoT m_tempo{0};
    tempoT m_targetTempo{-1};

};


}

#endif
