/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SimpleEventEditDialog.h"
#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/guitar/Chord.h"
#include "misc/Strings.h"
#include "PitchDialog.h"
#include "TimeDialog.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/FileDialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFile>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QLayout>



namespace Rosegarden
{

SimpleEventEditDialog::SimpleEventEditDialog(QWidget *parent,
        RosegardenDocument *doc,
        const Event &event,
        bool inserting) :
        QDialog(parent),
        m_event(event),
        m_doc(doc),
        m_type(event.getType()),
        m_absoluteTime(event.getAbsoluteTime()),
        m_duration(event.getDuration()),
        m_modified(false)
{
    setModal(true);
    setWindowTitle(tr(inserting ?
                          tr("Insert Event").toStdString().c_str() :
                          tr("Edit Event").toStdString().c_str()));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    QGroupBox *frame = new QGroupBox( tr("Event Properties"), vbox );
    frame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);
    vboxLayout->addWidget(frame);

    layout->addWidget(new QLabel(tr("Event type:"), frame), 0, 0);

    if (inserting) {

        m_typeLabel = 0;

        m_typeCombo = new QComboBox(frame);
        layout->addWidget(m_typeCombo, 0, 1);

        m_typeCombo->addItem(strtoqstr(Note::EventType));
        m_typeCombo->addItem(strtoqstr(Controller::EventType));
        m_typeCombo->addItem(strtoqstr(KeyPressure::EventType));
        m_typeCombo->addItem(strtoqstr(ChannelPressure::EventType));
        m_typeCombo->addItem(strtoqstr(ProgramChange::EventType));
        m_typeCombo->addItem(strtoqstr(SystemExclusive::EventType));
        m_typeCombo->addItem(strtoqstr(PitchBend::EventType));
        m_typeCombo->addItem(strtoqstr(Indication::EventType));
        m_typeCombo->addItem(strtoqstr(Text::EventType));
        m_typeCombo->addItem(strtoqstr(Note::EventRestType));
        m_typeCombo->addItem(strtoqstr(Clef::EventType));
        m_typeCombo->addItem(strtoqstr(::Rosegarden::Key::EventType));
        m_typeCombo->addItem(strtoqstr(Guitar::Chord::EventType));

        // Connect up the combos
        //
        connect(m_typeCombo, SIGNAL(activated(int)),
                SLOT(slotEventTypeChanged(int)));

    } else {

        m_typeCombo = 0;

        m_typeLabel = new QLabel(frame);
        layout->addWidget(m_typeLabel, 0, 1);
    }

    m_timeLabel = new QLabel(tr("Absolute time:"), frame);
    layout->addWidget(m_timeLabel, 1, 0);
    m_timeSpinBox = new QSpinBox(frame);
    m_timeSpinBox->setMinimum(INT_MIN);
    m_timeSpinBox->setMaximum(INT_MAX);
    m_timeSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_timeEditButton = new QPushButton(tr("edit"), frame);
    layout->addWidget(m_timeSpinBox, 1, 1);
    layout->addWidget(m_timeEditButton, 1, 2);

    connect(m_timeSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotAbsoluteTimeChanged(int)));
    connect(m_timeEditButton, SIGNAL(released()),
            SLOT(slotEditAbsoluteTime()));

    m_durationLabel = new QLabel(tr("Duration:"), frame);
    layout->addWidget(m_durationLabel, 2, 0);
    m_durationSpinBox = new QSpinBox(frame);
    m_durationSpinBox->setMinimum(0);
    m_durationSpinBox->setMaximum(INT_MAX);
    m_durationSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_durationEditButton = new QPushButton(tr("edit"), frame);
    layout->addWidget(m_durationSpinBox, 2, 1);
    layout->addWidget(m_durationEditButton, 2, 2);

    connect(m_durationSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotDurationChanged(int)));
    connect(m_durationEditButton, SIGNAL(released()),
            SLOT(slotEditDuration()));

    m_pitchLabel = new QLabel(tr("Pitch:"), frame);
    layout->addWidget(m_pitchLabel, 3, 0);
    m_pitchSpinBox = new QSpinBox(frame);
    m_pitchEditButton = new QPushButton(tr("edit"), frame);
    layout->addWidget(m_pitchSpinBox, 3, 1);
    layout->addWidget(m_pitchEditButton, 3, 2);

    connect(m_pitchSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPitchChanged(int)));
    connect(m_pitchEditButton, SIGNAL(released()),
            SLOT(slotEditPitch()));

    m_pitchSpinBox->setMinimum(MidiMinValue);
    m_pitchSpinBox->setMaximum(MidiMaxValue);

    m_controllerLabel = new QLabel(tr("Controller name:"), frame);
    m_controllerLabelValue = new QLabel(tr("<none>"), frame);
    m_controllerLabelValue->setAlignment( Qt::AlignRight );

    layout->addWidget(m_controllerLabel, 4, 0);
    layout->addWidget(m_controllerLabelValue, 4, 1);

    m_velocityLabel = new QLabel(tr("Velocity:"), frame);
    layout->addWidget(m_velocityLabel, 5, 0);
    m_velocitySpinBox = new QSpinBox(frame);
    layout->addWidget(m_velocitySpinBox, 5, 1);

    connect(m_velocitySpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotVelocityChanged(int)));

    m_velocitySpinBox->setMinimum(MidiMinValue);
    m_velocitySpinBox->setMaximum(MidiMaxValue);

    m_metaLabel = new QLabel(tr("Meta string:"), frame);
    layout->addWidget(m_metaLabel, 6, 0);
    m_metaEdit = new LineEdit(frame);
    layout->addWidget(m_metaEdit, 6, 1);

    m_sysexLoadButton = new QPushButton(tr("Load data"), frame);
    layout->addWidget(m_sysexLoadButton, 6, 2);
    m_sysexSaveButton = new QPushButton(tr("Save data"), frame);
    layout->addWidget(m_sysexSaveButton, 4, 2);

    frame->setLayout(layout);

    connect(m_metaEdit, SIGNAL(textChanged(const QString &)),
            SLOT(slotMetaChanged(const QString &)));
    connect(m_sysexLoadButton, SIGNAL(released()),
            SLOT(slotSysexLoad()));
    connect(m_sysexSaveButton, SIGNAL(released()),
            SLOT(slotSysexSave()));

    m_notationGroupBox = new QGroupBox( tr("Notation Properties"), vbox );
    m_notationGroupBox->setContentsMargins(5, 5, 5, 5);
    layout = new QGridLayout(m_notationGroupBox);
    layout->setSpacing(5);
    vboxLayout->addWidget(m_notationGroupBox);
    vbox->setLayout(vboxLayout);

    m_lockNotationValues = new QCheckBox(tr("Lock to changes in performed values"), m_notationGroupBox);
    layout->addWidget(m_lockNotationValues, 0, 0, 0- 0+1, 2-0+ 1);
    m_lockNotationValues->setChecked(true);

    connect(m_lockNotationValues, SIGNAL(released()),
            SLOT(slotLockNotationChanged()));

    m_notationTimeLabel = new QLabel(tr("Notation time:"), m_notationGroupBox);
    layout->addWidget(m_notationTimeLabel, 1, 0);
    m_notationTimeSpinBox = new QSpinBox(m_notationGroupBox);
    m_notationTimeSpinBox->setMinimum(INT_MIN);
    m_notationTimeSpinBox->setMaximum(INT_MAX);
    m_notationTimeSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_notationTimeEditButton = new QPushButton(tr("edit"), m_notationGroupBox);
    layout->addWidget(m_notationTimeSpinBox, 1, 1);
    layout->addWidget(m_notationTimeEditButton, 1, 2);

    connect(m_notationTimeSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotNotationAbsoluteTimeChanged(int)));
    connect(m_notationTimeEditButton, SIGNAL(released()),
            SLOT(slotEditNotationAbsoluteTime()));

    m_notationDurationLabel = new QLabel(tr("Notation duration:"), m_notationGroupBox);
    layout->addWidget(m_notationDurationLabel, 2, 0);
    m_notationDurationSpinBox = new QSpinBox(m_notationGroupBox);
    m_notationDurationSpinBox->setMinimum(0);
    m_notationDurationSpinBox->setMaximum(INT_MAX);
    m_notationDurationSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_notationDurationEditButton = new QPushButton(tr("edit"), m_notationGroupBox);
    layout->addWidget(m_notationDurationSpinBox, 2, 1);
    layout->addWidget(m_notationDurationEditButton, 2, 2);

    m_notationGroupBox->setLayout(layout);

    connect(m_notationDurationSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotNotationDurationChanged(int)));
    connect(m_notationDurationEditButton, SIGNAL(released()),
            SLOT(slotEditNotationDuration()));

    setupForEvent();
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
SimpleEventEditDialog::setupForEvent()
{
    using BaseProperties::PITCH;
    using BaseProperties::VELOCITY;

    if (m_typeCombo) {
        m_typeCombo->blockSignals(true);
    }
    m_timeSpinBox->blockSignals(true);
    m_notationTimeSpinBox->blockSignals(true);
    m_durationSpinBox->blockSignals(true);
    m_notationDurationSpinBox->blockSignals(true);
    m_pitchSpinBox->blockSignals(true);
    m_velocitySpinBox->blockSignals(true);
    m_metaEdit->blockSignals(true);

    m_pitchSpinBox->setMinimum(MidiMinValue);
    m_pitchSpinBox->setMaximum(MidiMaxValue);

    // Some common settings
    //
    m_durationLabel->setText(tr("Absolute time:"));
    m_timeLabel->show();
    m_timeSpinBox->show();
    m_timeEditButton->show();
    m_timeSpinBox->setValue(m_event.getAbsoluteTime());

    m_durationLabel->setText(tr("Duration:"));
    m_durationLabel->show();
    m_durationSpinBox->show();
    m_durationEditButton->show();
    m_durationSpinBox->setValue(m_event.getDuration());

    m_notationGroupBox->hide();

    // satisfy an ancient feature request:  if the performance and absolute
    // durations differ or the performance and absolute start times differ, then
    // this is a notation quantized note, and we set this checkbox off by
    // default, because "This is ok for ordinary notes but for a
    // notation-quantised note the note will likely already
    // have different performance and notation durations (and
    // probably different start times too) which makes the
    // lock inappropriate."
    m_lockNotationValues->setChecked(m_event.getDuration() == m_event.getNotationDuration() &&
                                     m_event.getAbsoluteTime() == m_event.getNotationAbsoluteTime());

    if (m_typeLabel)
        m_typeLabel->setText(strtoqstr(m_event.getType()));

    m_absoluteTime = m_event.getAbsoluteTime();
    m_notationAbsoluteTime = m_event.getNotationAbsoluteTime();
    m_duration = m_event.getDuration();
    m_notationDuration = m_event.getNotationDuration();

    m_sysexLoadButton->hide();
    m_sysexSaveButton->hide();

    if (m_type == Note::EventType) {
        m_notationGroupBox->show();
        m_notationTimeSpinBox->setValue(m_notationAbsoluteTime);
        m_notationDurationSpinBox->setValue(m_notationDuration);

        m_pitchLabel->show();
        m_pitchLabel->setText(tr("Note pitch:"));
        m_pitchSpinBox->show();
        m_pitchEditButton->show();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->show();
        m_velocityLabel->setText(tr("Note velocity:"));
        m_velocitySpinBox->show();

        m_metaLabel->hide();
        m_metaEdit->hide();

        try {
            m_pitchSpinBox->setValue(m_event.get<Int>(PITCH));
        } catch (Event::NoData) {
            m_pitchSpinBox->setValue(60);
        }

        try {
            m_velocitySpinBox->setValue(m_event.get<Int>(VELOCITY));
        } catch (Event::NoData) {
            m_velocitySpinBox->setValue(100);
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(0);

    } else if (m_type == Controller::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(tr("Controller number:"));
        m_pitchSpinBox->show();
        m_pitchEditButton->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();
        m_controllerLabel->setText(tr("Controller name:"));

        m_velocityLabel->show();
        m_velocityLabel->setText(tr("Controller value:"));
        m_velocitySpinBox->show();

        m_metaLabel->hide();
        m_metaEdit->hide();

        try {
            m_pitchSpinBox->setValue(m_event.get<Int>
                                     (Controller::NUMBER));
        } catch (Event::NoData) {
            m_pitchSpinBox->setValue(0);
        }

        try {
            m_velocitySpinBox->setValue(m_event.get<Int>
                                        (Controller::VALUE));
        } catch (Event::NoData) {
            m_velocitySpinBox->setValue(0);
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(1);

    } else if (m_type == KeyPressure::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(tr("Key pitch:"));
        m_pitchSpinBox->show();
        m_pitchEditButton->show();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->show();
        m_velocityLabel->setText(tr("Key pressure:"));
        m_velocitySpinBox->show();

        m_metaLabel->hide();
        m_metaEdit->hide();

        try {
            m_pitchSpinBox->setValue(m_event.get<Int>
                                     (KeyPressure::PITCH));
        } catch (Event::NoData) {
            m_pitchSpinBox->setValue(0);
        }

        try {
            m_velocitySpinBox->setValue(m_event.get<Int>
                                        (KeyPressure::PRESSURE));
        } catch (Event::NoData) {
            m_velocitySpinBox->setValue(0);
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(2);

    } else if (m_type == ChannelPressure::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(tr("Channel pressure:"));
        m_pitchSpinBox->show();
        m_pitchEditButton->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        try {
            m_pitchSpinBox->setValue(m_event.get<Int>
                                     (ChannelPressure::PRESSURE));
        } catch (Event::NoData) {
            m_pitchSpinBox->setValue(0);
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(3);

    } else if (m_type == ProgramChange::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchSpinBox->setMinimum(MidiMinValue + 1);
        m_pitchSpinBox->setMaximum(MidiMaxValue + 1);

        m_pitchLabel->show();
        m_pitchLabel->setText(tr("Program change:"));
        m_pitchSpinBox->show();
        m_pitchEditButton->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        try {
            m_pitchSpinBox->setValue(m_event.get<Int>
                                     (ProgramChange::PROGRAM) + 1);
        } catch (Event::NoData) {
            m_pitchSpinBox->setValue(0);
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(4);

    } else if (m_type == SystemExclusive::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();
        m_pitchEditButton->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->show();
        m_metaEdit->show();

        m_sysexLoadButton->show();
        m_sysexSaveButton->show();

        m_controllerLabel->setText(tr("Data length:"));
        m_metaLabel->setText(tr("Data:"));
        try {
            SystemExclusive sysEx(m_event);
            m_controllerLabelValue->setText(QString("%1").
                                            arg(sysEx.getRawData().length()));
            m_metaEdit->setText(strtoqstr(sysEx.getHexData()));
        } catch (...) {
            m_controllerLabelValue->setText("0");
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(5);

    } else if (m_type == PitchBend::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(tr("Pitchbend MSB:"));
        m_pitchSpinBox->show();
        m_pitchEditButton->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->show();
        m_velocityLabel->setText(tr("Pitchbend LSB:"));
        m_velocitySpinBox->show();

        m_metaLabel->hide();
        m_metaEdit->hide();

        try {
            m_pitchSpinBox->setValue(m_event.get<Int>
                                     (PitchBend::MSB));
        } catch (Event::NoData) {
            m_pitchSpinBox->setValue(0);
        }

        try {
            m_velocitySpinBox->setValue(m_event.get<Int>
                                        (PitchBend::LSB));
        } catch (Event::NoData) {
            m_velocitySpinBox->setValue(0);
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(6);

    } else if (m_type == Indication::EventType) {

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();
        m_pitchEditButton->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->show();
        m_metaEdit->show();
        m_metaLabel->setText(tr("Indication:"));

        try {
            Indication ind(m_event);
            m_metaEdit->setText(strtoqstr(ind.getIndicationType()));
            m_durationSpinBox->setValue(ind.getIndicationDuration());
        } catch (...) {
            m_metaEdit->setText(tr("<none>"));
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(7);

    } else if (m_type == Text::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();
        m_pitchEditButton->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->show();
        m_metaEdit->show();

        m_controllerLabel->setText(tr("Text type:"));
        m_metaLabel->setText(tr("Text:"));

        // get the text event
        try {
            Text text(m_event);
            m_controllerLabelValue->setText(strtoqstr(text.getTextType()));
            m_metaEdit->setText(strtoqstr(text.getText()));
        } catch (...) {
            m_controllerLabelValue->setText(tr("<none>"));
            m_metaEdit->setText(tr("<none>"));
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(8);

    } else if (m_type == Note::EventRestType) {

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();
        m_pitchEditButton->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(9);

    } else if (m_type == Clef::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();
        m_pitchEditButton->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();

        m_controllerLabel->setText(tr("Clef type:"));

        try {
            Clef clef(m_event);
            m_controllerLabelValue->setText(strtoqstr(clef.getClefType()));
        } catch (...) {
            m_controllerLabelValue->setText(tr("<none>"));
        }

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(10);

    } else if (m_type == ::Rosegarden::Key::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();
        m_pitchEditButton->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();

        m_controllerLabel->setText(tr("Key name:"));

        try {
            ::Rosegarden::Key key(m_event);
            m_controllerLabelValue->setText(strtoqstr(key.getName()));
        } catch (...) {
            m_controllerLabelValue->setText(tr("<none>"));
        }

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(11);

    } else if (m_type == Guitar::Chord::EventType) {

        m_durationLabel->hide();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();
        m_pitchEditButton->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        // m_controllerLabel->setText(tr("Text type:"));
        // m_metaLabel->setText(tr("Chord:"));

        // get the fingering event
        try {
            Guitar::Chord chord( m_event );
        } catch (...) {
            // m_controllerLabelValue->setText(tr("<none>"));
            // m_metaEdit->setText(tr("<none>"));
        }

        if (m_typeCombo)
            m_typeCombo->setCurrentIndex(12);

    } else {

        m_durationLabel->setText(tr("Unsupported event type:"));
        m_durationLabel->show();
        m_durationSpinBox->hide();
        m_durationEditButton->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();
        m_pitchEditButton->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->show();
        m_controllerLabelValue->setText(strtoqstr(m_type));

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        if (m_typeCombo)
            m_typeCombo->setEnabled(false);
    }

    if (m_typeCombo)
        m_typeCombo->blockSignals(false);
    m_timeSpinBox->blockSignals(false);
    m_notationTimeSpinBox->blockSignals(false);
    m_durationSpinBox->blockSignals(false);
    m_notationDurationSpinBox->blockSignals(false);
    m_pitchSpinBox->blockSignals(false);
    m_velocitySpinBox->blockSignals(false);
    m_metaEdit->blockSignals(false);

    slotLockNotationChanged();
}

Event
SimpleEventEditDialog::getEvent()
{
    bool useSeparateNotationValues =
        (m_event.getType() == Note::EventType);

    if (m_typeCombo) {

        int subordering = 0;
        if (m_type == Indication::EventType) {
            subordering = Indication::EventSubOrdering;
        } else if (m_type == Clef::EventType) {
            subordering = Clef::EventSubOrdering;
        } else if (m_type == ::Rosegarden::Key::EventType) {
            subordering = ::Rosegarden::Key::EventSubOrdering;
        } else if (m_type == Text::EventType) {
            subordering = Text::EventSubOrdering;
        } else if (m_type == Note::EventRestType) {
            subordering = Note::EventRestSubOrdering;
        } else if (m_type == PitchBend::EventType) {
            subordering = PitchBend::EventSubOrdering;
        } else if (m_type == Controller::EventType) {
            subordering = Controller::EventSubOrdering;
        } else if (m_type == KeyPressure::EventType) {
            subordering = KeyPressure::EventSubOrdering;
        } else if (m_type == ChannelPressure::EventType) {
            subordering = ChannelPressure::EventSubOrdering;
        } else if (m_type == ProgramChange::EventType) {
            subordering = ProgramChange::EventSubOrdering;
        } else if (m_type == SystemExclusive::EventType) {
            subordering = SystemExclusive::EventSubOrdering;
        }

        m_event = Event(m_type,
                        m_absoluteTime,
                        m_duration,
                        subordering,
                        (useSeparateNotationValues ?
                         m_notationAbsoluteTime : m_absoluteTime),
                        (useSeparateNotationValues ?
                         m_notationDuration : m_duration));

        // ensure these are set on m_event correctly
        slotPitchChanged(m_pitchSpinBox->value());
        slotVelocityChanged(m_velocitySpinBox->value());
    }

    Event event(m_event,
                m_absoluteTime,
                m_duration,
                m_event.getSubOrdering(),
                (useSeparateNotationValues ?
                 m_notationAbsoluteTime : m_absoluteTime),
                (useSeparateNotationValues ?
                 m_notationDuration : m_duration));

    // Values from the pitch and velocity spin boxes should already
    // have been set on m_event (and thus on event) by slotPitchChanged
    // and slotVelocityChanged.  Absolute time and duration were set in
    // the event ctor above; that just leaves the meta values.

    if (m_type == Indication::EventType) {

        event.set<String>(Indication::IndicationTypePropertyName,
                          qstrtostr(m_metaEdit->text()));

    } else if (m_type == Text::EventType) {

        event.set<String>(Text::TextTypePropertyName,
                          qstrtostr(m_controllerLabelValue->text()));
        event.set<String>(Text::TextPropertyName,
                          qstrtostr(m_metaEdit->text()));

    } else if (m_type == Clef::EventType) {

        event.set<String>(Clef::ClefPropertyName,
                          qstrtostr(m_controllerLabelValue->text()));

    } else if (m_type == ::Rosegarden::Key::EventType) {

        event.set<String>(::Rosegarden::Key::KeyPropertyName,
                          qstrtostr(m_controllerLabelValue->text()));

    } else if (m_type == SystemExclusive::EventType) {

        event.set<String>(SystemExclusive::DATABLOCK,
                          qstrtostr(m_metaEdit->text()));

    }

    return event;
}

void
SimpleEventEditDialog::slotEventTypeChanged(int value)
{
    m_type = qstrtostr(m_typeCombo->itemText(value));
    m_modified = true;

    if (m_type != m_event.getType())
        Event m_event(m_type, m_absoluteTime, m_duration);

    setupForEvent();

    // update whatever pitch and velocity correspond to
    if (!m_pitchSpinBox->isHidden())
        slotPitchChanged(m_pitchSpinBox->value());
    if (!m_velocitySpinBox->isHidden())
        slotVelocityChanged(m_velocitySpinBox->value());
}

void
SimpleEventEditDialog::slotAbsoluteTimeChanged(int value)
{
    m_absoluteTime = value;

    if (m_notationGroupBox->isHidden()) {
        m_notationAbsoluteTime = value;
    } else if (m_lockNotationValues->isChecked()) {
        m_notationAbsoluteTime = value;
        m_notationTimeSpinBox->setValue(value);
    }

    m_modified = true;
}

void
SimpleEventEditDialog::slotNotationAbsoluteTimeChanged(int value)
{
    m_notationAbsoluteTime = value;
    m_modified = true;
}

void
SimpleEventEditDialog::slotDurationChanged(int value)
{
    m_duration = value;

    if (m_notationGroupBox->isHidden()) {
        m_notationDuration = value;
    } else if (m_lockNotationValues->isChecked()) {
        m_notationDuration = value;
        m_notationDurationSpinBox->setValue(value);
    }

    m_modified = true;
}

void
SimpleEventEditDialog::slotNotationDurationChanged(int value)
{
    m_notationDuration = value;
    m_modified = true;
}

void
SimpleEventEditDialog::slotPitchChanged(int value)
{
    m_modified = true;

    if (m_type == Note::EventType) {
        m_event.set<Int>(BaseProperties::PITCH, value);

    } else if (m_type == Controller::EventType) {
        m_event.set<Int>(Controller::NUMBER, value);

    } else if (m_type == KeyPressure::EventType) {
        m_event.set<Int>(KeyPressure::PITCH, value);

    } else if (m_type == ChannelPressure::EventType) {
        m_event.set<Int>(ChannelPressure::PRESSURE, value);

    } else if (m_type == ProgramChange::EventType) {
        if (value < 1)
            value = 1;
        m_event.set<Int>(ProgramChange::PROGRAM, value - 1);

    } else if (m_type == PitchBend::EventType) {
        m_event.set<Int>(PitchBend::MSB, value);
    }
    //!!!??? sysex?
}

void
SimpleEventEditDialog::slotVelocityChanged(int value)
{
    m_modified = true;

    if (m_type == Note::EventType) {
        m_event.set<Int>(BaseProperties::VELOCITY, value);

    } else if (m_type == Controller::EventType) {
        m_event.set<Int>(Controller::VALUE, value);

    } else if (m_type == KeyPressure::EventType) {
        m_event.set<Int>(KeyPressure::PRESSURE, value);

    } else if (m_type == PitchBend::EventType) {
        m_event.set<Int>(PitchBend::LSB, value);
    }
}

void
SimpleEventEditDialog::slotMetaChanged(const QString &)
{
    m_modified = true;
}

void
SimpleEventEditDialog::slotLockNotationChanged()
{
    bool enable = !m_lockNotationValues->isChecked();
    m_notationTimeSpinBox->setEnabled(enable);
    m_notationTimeEditButton->setEnabled(enable);
    m_notationDurationSpinBox->setEnabled(enable);
    m_notationDurationEditButton->setEnabled(enable);
}

void
SimpleEventEditDialog::slotEditAbsoluteTime()
{
    TimeDialog dialog(this, tr("Edit Event Time"),
                      &m_doc->getComposition(),
                      m_timeSpinBox->value(),
                      true);
    if (dialog.exec() == QDialog::Accepted) {
        m_timeSpinBox->setValue(dialog.getTime());
    }
}

void
SimpleEventEditDialog::slotEditNotationAbsoluteTime()
{
    TimeDialog dialog(this, tr("Edit Event Notation Time"),
                      &m_doc->getComposition(),
                      m_notationTimeSpinBox->value(),
                      true);
    if (dialog.exec() == QDialog::Accepted) {
        m_notationTimeSpinBox->setValue(dialog.getTime());
    }
}

void
SimpleEventEditDialog::slotEditDuration()
{
    TimeDialog dialog(this, tr("Edit Duration"),
                      &m_doc->getComposition(),
                      m_timeSpinBox->value(),
                      m_durationSpinBox->value(),
                      1,
                      true);
    if (dialog.exec() == QDialog::Accepted) {
        m_durationSpinBox->setValue(dialog.getTime());
    }
}

void
SimpleEventEditDialog::slotEditNotationDuration()
{
    TimeDialog dialog(this, tr("Edit Notation Duration"),
                      &m_doc->getComposition(),
                      m_notationTimeSpinBox->value(),
                      m_notationDurationSpinBox->value(),
                      1,
                      true);
    if (dialog.exec() == QDialog::Accepted) {
        m_notationDurationSpinBox->setValue(dialog.getTime());
    }
}

void
SimpleEventEditDialog::slotEditPitch()
{
    PitchDialog dialog(this, tr("Edit Pitch"), m_pitchSpinBox->value());
    if (dialog.exec() == QDialog::Accepted) {
        m_pitchSpinBox->setValue(dialog.getPitch());
    }
}

void
SimpleEventEditDialog::slotSysexLoad()
{
    QString path = FileDialog::getOpenFileName( this, tr("Load System Exclusive data in File"), QDir::currentPath(),
                   tr("System exclusive files") + " (*.syx *.SYX)" + ";;" +
                   tr("All files") + " (*)"); //":SYSTEMEXCLUSIVE",

    if (path.isNull())
        return ;

    QFile file(path);
    file.open(QIODevice::ReadOnly);
    std::string s;
    char c;
    bool ioOk;

    while ((ioOk = file.getChar(&c))) {
        if (c == static_cast<char>(0xf0)) break;
    }
    while (ioOk) {
        s += c;
        if (c == static_cast<char>(0xf7))
            break;
        ioOk = file.getChar(&c);
    }
    file.close();
    m_metaEdit->setText(strtoqstr(SystemExclusive::toHex(s)));
}

void
SimpleEventEditDialog::slotSysexSave()
{
    QString path = FileDialog::getSaveFileName( this, //":SYSTEMEXCLUSIVE",
			tr("Save System Exclusive data to..."),
					tr("*.syx|System exclusive files (*.syx)")
					);
    if (path.isNull())
        return ;

    QFile file(path);
    file.open(QIODevice::WriteOnly);
    SystemExclusive sysEx(m_event);
    file.write(sysEx.getRawData().c_str(),
               static_cast<qint64>(sysEx.getRawData().length()));
    file.close();
}

}
