/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PitchBendSequenceDialog.h"
#include "base/RealTime.h"
#include "base/MidiTypes.h"
#include "commands/edit/EventInsertionCommand.h"
#include "document/CommandHistory.h"
#include "document/Command.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>

namespace Rosegarden
{

PitchBendSequenceDialog::PitchBendSequenceDialog(QWidget *parent, Segment *segment, timeT startTime, timeT endTime) :
    QDialog(parent),
    m_segment(segment),
    m_startTime(startTime),
    m_endTime(endTime)
{
    setModal(true);
    setWindowTitle(tr("Pitch Bend Sequence"));

    QWidget* vbox = dynamic_cast<QWidget*>( this );
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vbox->setLayout(vboxLayout);

    QGroupBox *prebendbox = new QGroupBox(tr("Pre Bend"), vbox);
    prebendbox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *prebendgrid = new QGridLayout;
    prebendgrid->setSpacing(5);
    vboxLayout->addWidget(prebendbox);

    prebendgrid->addWidget(new QLabel(tr("Value %:"), prebendbox), 0, 0);
    m_prebendValue = new QDoubleSpinBox(prebendbox);
    m_prebendValue->setMaximum(100);
    m_prebendValue->setMinimum(-100);
    m_prebendValue->setSingleStep(5);
    prebendgrid->addWidget(m_prebendValue, 0 , 1);
    
    prebendbox->setLayout(prebendgrid);

    if (m_startTime < m_endTime) {
        QLabel *durationLabel = new QLabel(tr("Duration %:"), prebendbox);
        prebendgrid->addWidget(durationLabel, 1, 0);
        m_prebendDuration = new QDoubleSpinBox(prebendbox);
        m_prebendDuration->setMaximum(100);
        m_prebendDuration->setMinimum(0);
        m_prebendDuration->setSingleStep(5);
        prebendgrid->addWidget(m_prebendDuration, 1 , 1);

        QGroupBox *sequencebox = new QGroupBox(tr("Bend Sequence"), vbox );
        sequencebox->setContentsMargins(5, 5, 5, 5);
        QGridLayout *sequencegrid = new QGridLayout;
        sequencegrid->setSpacing(5);
        vboxLayout->addWidget(sequencebox);

        sequencegrid->addWidget(new QLabel(tr("Preset:"), sequencebox), 0, 0);
        m_sequencePreset = new QComboBox(sequencebox);
        sequencegrid->addWidget(m_sequencePreset, 0, 1);
        m_sequencePreset->addItem(tr("Linear ramp"));
        m_sequencePreset->addItem(tr("Fast vibrato arm release"));
        m_sequencePreset->addItem(tr("Vibrato"));

        sequencegrid->addWidget(new QLabel(tr("Ramp duration %:"), sequencebox), 1, 0);
        m_sequenceRampDuration = new QDoubleSpinBox(sequencebox);
        m_sequenceRampDuration->setMaximum(100);
        m_sequenceRampDuration->setMinimum(0);
        m_sequenceRampDuration->setSingleStep(5);
        sequencegrid->addWidget(m_sequenceRampDuration, 1, 1);

        sequencegrid->addWidget(new QLabel(tr("End value %:"), sequencebox), 2, 0);
        m_sequenceEndValue = new QDoubleSpinBox(sequencebox);
        m_sequenceEndValue->setMaximum(100);
        m_sequenceEndValue->setMinimum(-100);
        m_sequenceEndValue->setSingleStep(5);
        sequencegrid->addWidget(m_sequenceEndValue, 2, 1);

        sequencegrid->addWidget(new QLabel(tr("Vibrato start amplitude %:"), 
                                sequencebox), 3, 0);
        m_vibratoStartAmplitude = new QDoubleSpinBox(sequencebox);
        m_vibratoStartAmplitude->setMaximum(100);
        m_vibratoStartAmplitude->setMinimum(0);
        m_vibratoStartAmplitude->setSingleStep(10);
        m_vibratoStartAmplitude->setValue(0);
        sequencegrid->addWidget(m_vibratoStartAmplitude, 3, 1);

        sequencegrid->addWidget(new QLabel(tr("Vibrato end amplitude %:"),
                                sequencebox), 4, 0);
        m_vibratoEndAmplitude = new QDoubleSpinBox(sequencebox);
        m_vibratoEndAmplitude->setMaximum(100);
        m_vibratoEndAmplitude->setMinimum(0);
        m_vibratoEndAmplitude->setSingleStep(10);
        m_vibratoEndAmplitude->setValue(0);
        sequencegrid->addWidget(m_vibratoEndAmplitude, 4, 1);

        sequencegrid->addWidget(new QLabel(tr("Vibrato wavelength:"), 
                            sequencebox), 5, 0);
        m_vibratoWaveLenght = new QDoubleSpinBox(sequencebox);
        m_vibratoWaveLenght->setMaximum(200);
        m_vibratoWaveLenght->setMinimum(2);
        m_vibratoWaveLenght->setSingleStep(5);
        m_vibratoWaveLenght->setValue(10);
        m_vibratoWaveLenght->setDecimals(0);
        sequencegrid->addWidget(m_vibratoWaveLenght, 5, 1);

        sequencegrid->addWidget(new QLabel(tr("Resolution:"), sequencebox), 6, 0);
        m_resolution = new QDoubleSpinBox(sequencebox);
        m_resolution->setMaximum(300);
        m_resolution->setMinimum(10);
        m_resolution->setSingleStep(10);
        m_resolution->setValue(100);
        m_resolution->setDecimals(0);
        sequencegrid->addWidget(m_resolution, 6, 1);

        sequencebox->setLayout(sequencegrid);

        slotSequencePresetChanged(m_sequencePreset->currentIndex());
        connect(m_sequencePreset, SIGNAL(activated(int)), this,
                SLOT(slotSequencePresetChanged(int)));
    } else {
        vboxLayout->addWidget(new QLabel(tr("Invalid end time. Have you selected some events?")));
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    vboxLayout->addWidget(buttonBox, 1, 0);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
PitchBendSequenceDialog::slotSequencePresetChanged(int index) {
    switch (index) {
      case 0: printf ("a\n");
        m_prebendDuration->setValue(0);
        m_sequenceRampDuration->setValue(100);
        m_vibratoStartAmplitude->setValue(0);
        m_vibratoEndAmplitude->setValue(0);
        break;
      case 1:
        m_prebendValue->setValue(-20);
        m_prebendDuration->setValue(5);
        m_sequenceRampDuration->setValue(0);
        m_sequenceEndValue->setValue(0);
        m_vibratoStartAmplitude->setValue(30);
        m_vibratoEndAmplitude->setValue(0);
        m_vibratoWaveLenght->setValue(2);
        break;
      case 2:
        m_prebendValue->setValue(0);
        m_prebendDuration->setValue(0);
        m_sequenceRampDuration->setValue(0);
        m_sequenceEndValue->setValue(0);
        m_vibratoStartAmplitude->setValue(10);
        m_vibratoEndAmplitude->setValue(10);
        m_vibratoWaveLenght->setValue(10);
    }	
}

void
PitchBendSequenceDialog::accept()
{
    int startvalue = (m_prebendValue->value()/100.0) * 8192.0 + 8192;

    if (startvalue < 0) startvalue = 0;
    if (startvalue > 16383) startvalue = 16383;

    MacroCommand *macro = new MacroCommand(tr("Pitch Bend Sequence"));

    Event *event = new Event (Rosegarden::PitchBend::EventType, m_startTime);
    event->set<Rosegarden::Int>(Rosegarden::PitchBend::MSB, (startvalue >> 7) & 0x7f);
    event->set<Rosegarden::Int>(Rosegarden::PitchBend::LSB, startvalue & 0x7f);
    
    macro->addCommand(new EventInsertionCommand (*m_segment, event));

    if (m_startTime < m_endTime) {
	timeT seqStartTime = m_startTime+(m_prebendDuration->value()*
                                          (m_endTime-m_startTime))/100;
        timeT rampEndTime = seqStartTime+(m_sequenceRampDuration->value()*
                                          (m_endTime-seqStartTime))/100;
        timeT duration = m_endTime - seqStartTime;
        
        int steps = (int) m_resolution->value();
        int vibratoValue=0;
        int vibratoWL=m_vibratoWaveLenght->value();
        int vibratoHWL = vibratoWL/2;
        int vibratoSA = m_vibratoStartAmplitude->value()*8192.0/100.0;
        int vibratoEA = m_vibratoEndAmplitude->value()*8192.0/100.0;
        int endvalue = (m_sequenceEndValue->value()/100.0) * 8192.0 + 8192;
        int value = endvalue;

        if (endvalue < 0) endvalue = 0;
        if (endvalue > 16383) endvalue = 16383;

        for ( int i = 1 ; i < steps ; i++) {
            timeT time = seqStartTime + (timeT) i*duration/(timeT) steps;

	    if (time > rampEndTime) {
		value = endvalue;
	    } else {
		value = ((endvalue - startvalue)*(time - seqStartTime)/(rampEndTime-seqStartTime)) + startvalue;
	    }

            int modulo = i%vibratoWL;
	    int amplitude = (vibratoEA - vibratoSA)*i/steps+ vibratoSA;
	    if (modulo > vibratoHWL) modulo = vibratoHWL-(modulo-vibratoHWL);
	    vibratoValue = 4*amplitude*modulo/vibratoWL-amplitude;
	    value = value + vibratoValue;

            if (value < 0) value = 0;
            if (value > 16383) value = 16383;

            Event *event = new Event (Rosegarden::PitchBend::EventType,time);
            event->set<Rosegarden::Int>(Rosegarden::PitchBend::MSB, (value >> 7) & 0x7f);
            event->set<Rosegarden::Int>(Rosegarden::PitchBend::LSB, value & 0x7f);

            macro->addCommand(new EventInsertionCommand (*m_segment, event));
        }
    }
    CommandHistory::getInstance()->addCommand(macro);

    QDialog::accept();
}


}

#include "PitchBendSequenceDialog.moc"
