// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2003
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef _eventfilter_H_
#define _eventfilter_H_

#include <qvariant.h>
#include <kdialog.h>
#include "Quantizer.h" // to get named durations

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;

class EventFilterDialog : public KDialog
{
    Q_OBJECT

public:

    EventFilterDialog(QWidget* parent = 0);
    ~EventFilterDialog();

    // initialize the whole kit and kaboodle
    void initDialog();

protected:
    
    //---------[ data members ]-----------------------------
    QCheckBox* m_noteCheckBox;
    QComboBox* m_notePitchIncludeComboBox;
    QLabel* m_pitchLabel;
    QComboBox* m_noteVelocityIncludeComboBox;
    QComboBox* m_noteDurationIncludeComboBox;
    QLabel* m_velocityLabel;
    QLabel* m_pitchToLabel;
    QSpinBox* m_pitchToSpinBox;
    QLabel* m_pitchFromLabel;
    QSpinBox* m_pitchFromSpinBox;
    QSpinBox* m_velocityFromSpinBox;
    QLabel* m_pitchFromValueLabel;
    QSpinBox* m_velocityToSpinBox;
    QLabel* m_durationLabel;
    QComboBox* m_noteDurationFromComboBox;
    QCheckBox* m_controllerCheckBox;
    QLabel* m_controllerFromLabel;
    QLabel* m_controllerToLabel;
    QLabel* m_pitchToValueLabel;
    QComboBox* m_noteDurationToComboBox;
    QComboBox* m_controllerNumberIncludeComboBox;
    QLabel* m_numberLabel;
    QSpinBox* m_controllerNumberFromSpinBox;
    QSpinBox* m_controllerNumberToSpinBox;
    QLabel* m_valueLabel;
    QComboBox* m_controllerValueIncludeComboBox;
    QSpinBox* m_controllerValueFromSpinBox;
    QSpinBox* m_controllerValueToSpinBox;
    QCheckBox* m_wheelCheckBox;
    QComboBox* m_wheelAmountIncludeComboBox;
    QLabel* m_wheelFromLabel;
    QSpinBox* m_wheelAmountFromSpinBox;
    QLabel* m_wheelAmountLabel;
    QSpinBox* m_wheelAmountToSpinBox;
    QLabel* m_wheelToLabel;
    QPushButton* m_buttonCancel;
    QPushButton* m_buttonOk;
    QPushButton* m_buttonNone;
    QPushButton* m_buttonAll;
    QGridLayout* layout;
    std::vector<Rosegarden::timeT> m_standardQuantizations;

protected slots:
    void slotToggleAll();
    void slotToggleNone();
    void slotNoteCheckBoxToggle(int);
    void slotControllerCheckBoxToggle(int);
    void slotWheelCheckBoxToggle(int);
    void slotPitchFromChanged(int pitch);
    void slotPitchToChanged(int pitch);
};

#endif // _eventfilter_H_
