/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "PitchChooser.h"

#include <klocale.h>
#include "gui/general/MidiPitchLabel.h"
#include "PitchDragLabel.h"
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

PitchChooser::PitchChooser(QString title,
                           QWidget *parent,
                           int defaultPitch) :
        QGroupBox(1, Horizontal, title, parent),
        m_defaultPitch(defaultPitch)
{
    m_pitchDragLabel = new PitchDragLabel(this, defaultPitch);

    QHBox *hbox = new QHBox(this);
    hbox->setSpacing(6);

    new QLabel(i18n("Pitch:"), hbox);

    m_pitch = new QSpinBox(hbox);
    m_pitch->setMinValue(0);
    m_pitch->setMaxValue(127);
    m_pitch->setValue(defaultPitch);

    MidiPitchLabel pl(defaultPitch);
    m_pitchLabel = new QLabel(pl.getQString(), hbox);
    m_pitchLabel->setMinimumWidth(40);

    connect(m_pitch, SIGNAL(valueChanged(int)),
            this, SLOT(slotSetPitch(int)));

    connect(m_pitch, SIGNAL(valueChanged(int)),
            this, SIGNAL(pitchChanged(int)));

    connect(m_pitch, SIGNAL(valueChanged(int)),
            this, SIGNAL(preview(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchDragged(int)),
            this, SLOT(slotSetPitch(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchChanged(int)),
            this, SLOT(slotSetPitch(int)));

    connect(m_pitchDragLabel, SIGNAL(pitchChanged(int)),
            this, SIGNAL(pitchChanged(int)));

    connect(m_pitchDragLabel, SIGNAL(preview(int)),
            this, SIGNAL(preview(int)));

}

int
PitchChooser::getPitch() const
{
    return m_pitch->value();
}

void
PitchChooser::slotSetPitch(int p)
{
    if (m_pitch->value() != p)
        m_pitch->setValue(p);
    if (m_pitchDragLabel->getPitch() != p)
        m_pitchDragLabel->slotSetPitch(p);

    MidiPitchLabel pl(p);
    m_pitchLabel->setText(pl.getQString());
    update();
}

void
PitchChooser::slotResetToDefault()
{
    slotSetPitch(m_defaultPitch);
}

}
#include "PitchChooser.moc"
