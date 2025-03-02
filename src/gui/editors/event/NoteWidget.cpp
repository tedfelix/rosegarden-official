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

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/PitchDialog.h"
#include "gui/dialogs/TimeDialog.h"

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
    propertiesGroup->setContentsMargins(5, 5, 5, 5);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Duration
    QLabel *durationLabel = new QLabel(tr("Duration:"), propertiesGroup);
    propertiesLayout->addWidget(durationLabel, row, 0);

    m_durationSpinBox = new QSpinBox(propertiesGroup);
    m_durationSpinBox->setMinimum(0);
    m_durationSpinBox->setMaximum(INT_MAX);
    m_durationSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_durationSpinBox->setValue(event.getDuration());
    propertiesLayout->addWidget(m_durationSpinBox, row, 1);

    QPushButton *durationEditButton =
            new QPushButton(tr("edit"), propertiesGroup);
    connect(durationEditButton, &QPushButton::clicked,
            this, &NoteWidget::slotEditDuration);
    propertiesLayout->addWidget(durationEditButton, row, 2);

    ++row;

    // Pitch
    QLabel *pitchLabel = new QLabel(tr("Pitch:"), propertiesGroup);
    propertiesLayout->addWidget(pitchLabel, row, 0);

    // ??? Make this a combo box with the note names.
    m_pitchSpinBox = new QSpinBox(propertiesGroup);
    m_pitchSpinBox->setMinimum(MidiMinValue);
    m_pitchSpinBox->setMaximum(MidiMaxValue);
    int pitch{0};
    if (event.has(BaseProperties::PITCH))
        pitch = event.get<Int>(BaseProperties::PITCH);
    m_pitchSpinBox->setValue(pitch);
    propertiesLayout->addWidget(m_pitchSpinBox, row, 1);

    m_pitchEditButton = new QPushButton(tr("edit"), propertiesGroup);
    connect(m_pitchEditButton, &QPushButton::clicked,
            this, &NoteWidget::slotEditPitch);
    propertiesLayout->addWidget(m_pitchEditButton, row, 2);


    // Get the sizing correct...

    // ??? Why do we have to do this?  Usually all this stuff is automatic.
    //     Putting things in a QWidget appears to break sizing.

    // ??? It would be nice if this thing's width would follow the parent's
    //     width.

    // Adjust the properties group box to be big enough to hold the
    // controls.
    propertiesGroup->adjustSize();

    // Adjust the NoteWidget to be big enough to hold the properties group box.
    adjustSize();

    // Make the current size the minimum size.
    setMinimumSize(geometry().size());

}

void
NoteWidget::slotEditDuration(bool /*checked*/)
{
#if 0
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    // ??? We need to be able to ask EditEvent for the current time as
    //     displayed on the absolute time spin box.
    const timeT startTime = ???;

    TimeDialog dialog(
            this,  // parent
            tr("Edit Duration"),  // title
            &composition,  // composition
            startTime,  // startTime
            m_durationSpinBox->value(),
            1,
            true);
    if (dialog.exec() == QDialog::Accepted)
        m_durationSpinBox->setValue(dialog.getTime());
#endif
}

timeT NoteWidget::getDuration()
{
    return m_durationSpinBox->value();
}

void
NoteWidget::slotEditPitch(bool /*checked*/)
{
    PitchDialog dialog(this, tr("Edit Pitch"), m_pitchSpinBox->value());
    if (dialog.exec() == QDialog::Accepted)
        m_pitchSpinBox->setValue(dialog.getPitch());
}


}
