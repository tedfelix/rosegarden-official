// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file Copyright 2003
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "eventfilter.h"

#include <klocale.h>
#include <qvariant.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "midipitchlabel.h"	// number to text pitch display
#include "rosedebug.h"          // debug stream
#include "notepixmapfactory.h"  // pixmaps for duration combos
#include "Quantizer.h"          // text for duration combos
#include "NotationTypes.h"      // likewise
#include "notationstrings.h"    // and so on


// Constructs a dialog to select various criteria used to filter events out of
// selections
EventFilterDialog::EventFilterDialog(QWidget* parent)
    : KDialog(parent),
      // lifted from segmentparameterbox.cpp on 5/31/03
      m_standardQuantizations(Rosegarden::BasicQuantizer::getStandardQuantizations())
{
    initDialog();
}

void EventFilterDialog::initDialog()
{
    setCaption(i18n("Event Filter"));
    setBackgroundOrigin(KDialog::WidgetOrigin);
    setFocusPolicy(KDialog::StrongFocus);
    layout = new QGridLayout(this, 1, 1, 13, 5);
    
    m_noteCheckBox = new QCheckBox(i18n("Note"),this);
    m_noteCheckBox->setChecked(true);
    layout->addWidget(m_noteCheckBox, 0, 0);

    m_pitchToLabel = new QLabel(i18n("to:"),this);
    layout->addWidget(m_pitchToLabel, 0, 4);

    m_pitchFromLabel = new QLabel(i18n("from:"),this);
    layout->addWidget(m_pitchFromLabel, 0, 2);

    m_notePitchIncludeComboBox = new QComboBox(0, this);
    m_notePitchIncludeComboBox->insertItem(i18n("include"));
    m_notePitchIncludeComboBox->insertItem(i18n("exclude"));
    layout->addWidget(m_notePitchIncludeComboBox, 1, 0);

    m_pitchLabel = new QLabel(i18n("Pitch:"),this);
    layout->addWidget(m_pitchLabel, 1, 1);

    m_noteVelocityIncludeComboBox = new QComboBox(0, this);
    m_noteVelocityIncludeComboBox->insertItem(i18n("include"));
    m_noteVelocityIncludeComboBox->insertItem(i18n("exclude"));
    layout->addWidget(m_noteVelocityIncludeComboBox, 2, 0);

    m_noteDurationIncludeComboBox = new QComboBox(0, this);
    m_noteDurationIncludeComboBox->insertItem(i18n("include"));
    m_noteDurationIncludeComboBox->insertItem(i18n("exclude"));
    layout->addWidget(m_noteDurationIncludeComboBox, 3, 0);

    m_velocityLabel = new QLabel(i18n("Velocity:"),this);
    layout->addWidget(m_velocityLabel, 2, 1);
    
    // from dialogs.cpp - to display the humanized pitches
    //!!! has this been internationalized, or does it always return English?
    Rosegarden::MidiPitchLabel plFrom(0);
    m_pitchFromValueLabel = new QLabel(plFrom.getQString(),this);
    layout->addWidget(m_pitchFromValueLabel, 1, 3);

    m_pitchFromSpinBox = new QSpinBox(this);
    m_pitchFromSpinBox->setMaxValue(127);
    layout->addWidget(m_pitchFromSpinBox, 1, 2);
    connect(m_pitchFromSpinBox, SIGNAL(valueChanged(int)),
	    SLOT(slotPitchFromChanged(int)));

    Rosegarden::MidiPitchLabel plTo(127);
    m_pitchToValueLabel = new QLabel(plTo.getQString(),this);
    layout->addWidget(m_pitchToValueLabel, 1, 5);

    m_pitchToSpinBox = new QSpinBox(this);
    m_pitchToSpinBox->setMaxValue(127);
    m_pitchToSpinBox->setValue(127);
    layout->addWidget(m_pitchToSpinBox, 1, 4);
    connect(m_pitchToSpinBox, SIGNAL(valueChanged(int)),
	    SLOT(slotPitchToChanged(int)));

    m_velocityFromSpinBox = new QSpinBox(this);
    m_velocityFromSpinBox->setMaxValue(127);
    layout->addWidget(m_velocityFromSpinBox, 2, 2);

    m_velocityToSpinBox = new QSpinBox(this);
    m_velocityToSpinBox->setMaxValue(127);
    m_velocityToSpinBox->setValue(127);
    layout->addWidget(m_velocityToSpinBox, 2, 4);

    m_durationLabel = new QLabel(i18n("Duration:"), this);
    layout->addWidget(m_durationLabel, 3, 1);

    m_controllerCheckBox = new QCheckBox(i18n("Controller"),this);
    m_controllerCheckBox->setChecked(true);
    layout->addWidget(m_controllerCheckBox, 4, 0);

    m_controllerFromLabel = new QLabel(i18n("from:"),this);
    layout->addWidget(m_controllerFromLabel, 4, 2);

    m_controllerToLabel = new QLabel(i18n("to:"),this);
    layout->addWidget(m_controllerToLabel, 4, 4);

    m_noteDurationFromComboBox = new QComboBox(0, this);
    m_noteDurationFromComboBox->insertItem(i18n("unlimited"));
    m_noteDurationFromComboBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
		(QSizePolicy::SizeType)0, 0, 0,
		m_noteDurationFromComboBox->sizePolicy().hasHeightForWidth()));
    layout->addMultiCellWidget(m_noteDurationFromComboBox, 3, 3, 2, 3);

    m_noteDurationToComboBox = new QComboBox(0, this);
    m_noteDurationToComboBox->insertItem(i18n("unlimited"));
    m_noteDurationToComboBox->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
		(QSizePolicy::SizeType)0, 0, 0,
		m_noteDurationToComboBox->sizePolicy().hasHeightForWidth())); 
    layout->addMultiCellWidget(m_noteDurationToComboBox, 3, 3, 4, 5);

    // lifted from segmentparameterbox.cpp on 5/31/03
    //
    // populate the quantize combos with reasonable durations and associated
    // pixmaps
    //
    NotePixmapFactory npf;
    QPixmap noMap = NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap("menu-no-note"));

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i)
    {

        Rosegarden::timeT time = m_standardQuantizations[i];
        Rosegarden::timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(time, error));
	m_noteDurationFromComboBox->insertItem(error ? noMap : pmap, label);
        m_noteDurationToComboBox->insertItem(error ? noMap : pmap, label);
    }
    m_noteDurationFromComboBox->insertItem(noMap, i18n("0"));
    m_noteDurationToComboBox->insertItem(noMap, i18n("0"));

    m_noteDurationFromComboBox->setCurrentItem(m_noteDurationToComboBox->count() - 1);
    m_noteDurationToComboBox->setCurrentItem(0);


    m_controllerNumberIncludeComboBox = new QComboBox(0, this);
    m_controllerNumberIncludeComboBox->insertItem(i18n("include"));
    m_controllerNumberIncludeComboBox->insertItem(i18n("exclude"));
    layout->addWidget(m_controllerNumberIncludeComboBox, 5, 0);

    m_numberLabel = new QLabel(i18n("Number:"),this);
    layout->addWidget(m_numberLabel, 5, 1);

    m_controllerNumberFromSpinBox = new QSpinBox(this);
    m_controllerNumberFromSpinBox->setMaxValue(127);
    layout->addWidget(m_controllerNumberFromSpinBox, 5, 2);

    m_controllerNumberToSpinBox = new QSpinBox(this);
    m_controllerNumberToSpinBox->setMaxValue(127);
    m_controllerNumberToSpinBox->setValue(127);
    layout->addWidget(m_controllerNumberToSpinBox, 5, 4);

    m_valueLabel = new QLabel(i18n("Value:"),this);
    layout->addWidget(m_valueLabel, 6, 1);

    m_controllerValueIncludeComboBox = new QComboBox(0, this);
    m_controllerValueIncludeComboBox->insertItem(i18n("include"));
    m_controllerValueIncludeComboBox->insertItem(i18n("exclude"));
    layout->addWidget(m_controllerValueIncludeComboBox, 6, 0);

    m_controllerValueFromSpinBox = new QSpinBox(this);
    m_controllerValueFromSpinBox->setMaxValue(127);
    layout->addWidget(m_controllerValueFromSpinBox, 6, 2);

    m_controllerValueToSpinBox = new QSpinBox(this);
    m_controllerValueToSpinBox->setMaxValue(127);
    m_controllerValueToSpinBox->setValue(127);
    layout->addWidget(m_controllerValueToSpinBox, 6, 4);

    m_wheelCheckBox = new QCheckBox(i18n("Wheel"),this);
    m_wheelCheckBox->setChecked(true);
    layout->addWidget(m_wheelCheckBox, 7, 0);

    m_wheelAmountIncludeComboBox = new QComboBox(0, this);
    m_wheelAmountIncludeComboBox->insertItem(i18n("include"));
    m_wheelAmountIncludeComboBox->insertItem(i18n("exclude"));
    layout->addWidget(m_wheelAmountIncludeComboBox, 8, 0);

    m_wheelFromLabel = new QLabel(i18n("from:"),this);
    layout->addWidget(m_wheelFromLabel, 7, 2);

    m_wheelAmountFromSpinBox = new QSpinBox(this);
    m_wheelAmountFromSpinBox->setMaxValue(8191);
    m_wheelAmountFromSpinBox->setMinValue(-8192);
    m_wheelAmountFromSpinBox->setValue(-8192);
    layout->addWidget(m_wheelAmountFromSpinBox, 8, 2);

    m_wheelAmountLabel = new QLabel(i18n("Amount:"),this);
    layout->addWidget(m_wheelAmountLabel, 8, 1);

    m_wheelAmountToSpinBox = new QSpinBox(this);
    m_wheelAmountToSpinBox->setMaxValue(8191);
    m_wheelAmountToSpinBox->setValue(8191);
    layout->addWidget(m_wheelAmountToSpinBox, 8, 4);

    m_wheelToLabel = new QLabel(i18n("to:"), this);
    layout->addWidget(m_wheelToLabel, 7, 4);

    m_buttonCancel = new QPushButton(i18n("Cancel"), this);
    m_buttonCancel->setAutoDefault(true);
    layout->addWidget(m_buttonCancel, 13, 3);  // 8,5

    m_buttonOk = new QPushButton(i18n("OK"), this);
    m_buttonOk->setAutoDefault(true);
    m_buttonOk->setDefault(true);
    layout->addWidget(m_buttonOk, 13, 2);

    m_buttonNone = new QPushButton(i18n("None"),this);
    m_buttonNone->setAutoDefault(true);
    layout->addWidget(m_buttonNone, 13, 5);

    m_buttonAll = new QPushButton(i18n("All"),this);
    m_buttonAll->setAutoDefault(true);
    layout->addWidget(m_buttonAll, 13, 4);

    // line to set off buttons
    QFrame* line;
    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
//    line->setFrameShadow(QFrame::Sunken);
    layout->addMultiCellWidget(line, 12, 1, 12, 5);
    
    resize(QSize(500, 300).expandedTo(minimumSizeHint()));

    // signals and slots connections
    connect(m_buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_buttonAll, SIGNAL(clicked()), this, SLOT(slotToggleAll()));
    connect(m_buttonNone, SIGNAL(clicked()), this, SLOT(slotToggleNone()));
    
    connect(m_noteCheckBox, SIGNAL(stateChanged(int)), this,
	    SLOT(slotNoteCheckBoxToggle(int))); 
    connect(m_controllerCheckBox, SIGNAL(stateChanged(int)), this,
	    SLOT(slotControllerCheckBoxToggle(int)));
    connect(m_wheelCheckBox, SIGNAL(stateChanged(int)), this,
	    SLOT(slotWheelCheckBoxToggle(int)));
} // end ctor

// Destructor
EventFilterDialog::~EventFilterDialog()
{
    // nothing here
}

void EventFilterDialog::slotToggleAll()
{
    m_noteCheckBox->setChecked(true);
    m_controllerCheckBox->setChecked(true);
    m_wheelCheckBox->setChecked(true);
}

void EventFilterDialog::slotToggleNone()
{
    m_noteCheckBox->setChecked(false);
    m_controllerCheckBox->setChecked(false);
    m_wheelCheckBox->setChecked(false);
}

void EventFilterDialog::slotNoteCheckBoxToggle(int)
{
    bool state = m_noteCheckBox->isChecked();
    m_notePitchIncludeComboBox->setEnabled(state);
    m_noteVelocityIncludeComboBox->setEnabled(state);
    m_noteDurationIncludeComboBox->setEnabled(state);
    m_pitchToSpinBox->setEnabled(state);
    m_pitchFromSpinBox->setEnabled(state);
    m_velocityToSpinBox->setEnabled(state);
    m_velocityFromSpinBox->setEnabled(state);
    m_noteDurationToComboBox->setEnabled(state);
    m_noteDurationFromComboBox->setEnabled(state);
}

void EventFilterDialog::slotControllerCheckBoxToggle(int)
{
    bool state = m_controllerCheckBox->isChecked();
    m_controllerNumberIncludeComboBox->setEnabled(state);
    m_controllerValueIncludeComboBox->setEnabled(state);
    m_controllerNumberToSpinBox->setEnabled(state);
    m_controllerNumberFromSpinBox->setEnabled(state);
    m_controllerValueToSpinBox->setEnabled(state);
    m_controllerValueFromSpinBox->setEnabled(state);
}

void EventFilterDialog::slotWheelCheckBoxToggle(int)
{
    bool state = m_wheelCheckBox->isChecked();
    m_wheelAmountIncludeComboBox->setEnabled(state);
    m_wheelAmountToSpinBox->setEnabled(state);
    m_wheelAmountFromSpinBox->setEnabled(state);
}

void EventFilterDialog::slotPitchFromChanged(int pitch)
{
    Rosegarden::MidiPitchLabel pl(pitch);
    m_pitchFromValueLabel->setText(" " + pl.getQString());
}

void EventFilterDialog::slotPitchToChanged(int pitch)
{
    Rosegarden::MidiPitchLabel pl(pitch);
    m_pitchToValueLabel->setText(" " + pl.getQString());
}
