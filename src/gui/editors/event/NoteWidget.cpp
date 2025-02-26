/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[NoteWidget]"
#define RG_NO_DEBUG_PRINT

#include "NoteWidget.h"

#include "base/Event.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"

#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QSpinBox>


namespace Rosegarden
{


NoteWidget::NoteWidget(QWidget *parent, const Event &event) :
        EventWidget(parent)
{
    if (event.getType() != Note::EventType)
        return;

    // Note Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(tr("Note Properties"), this);
    m_propertiesGroupDEBUG = propertiesGroup;
    propertiesGroup->setContentsMargins(5, 5, 5, 5);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Pitch
    m_pitchLabel = new QLabel(tr("Pitch:"), propertiesGroup);
    propertiesLayout->addWidget(m_pitchLabel, row, 0);

    // ??? Make this a combo box with the note names.
    m_pitchSpinBox = new QSpinBox(propertiesGroup);
    m_pitchSpinBox->setMinimum(MidiMinValue);
    m_pitchSpinBox->setMaximum(MidiMaxValue);
    propertiesLayout->addWidget(m_pitchSpinBox, row, 1);

    m_pitchEditButton = new QPushButton(tr("edit"), propertiesGroup);
    propertiesLayout->addWidget(m_pitchEditButton, row, 2);

    //connect(m_pitchSpinBox,
    //            static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
    //        this, &EditEvent::slotPitchChanged);
    //connect(m_pitchEditButton, &QAbstractButton::released,
    //        this, &EditEvent::slotEditPitch);


    // Get the sizing correct...

    // ??? Why do we have to do this?  Usually all this stuff is automatic.
    //     Putting things in a QWidget appears to break sizing.

    // Adjust the properties group box to be big enough to hold the
    // controls.
    propertiesGroup->adjustSize();

    // Adjust the NoteWidget to be big enough to hold the properties group box.
    adjustSize();

    // Make the current size the minimum size.
    setMinimumSize(geometry().size());

}


}
