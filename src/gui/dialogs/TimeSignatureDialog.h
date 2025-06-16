
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

#ifndef RG_TIMESIGNATUREDIALOG_H
#define RG_TIMESIGNATUREDIALOG_H

#include "base/TimeT.h"
#include "base/TimeSignature.h"

#include <QDialog>
#include <QString>

class QWidget;
class QRadioButton;
class QLabel;
class QDialogButtonBox;
class QCheckBox;


namespace Rosegarden
{


class TimeWidget2;
class Composition;


class TimeSignatureDialog : public QDialog
{
    Q_OBJECT

public:

    TimeSignatureDialog(QWidget *parent,
                        Composition *composition,
                        timeT insertionTime,
                        const TimeSignature& defaultSig =
                            TimeSignature(4,4),
                        bool timeEditable = false,
                        QString explanatoryText = "");

    TimeSignature getTimeSignature() const;

    timeT getTime() const;
    bool shouldNormalizeRests() const;

public slots:

    void slotNumUp();
    void slotNumDown();
    void slotDenomUp();
    void slotDenomDown();
    void slotUpdateCommonTimeButton();
    void slotHelpRequested();

private slots:

    void slotIsValid(bool valid);

private:

    Composition *m_composition;
    TimeSignature m_timeSignature;
    timeT m_time;

    QLabel *m_numLabel;
    QLabel *m_denomLabel;
    QLabel *m_explanatoryLabel;

    QCheckBox *m_commonTimeButton;
    QCheckBox *m_hideSignatureButton;
    QCheckBox *m_hideBarsButton;
    QCheckBox *m_normalizeRestsButton;

    QRadioButton *m_asGivenButton;
    QRadioButton *m_startOfBarButton;

    TimeWidget2 *m_timeWidget;

    QDialogButtonBox *m_buttonBox;
};



}

#endif
