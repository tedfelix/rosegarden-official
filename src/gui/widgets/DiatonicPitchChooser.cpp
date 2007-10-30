/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "DiatonicPitchChooser.h"

#include <iostream>
#include <klocale.h>
#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "gui/general/MidiPitchLabel.h"
#include "PitchDragLabel.h"
#include <kcombobox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qwidget.h>

namespace Rosegarden
{

DiatonicPitchChooser::DiatonicPitchChooser(QString title,
                           QWidget *parent,
                           int defaultNote,
                           int defaultPitch,
                           int defaultOctave) :
        QGroupBox(1, Horizontal, title, parent),
        m_defaultPitch(defaultPitch)
{
    m_pitchDragLabel = new PitchDragLabel(this, defaultPitch);

    QHBox *hbox = new QHBox(this);
    hbox->setSpacing(6);

    m_step = new KComboBox( hbox );
    m_step->setSizeLimit( 7 );
    m_step->insertItem(i18n("C"));
    m_step->insertItem(i18n("D"));
    m_step->insertItem(i18n("E"));
    m_step->insertItem(i18n("F"));
    m_step->insertItem(i18n("G"));
    m_step->insertItem(i18n("A"));
    m_step->insertItem(i18n("B"));
    m_step->setCurrentItem(defaultNote);

    m_octave = new KComboBox( hbox );
    m_octave->insertItem(i18n("-2"));
    m_octave->insertItem(i18n("-1"));
    m_octave->insertItem(i18n("0"));
    m_octave->insertItem(i18n("1"));
    m_octave->insertItem(i18n("2"));
    m_octave->insertItem(i18n("3"));
    m_octave->insertItem(i18n("4"));
    m_octave->insertItem(i18n("5"));
    m_octave->insertItem(i18n("6"));
    m_octave->insertItem(i18n("7"));
    m_octave->setCurrentItem(defaultOctave);

    m_accidental = new KComboBox( hbox );
    m_accidental->insertItem(i18n("double flat"));
    m_accidental->insertItem(i18n("flat"));
    m_accidental->insertItem(i18n("natural"));
    m_accidental->insertItem(i18n("sharp"));
    m_accidental->insertItem(i18n("double sharp"));
    m_accidental->setCurrentItem(2); // default: natural

    m_pitchLabel = new QLabel(QString("%1").arg(getPitch()), hbox);
    
    m_pitchLabel->setMinimumWidth(40);

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
    return 12 * m_octave->currentItem() + scale_Cmajor[m_step->currentItem()] + 
    	(m_accidental->currentItem() - 2);
}

int 
DiatonicPitchChooser::getAccidental()
{
    return m_accidental->currentItem() - 2;
}

void
DiatonicPitchChooser::slotSetPitch(int pitch)
{
    if (m_pitchDragLabel->getPitch() != pitch)
        m_pitchDragLabel->slotSetPitch(pitch);

    m_octave->setCurrentItem((int)(((long) pitch) / 12));

    int step = steps_Cmajor[pitch % 12];
    m_step->setCurrentItem(step);
    
    int pitchChange = (pitch % 12) - scale_Cmajor[step];
    
    m_accidental->setCurrentItem(pitchChange + 2);

    m_pitchLabel->setText(QString("%1").arg(pitch));
    
    update();
}

void
DiatonicPitchChooser::slotSetStep(int step)
{
    if (m_step->currentItem() != step)
       m_step->setCurrentItem(step);
    std::cout << "slot_step called" << std::endl;
    setLabelsIfNeeded();
    update();
}

void
DiatonicPitchChooser::slotSetOctave(int octave)
{
    if (m_octave->currentItem() != octave)
       m_octave->setCurrentItem(octave);
    setLabelsIfNeeded();
    update();
}

/** input 0..4: doubleflat .. doublesharp */
void
DiatonicPitchChooser::slotSetAccidental(int accidental)
{
    if (m_accidental->currentItem() != accidental)
       m_accidental->setCurrentItem(accidental);
    setLabelsIfNeeded();
    update();
}

/** sets the m_pitchDragLabel and m_pitchLabel if needed */
void
DiatonicPitchChooser::setLabelsIfNeeded()
{
    //if (m_pitchDragLabel->)
    //{
       m_pitchDragLabel->slotSetPitch(getPitch(), m_octave->currentItem(), m_step->currentItem());
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

    m_octave->setCurrentItem(octave);
    m_step->setCurrentItem(step);
    
    int pitchOffset = pitch - (octave * 12 + scale_Cmajor[step]);
    m_accidental->setCurrentItem(pitchOffset + 2);

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
    return m_octave->currentItem();
}


int
DiatonicPitchChooser::getStep() const
{
    return m_step->currentItem();
}

}
#include "DiatonicPitchChooser.moc"
