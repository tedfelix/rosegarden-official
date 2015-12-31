/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "DiatonicPitchChooser.h"

#include <iostream>
#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "gui/general/MidiPitchLabel.h"
#include "PitchDragLabel.h"
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace Rosegarden
{

DiatonicPitchChooser::DiatonicPitchChooser(QString title,
                           QWidget *parent,
                           int defaultNote,
                           int defaultPitch,
                           int defaultOctave) :
        QGroupBox(title, parent),
        m_defaultPitch(defaultPitch)
{
    QVBoxLayout *layout = new QVBoxLayout;

    m_pitchDragLabel = new PitchDragLabel(this, defaultPitch);
    layout->addWidget(m_pitchDragLabel);

    QWidget *hbox = new QWidget(this);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hboxLayout->setSpacing(6);
    layout->addWidget(hbox);

    m_step = new QComboBox( hbox );
    hboxLayout->addWidget(m_step);
    m_step->setMaxVisibleItems( 7 );
    m_step->addItem(tr("C", "note name"));
    m_step->addItem(tr("D", "note name"));
    m_step->addItem(tr("E", "note name"));
    m_step->addItem(tr("F", "note name"));
    m_step->addItem(tr("G", "note name"));
    m_step->addItem(tr("A", "note name"));
    m_step->addItem(tr("B", "note name"));
    m_step->setCurrentIndex(defaultNote);

    m_octave = new QComboBox( hbox );
    hboxLayout->addWidget(m_octave);
    m_octave->addItem(tr("-2"));
    m_octave->addItem(tr("-1"));
    m_octave->addItem(tr("0"));
    m_octave->addItem(tr("1"));
    m_octave->addItem(tr("2"));
    m_octave->addItem(tr("3"));
    m_octave->addItem(tr("4"));
    m_octave->addItem(tr("5"));
    m_octave->addItem(tr("6"));
    m_octave->addItem(tr("7"));
    m_octave->setCurrentIndex(defaultOctave);

    m_accidental = new QComboBox( hbox );
    hboxLayout->addWidget(m_accidental);
    m_accidental->addItem(tr("double flat"));
    m_accidental->addItem(tr("flat"));
    m_accidental->addItem(tr("natural"));
    m_accidental->addItem(tr("sharp"));
    m_accidental->addItem(tr("double sharp"));
    m_accidental->setCurrentIndex(2); // default: natural

    m_pitchLabel = new QLabel(QString("%1").arg(getPitch()), hbox );
    hboxLayout->addWidget(m_pitchLabel);
    m_pitchLabel->setMinimumWidth(40);

    hbox->setLayout(hboxLayout);
    setLayout(layout);

    connect(m_accidental, SIGNAL(activated(int)),
            this, SLOT(slotSetAccidental(int)));

    connect(m_octave, SIGNAL(activated(int)),
            this, SLOT(slotSetOctave(int)));

    connect(m_step, SIGNAL(activated(int)),
            this, SLOT(slotSetStep(int)));

    //connect(m_pitch, SIGNAL(valueChanged(int)),
    //        this, SIGNAL(pitchChanged(int)));

    //connect(m_pitch, SIGNAL(valueChanged(int)),
    //        this, SIGNAL(preview(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchDragged(int,int,int)),
            this, SLOT(slotSetNote(int,int,int)));

    //connect(m_pitchDragLabel, SIGNAL(pitchChanged(int)),
    //        this, SLOT(slotSetPitch(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchChanged(int,int,int)),
            this, SLOT(slotSetNote(int,int,int)));

    //connect(m_pitchDragLabel, SIGNAL(pitchChanged(int)),
    //        this, SIGNAL(pitchChanged(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchDragged(int,int,int)),
            this, SIGNAL(noteChanged(int,int,int)));

    connect(m_pitchDragLabel, SIGNAL(pitchChanged(int,int,int)),
            this, SIGNAL(noteChanged(int,int,int)));

    connect(m_pitchDragLabel, SIGNAL(preview(int)),
            this, SIGNAL(preview(int)));

}

int
DiatonicPitchChooser::getPitch() const
{
    return 12 * m_octave->currentIndex() + scale_Cmajor[m_step->currentIndex()] + 
    	(m_accidental->currentIndex() - 2);
}

int 
DiatonicPitchChooser::getAccidental()
{
    return m_accidental->currentIndex() - 2;
}

void
DiatonicPitchChooser::slotSetPitch(int pitch)
{
    if (m_pitchDragLabel->getPitch() != pitch)
        m_pitchDragLabel->slotSetPitch(pitch);

    m_octave->setCurrentIndex((int)(((long) pitch) / 12));

    int step = steps_Cmajor[pitch % 12];
    m_step->setCurrentIndex(step);
    
    int pitchChange = (pitch % 12) - scale_Cmajor[step];
    
    m_accidental->setCurrentIndex(pitchChange + 2);

    m_pitchLabel->setText(QString("%1").arg(pitch));
    
    update();
}

void
DiatonicPitchChooser::slotSetStep(int step)
{
    if (m_step->currentIndex() != step)
       m_step->setCurrentIndex(step);
    std::cout << "slot_step called" << std::endl;
    setLabelsIfNeeded();
    update();
}

void
DiatonicPitchChooser::slotSetOctave(int octave)
{
    if (m_octave->currentIndex() != octave)
       m_octave->setCurrentIndex(octave);
    setLabelsIfNeeded();
    update();
}

/** input 0..4: doubleflat .. doublesharp */
void
DiatonicPitchChooser::slotSetAccidental(int accidental)
{
    if (m_accidental->currentIndex() != accidental)
       m_accidental->setCurrentIndex(accidental);
    setLabelsIfNeeded();
    update();
}

/** sets the m_pitchDragLabel and m_pitchLabel if needed */
void
DiatonicPitchChooser::setLabelsIfNeeded()
{
    //if (m_pitchDragLabel->)
    //{
       m_pitchDragLabel->slotSetPitch(getPitch(), m_octave->currentIndex(), m_step->currentIndex());
    //}
    m_pitchLabel->setText(QString("%1").arg(getPitch()));
}

void
DiatonicPitchChooser::slotSetNote(int pitch, int octave, int step)
{
    //if (m_pitch->value() != p)
    //    m_pitch->setValue(p);
    if (m_pitchDragLabel->getPitch() != pitch)
        m_pitchDragLabel->slotSetPitch(pitch, octave, step);

    m_octave->setCurrentIndex(octave);
    m_step->setCurrentIndex(step);
    
    int pitchOffset = pitch - (octave * 12 + scale_Cmajor[step]);
    m_accidental->setCurrentIndex(pitchOffset + 2);

    //MidiPitchLabel pl(p);
    m_pitchLabel->setText(QString("%1").arg(pitch));
    update();
}

void
DiatonicPitchChooser::slotResetToDefault()
{
    slotSetPitch(m_defaultPitch);
}

int
DiatonicPitchChooser::getOctave() const
{
    return m_octave->currentIndex();
}


int
DiatonicPitchChooser::getStep() const
{
    return m_step->currentIndex();
}

}
