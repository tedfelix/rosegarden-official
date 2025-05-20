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

#define RG_MODULE_STRING "[PitchChooser]"
#define RG_NO_DEBUG_PRINT

#include "PitchChooser.h"

#include "PitchDragLabel.h"

#include "base/Pitch.h"

#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>


namespace Rosegarden
{


PitchChooser::PitchChooser(const QString &title,
                           QWidget *parent,
                           int defaultPitch) :
    QGroupBox(title, parent),
    m_defaultPitch(defaultPitch)
{
    m_layout = new QVBoxLayout;

    // Edit Pitch
    m_pitchDragLabel = new PitchDragLabel(this, defaultPitch);
    m_layout->addWidget(m_pitchDragLabel);

    QWidget *hbox = new QWidget(this);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hboxLayout->setSpacing(6);
    m_layout->addWidget(hbox);

    // Pitch:
    QLabel *child_4 = new QLabel(tr("Pitch:"), hbox);
    hboxLayout->addWidget(child_4);

    m_pitch = new QSpinBox( hbox );
    hboxLayout->addWidget(m_pitch);
    m_pitch->setMinimum(0);
    m_pitch->setMaximum(127);
    m_pitch->setValue(defaultPitch);

    m_pitchLabel = new QLabel(Pitch::toStringOctave(defaultPitch), hbox);
    hboxLayout->addWidget(m_pitchLabel);
    hbox->setLayout(hboxLayout);
    m_pitchLabel->setMinimumWidth(40);

    setLayout(m_layout);

    // ??? Three connections is expensive CPU-wise.  Make a single
    //     connection to a single routine that calls the other three.
    connect(m_pitch, (void(QSpinBox::*)(int))&QSpinBox::valueChanged,
            this, &PitchChooser::slotSetPitch);
    connect(m_pitch, (void(QSpinBox::*)(int))&QSpinBox::valueChanged,
            this, &PitchChooser::pitchChanged);
    connect(m_pitch, (void(QSpinBox::*)(int))&QSpinBox::valueChanged,
            this, &PitchChooser::preview);

    connect(m_pitchDragLabel, (void(PitchDragLabel::*)(int))
                    &PitchDragLabel::pitchDragged,
            this, &PitchChooser::slotSetPitch);

    // ??? Two connections is a bit expensive CPU-wise.  Make a single
    //     connection to a single routine that calls the other two.
    connect(m_pitchDragLabel, (void(PitchDragLabel::*)(int))
                    &PitchDragLabel::pitchChanged,
            this, &PitchChooser::slotSetPitch);
    connect(m_pitchDragLabel, (void(PitchDragLabel::*)(int))
                    &PitchDragLabel::pitchChanged,
            this, &PitchChooser::pitchChanged);

    connect(m_pitchDragLabel, &PitchDragLabel::preview,
            this, &PitchChooser::preview);

}

void
PitchChooser::addWidgetToLayout(QWidget *widget)
{
    m_layout->addWidget(widget);
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

    m_pitchLabel->setText(Pitch::toStringOctave(p));
    update();
}

void
PitchChooser::slotResetToDefault()
{
    slotSetPitch(m_defaultPitch);
}

}
