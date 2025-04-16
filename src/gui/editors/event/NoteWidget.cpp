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

#include "EditEvent.h"

#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/MidiProgram.h"  // For MidiMinValue, etc...
#include "base/NotationTypes.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/PitchDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/general/MidiPitchLabel.h"
#include "misc/PreferenceInt.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>


namespace Rosegarden
{


namespace
{

    QString NoteWidgetGroup{"NoteWidget"};
    PreferenceInt a_durationSetting(NoteWidgetGroup, "Duration", 960);
    PreferenceInt a_pitchSetting(NoteWidgetGroup, "Pitch", 60);
    PreferenceInt a_velocitySetting(NoteWidgetGroup, "Velocity", 100);

}


NoteWidget::NoteWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent),
    m_parent(parent)
{
    if (event.getType() != Note::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);
    // Get rid of any extra margins introduced by the layout.
    mainLayout->setContentsMargins(0,0,0,0);

    // Note Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(tr("Note Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Duration
    QLabel *durationLabel = new QLabel(tr("Duration:"), propertiesGroup);
    propertiesLayout->addWidget(durationLabel, row, 0);

    m_durationSpinBox = new QSpinBox(propertiesGroup);
    m_durationSpinBox->setMinimum(1);
    m_durationSpinBox->setMaximum(INT_MAX);
    m_durationSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    timeT duration{event.getDuration()};
    if (duration == 0)
        duration = a_durationSetting.get();
    m_durationSpinBox->setValue(duration);
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

    m_pitchComboBox = new QComboBox(propertiesGroup);
    for (int pitch = 0; pitch < 128; ++pitch) {
        m_pitchComboBox->addItem(QString("%1 (%2)").
                arg(MidiPitchLabel::pitchToString(pitch)).
                arg(pitch));
    }
    int pitch{a_pitchSetting.get()};
    if (event.has(BaseProperties::PITCH))
        pitch = event.get<Int>(BaseProperties::PITCH);
    m_pitchComboBox->setCurrentIndex(pitch);
    propertiesLayout->addWidget(m_pitchComboBox, row, 1);

    QPushButton *pitchEditButton =
            new QPushButton(tr("edit"), propertiesGroup);
    connect(pitchEditButton, &QPushButton::clicked,
            this, &NoteWidget::slotEditPitch);
    propertiesLayout->addWidget(pitchEditButton, row, 2);

    ++row;

    // Velocity
    QLabel *velocityLabel = new QLabel(tr("Velocity:"), propertiesGroup);
    propertiesLayout->addWidget(velocityLabel, row, 0);

    m_velocitySpinBox = new QSpinBox(propertiesGroup);
    m_velocitySpinBox->setMinimum(MidiMinValue);
    m_velocitySpinBox->setMaximum(MidiMaxValue);
    int velocity{a_velocitySetting.get()};
    if (event.has(BaseProperties::VELOCITY))
        velocity = event.get<Int>(BaseProperties::VELOCITY);
    m_velocitySpinBox->setValue(velocity);
    propertiesLayout->addWidget(m_velocitySpinBox, row, 1);

    ++row;

    // Lock notation
    m_lockNotation =
            new QCheckBox(tr("Lock notation to performance"), propertiesGroup);
    // ??? What's the best way to handle locking?  The simplest is just to
    //     lock and lose the performance/notation difference.  That's what
    //     we've done here.  For big changes to the performance values, this
    //     makes sense.  For small changes, the user might want to unlock and
    //     leave the notation values alone.  The following commented-out lines
    //     will do that.  Going with the simplest for now.
    //const bool lockNotation =
    //        (event.getNotationAbsoluteTime() == event.getAbsoluteTime()  &&
    //         event.getNotationDuration() == event.getDuration());
    //m_lockNotationValues->setChecked(lockNotation);
    m_lockNotation->setChecked(true);
    connect(m_lockNotation, &QPushButton::clicked,
            this, &NoteWidget::slotLockNotationClicked);
    propertiesLayout->addWidget(m_lockNotation, row, 0, 1, 3);

    ++row;

    // Notation time
    m_notationTimeLabel = new QLabel(tr("Notation time:"), propertiesGroup);
    propertiesLayout->addWidget(m_notationTimeLabel, row, 0);

    m_notationTimeSpinBox = new QSpinBox(propertiesGroup);
    m_notationTimeSpinBox->setMinimum(INT_MIN);
    m_notationTimeSpinBox->setMaximum(INT_MAX);
    m_notationTimeSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_notationTimeSpinBox->setValue(event.getNotationAbsoluteTime());
    propertiesLayout->addWidget(m_notationTimeSpinBox, row, 1);

    m_notationTimeEditButton = new QPushButton(tr("edit"), propertiesGroup);
    connect(m_notationTimeEditButton, &QPushButton::clicked,
            this, &NoteWidget::slotNotationTimeEditClicked);
    propertiesLayout->addWidget(m_notationTimeEditButton, row, 2);

    ++row;

    // Notation duration
    m_notationDurationLabel =
            new QLabel(tr("Notation duration:"), propertiesGroup);
    propertiesLayout->addWidget(m_notationDurationLabel, row, 0);

    m_notationDurationSpinBox = new QSpinBox(propertiesGroup);
    m_notationDurationSpinBox->setMinimum(0);
    m_notationDurationSpinBox->setMaximum(INT_MAX);
    m_notationDurationSpinBox->setSingleStep(
            Note(Note::Shortest).getDuration());
    m_notationDurationSpinBox->setValue(event.getNotationDuration());
    propertiesLayout->addWidget(m_notationDurationSpinBox, row, 1);

    m_notationDurationEditButton = new QPushButton(tr("edit"), propertiesGroup);
    connect(m_notationDurationEditButton, &QPushButton::clicked,
            this, &NoteWidget::slotNotationDurationEditClicked);
    propertiesLayout->addWidget(m_notationDurationEditButton, row, 2);

    // Sync up notation widget enable state.
    slotLockNotationClicked(true);
}

NoteWidget::~NoteWidget()
{
    a_durationSetting.set(m_durationSpinBox->value());
    a_pitchSetting.set(m_pitchComboBox->currentIndex());
    a_velocitySetting.set(m_velocitySpinBox->value());
}

EventWidget::PropertyNameSet
NoteWidget::getPropertyFilter() const
{
    return PropertyNameSet{
            BaseProperties::PITCH, BaseProperties::VELOCITY,
            Event::NotationTime, Event::NotationDuration};
}

void
NoteWidget::slotEditDuration(bool /*checked*/)
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    // This is needed to get the correct bar counts based on the current
    // time signature.  E.g. in 4/4, 3840 is one bar, in 2/4, 3840 is two bars.
    const timeT startTime = m_parent->getAbsoluteTime();

    TimeDialog dialog(
            this,  // parent
            tr("Edit Duration"),  // title
            &composition,  // composition
            startTime,  // startTime
            m_durationSpinBox->value(),  // defaultDuration
            1,  // minimumDuration
            true);  // constrainToCompositionDuration
    if (dialog.exec() == QDialog::Accepted)
        m_durationSpinBox->setValue(dialog.getTime());
}

void
NoteWidget::slotEditPitch(bool /*checked*/)
{
    PitchDialog dialog(this, tr("Edit Pitch"), m_pitchComboBox->currentIndex());
    if (dialog.exec() == QDialog::Accepted)
        m_pitchComboBox->setCurrentIndex(dialog.getPitch());
}

void
NoteWidget::slotLockNotationClicked(bool checked)
{
    m_notationTimeLabel->setEnabled(!checked);
    m_notationTimeSpinBox->setEnabled(!checked);
    m_notationTimeEditButton->setEnabled(!checked);
    m_notationDurationLabel->setEnabled(!checked);
    m_notationDurationSpinBox->setEnabled(!checked);
    m_notationDurationEditButton->setEnabled(!checked);
}

void NoteWidget::slotNotationTimeEditClicked(bool /*checked*/)
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    TimeDialog dialog(
            this,  // parent
            tr("Edit Event Notation Time"),  // title
            &composition,  // composition
            m_notationTimeSpinBox->value(),  // defaultTime
            true);  // constrainToCompositionDuration
    if (dialog.exec() == QDialog::Accepted)
        m_notationTimeSpinBox->setValue(dialog.getTime());
}

void NoteWidget::slotNotationDurationEditClicked(bool /*checked*/)
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    TimeDialog dialog(
            this,  // parent
            tr("Edit Notation Duration"),  // title
            &composition,  // composition
            m_notationTimeSpinBox->value(),  // startTime
            m_notationDurationSpinBox->value(),  // defaultDuration
            1,  // minimumDuration
            true);  // constrainToCompositionDuration
    if (dialog.exec() == QDialog::Accepted)
        m_notationDurationSpinBox->setValue(dialog.getTime());
}

void NoteWidget::updateEvent(Event &event) const
{
    event.setDuration(m_durationSpinBox->value());
    event.set<Int>(BaseProperties::PITCH, m_pitchComboBox->currentIndex());
    event.set<Int>(BaseProperties::VELOCITY, m_velocitySpinBox->value());
    if (m_lockNotation->isChecked()) {
        event.setNotationAbsoluteTime(m_parent->getAbsoluteTime());
        event.setNotationDuration(m_durationSpinBox->value());
    } else {
        event.setNotationAbsoluteTime(m_notationTimeSpinBox->value());
        event.setNotationDuration(m_notationDurationSpinBox->value());
    }
}


}
