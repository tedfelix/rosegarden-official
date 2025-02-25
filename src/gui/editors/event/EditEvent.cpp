/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[EditEvent]"

#include "EditEvent.h"

#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/guitar/Chord.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"  // for LastUsedPathsConfigGroup
#include "gui/dialogs/PitchDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/FileDialog.h"
#include "sound/Midi.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFile>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QLayout>
#include <QSettings>


namespace Rosegarden
{


#if 0
        // ??? Why not just load these into a combo box?  Then there will be
        //     no need for update trickery.
        // ??? This whole dialog is pretty cumbersome.  Reusing each control
        //     for various things is confusing.  Might want to consider using
        //     a QStackedLayout of 13 small dialogs, one for each event type.
        //     See if they can be reused by EventEditDialog.
        //     Maybe consider combining the two using an "Advanced" button to
        //     display the "advanced" stuff.
        // ??? We should also show these on the ELE.
        // ??? We should also consider using the controller list that is in the
        //     Device.  That is probably more accurate in some cases and should
        //     have higher priority than these names.  Maybe bold-face the names
        //     we get from the Device to indicate "official".
        // ??? Move this to a more central location.
        static std::map<int, QString> controllerNames = {
                { 0, "Bank Select MSB" },
                { 1, "Mod Wheel MSB" },
                { 2, "Breath Controller MSB" },
                { 4, "Foot Pedal MSB" },
                { 5, "Portamento Time MSB" },
                { 6, "Data Entry MSB" },
                { 7, "Volume MSB" },
                { 8, "Stereo Balance MSB" },
                { 10, "Pan MSB" },
                { 11, "Expression MSB" },
                { 12, "Effect 1 MSB" },
                { 13, "Effect 2 MSB" },
                { 32, "Bank Select LSB" },
                { 33, "Mod Wheel LSB" },
                { 34, "Breath Controller LSB" },
                { 36, "Foot Pedal LSB" },
                { 37, "Portamento Time LSB" },
                { 38, "Data Entry LSB" },
                { 39, "Volume LSB" },
                { 40, "Stereo Balance LSB" },
                { 42, "Pan LSB" },
                { 43, "Expression LSB" },
                { 44, "Effect 1 LSB" },
                { 45, "Effect 2 LSB" },
                { 64, "Sustain" },
                { 65, "Portamento On/Off" },
                { 66, "Sostenuto" },
                { 67, "Soft Pedal" },
                { 68, "Legato" },
                { 69, "Hold Pedal 2" },
                { 91, "Reverb" },
                { 92, "Tremolo" },
                { 93, "Chorus" },
                { 94, "Detuning" },
                { 95, "Phaser" },
                { 96, "Data +" },
                { 97, "Data -" },
                { 98, "NRPN LSB" },
                { 99, "NRPN MSB" },
                { 100, "RPN LSB" },
                { 101, "RPN MSB" },
                { 120, "Channel Mute" },
                { 121, "Reset All Controllers" },
                { 122, "Local On/Off" },
                { 123, "All MIDI Notes Off" },
                { 124, "Omni Off" },
                { 125, "Omni On" },
                { 126, "Mono On/Off" },
                { 127, "Poly On/Off" }
            };
#endif


EditEvent::EditEvent(
        QWidget *parent,
        RosegardenDocument *doc,
        const Event &event,
        bool inserting) :
    QDialog(parent),
    m_doc(doc),
    m_event(event),
    m_type(event.getType()),
    m_absoluteTime(event.getAbsoluteTime()),
    m_duration(event.getDuration())
{
    setModal(true);
    setWindowTitle(inserting ? tr("Insert Event").toStdString().c_str() :
                               tr("Edit Event").toStdString().c_str());

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout(vbox);
    metagrid->addWidget(vbox, 0, 0);


    // Event Properties Group

    QGroupBox *frame = new QGroupBox( tr("Event Properties"), vbox );
    frame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);
    vboxLayout->addWidget(frame);

    // Event type
    layout->addWidget(new QLabel(tr("Event type:"), frame), 0, 0);

    // If the user is inserting a new Event, provide them with a
    // combo box for selecting the event type.
    if (inserting) {

        m_typeLabel = nullptr;

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
        connect(m_typeCombo,
                    static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                this, &EditEvent::slotEventTypeChanged);

    } else {  // Display event type read-only.

        m_typeCombo = nullptr;

        m_typeLabel = new QLabel(frame);
        layout->addWidget(m_typeLabel, 0, 1);
    }

    // Absolute time
    m_timeLabel = new QLabel(tr("Absolute time:"), frame);
    layout->addWidget(m_timeLabel, 1, 0);
    m_timeSpinBox = new QSpinBox(frame);
    m_timeSpinBox->setMinimum(INT_MIN);
    m_timeSpinBox->setMaximum(INT_MAX);
    m_timeSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_timeEditButton = new QPushButton(tr("edit"), frame);
    layout->addWidget(m_timeSpinBox, 1, 1);
    layout->addWidget(m_timeEditButton, 1, 2);

    connect(m_timeSpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotAbsoluteTimeChanged);
    connect(m_timeEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditAbsoluteTime);

    // Duration
    m_durationLabel = new QLabel(tr("Duration:"), frame);
    layout->addWidget(m_durationLabel, 2, 0);
    m_durationSpinBox = new QSpinBox(frame);
    m_durationSpinBox->setMinimum(0);
    m_durationSpinBox->setMaximum(INT_MAX);
    m_durationSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_durationEditButton = new QPushButton(tr("edit"), frame);
    layout->addWidget(m_durationSpinBox, 2, 1);
    layout->addWidget(m_durationEditButton, 2, 2);

    connect(m_durationSpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotDurationChanged);
    connect(m_durationEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditDuration);

#if 0
    // Pitch
    m_pitchLabel = new QLabel(tr("Pitch:"), frame);
    layout->addWidget(m_pitchLabel, 3, 0);
    m_pitchSpinBox = new QSpinBox(frame);
    m_pitchEditButton = new QPushButton(tr("edit"), frame);
    layout->addWidget(m_pitchSpinBox, 3, 1);
    layout->addWidget(m_pitchEditButton, 3, 2);

    connect(m_pitchSpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotPitchChanged);
    connect(m_pitchEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditPitch);

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

    connect(m_velocitySpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotVelocityChanged);

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

    connect(m_metaEdit, &QLineEdit::textChanged,
            this, &EditEvent::slotMetaChanged);
    connect(m_sysexLoadButton, &QAbstractButton::released,
            this, &EditEvent::slotSysexLoad);
    connect(m_sysexSaveButton, &QAbstractButton::released,
            this, &EditEvent::slotSysexSave);


    // Notation Properties Group

    m_notationGroupBox = new QGroupBox( tr("Notation Properties"), vbox );
    m_notationGroupBox->setContentsMargins(5, 5, 5, 5);
    layout = new QGridLayout(m_notationGroupBox);
    layout->setSpacing(5);
    vboxLayout->addWidget(m_notationGroupBox);

    m_lockNotationValues = new QCheckBox(tr("Lock to changes in performed values"), m_notationGroupBox);
    layout->addWidget(m_lockNotationValues, 0, 0, 0- 0+1, 2-0+ 1);
    m_lockNotationValues->setChecked(true);

    connect(m_lockNotationValues, &QAbstractButton::released,
            this, &EditEvent::slotLockNotationChanged);

    m_notationTimeLabel = new QLabel(tr("Notation time:"), m_notationGroupBox);
    layout->addWidget(m_notationTimeLabel, 1, 0);
    m_notationTimeSpinBox = new QSpinBox(m_notationGroupBox);
    m_notationTimeSpinBox->setMinimum(INT_MIN);
    m_notationTimeSpinBox->setMaximum(INT_MAX);
    m_notationTimeSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_notationTimeEditButton = new QPushButton(tr("edit"), m_notationGroupBox);
    layout->addWidget(m_notationTimeSpinBox, 1, 1);
    layout->addWidget(m_notationTimeEditButton, 1, 2);

    connect(m_notationTimeSpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotNotationAbsoluteTimeChanged);
    connect(m_notationTimeEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditNotationAbsoluteTime);

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

    connect(m_notationDurationSpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotNotationDurationChanged);
    connect(m_notationDurationEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditNotationDuration);

#endif

    setupForEvent();

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void
EditEvent::setupForEvent()
{
    // Block Signals

    // ??? Get rid of these.  Use the signal that doesn't fire on
    //     non-user input.
    if (m_typeCombo) {
        m_typeCombo->blockSignals(true);
    }
    m_timeSpinBox->blockSignals(true);
    m_durationSpinBox->blockSignals(true);
#if 0
    m_notationTimeSpinBox->blockSignals(true);
    m_notationDurationSpinBox->blockSignals(true);
    m_pitchSpinBox->blockSignals(true);
    m_velocitySpinBox->blockSignals(true);
    m_metaEdit->blockSignals(true);

    m_pitchSpinBox->setMinimum(MidiMinValue);
    m_pitchSpinBox->setMaximum(MidiMaxValue);
#endif


    // Common parameters

    // Event type
    if (m_typeLabel)
        m_typeLabel->setText(strtoqstr(m_event.getType()));

    // Absolute time
    m_durationLabel->setText(tr("Absolute time:"));
    m_timeLabel->show();
    m_timeSpinBox->show();
    m_timeEditButton->show();
    m_timeSpinBox->setValue(m_event.getAbsoluteTime());
    m_absoluteTime = m_event.getAbsoluteTime();

    // Duration
    m_durationLabel->setText(tr("Duration:"));
    m_durationLabel->show();
    m_durationSpinBox->show();
    m_durationEditButton->show();
    m_durationSpinBox->setValue(m_event.getDuration());
    m_duration = m_event.getDuration();

#if 0
    // Notation Group
    m_notationGroupBox->hide();
    m_lockNotationValues->setChecked(
            m_event.getDuration() == m_event.getNotationDuration()  &&
            m_event.getAbsoluteTime() == m_event.getNotationAbsoluteTime());
    m_notationAbsoluteTime = m_event.getNotationAbsoluteTime();
    m_notationDuration = m_event.getNotationDuration();

    // Sysex
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
            m_pitchSpinBox->setValue(m_event.get<Int>(BaseProperties::PITCH));
        } catch (const Event::NoData &) {
            m_pitchSpinBox->setValue(60);
        }

        try {
            m_velocitySpinBox->setValue(m_event.get<Int>(BaseProperties::VELOCITY));
        } catch (const Event::NoData &) {
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

        int controllerNumber{0};

        if (m_event.has(Controller::NUMBER))
            controllerNumber = m_event.get<Int>(Controller::NUMBER);

        m_pitchSpinBox->setValue(controllerNumber);

#if 0
        // ??? Ok, but this needs to change dynamically as the user changes
        //     the controller number.
        if (controllerNames.find(controllerNumber) != controllerNames.end())
            m_controllerLabelValue->setText(controllerNames[controllerNumber]);
        else
            m_controllerLabelValue->setText(tr("<none>"));
#endif

        try {
            m_velocitySpinBox->setValue(m_event.get<Int>
                                        (Controller::VALUE));
        } catch (const Event::NoData &) {
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
        } catch (const Event::NoData &) {
            m_pitchSpinBox->setValue(0);
        }

        try {
            m_velocitySpinBox->setValue(m_event.get<Int>
                                        (KeyPressure::PRESSURE));
        } catch (const Event::NoData &) {
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
        } catch (const Event::NoData &) {
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
        } catch (const Event::NoData &) {
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
            std::string datablock;
            m_event.get<String>(SystemExclusive::DATABLOCK, datablock);
            m_controllerLabelValue->setText(QString("%1").
                    arg(SystemExclusive::toRaw(datablock).length()));
            m_metaEdit->setText(strtoqstr(datablock));
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
        } catch (const Event::NoData &) {
            m_pitchSpinBox->setValue(0);
        }

        try {
            m_velocitySpinBox->setValue(m_event.get<Int>
                                        (PitchBend::LSB));
        } catch (const Event::NoData &) {
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
#endif


    // Unblock signals.

    if (m_typeCombo)
        m_typeCombo->blockSignals(false);
    m_timeSpinBox->blockSignals(false);
    m_durationSpinBox->blockSignals(false);

#if 0
    m_notationTimeSpinBox->blockSignals(false);
    m_notationDurationSpinBox->blockSignals(false);
    m_pitchSpinBox->blockSignals(false);
    m_velocitySpinBox->blockSignals(false);
    m_metaEdit->blockSignals(false);
#endif

    slotLockNotationChanged();
}

Event
EditEvent::getEvent()
{
    bool useSeparateNotationValues =
        (m_event.getType() == Note::EventType);

    // If we are inserting a new Event...
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

        m_event = Event(
                m_type,
                m_absoluteTime,
                m_duration,
                subordering,
                useSeparateNotationValues ?
                        m_notationAbsoluteTime : m_absoluteTime,
                useSeparateNotationValues ? m_notationDuration : m_duration);

#if 0
        // ensure these are set on m_event correctly
        slotPitchChanged(m_pitchSpinBox->value());
        slotVelocityChanged(m_velocitySpinBox->value());
#endif
    }

    Event event(
            m_event,
            m_absoluteTime,
            m_duration,
            m_event.getSubOrdering(),
            useSeparateNotationValues ? m_notationAbsoluteTime : m_absoluteTime,
            useSeparateNotationValues ? m_notationDuration : m_duration);

    // Values from the pitch and velocity spin boxes should already
    // have been set on m_event (and thus on event) by slotPitchChanged
    // and slotVelocityChanged.  Absolute time and duration were set in
    // the event ctor above; that just leaves the meta values.

#if 0
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
#endif

    return event;
}

void
EditEvent::slotEventTypeChanged(int value)
{
    m_type = qstrtostr(m_typeCombo->itemText(value));
    m_modified = true;

    if (m_type != m_event.getType())
        Event m_event(m_type, m_absoluteTime, m_duration);

    setupForEvent();

#if 0
    // update whatever pitch and velocity correspond to
    if (!m_pitchSpinBox->isHidden())
        slotPitchChanged(m_pitchSpinBox->value());
    if (!m_velocitySpinBox->isHidden())
        slotVelocityChanged(m_velocitySpinBox->value());
#endif
}

void
EditEvent::slotAbsoluteTimeChanged(int value)
{
    m_absoluteTime = value;

#if 0
    if (m_notationGroupBox->isHidden()) {
        m_notationAbsoluteTime = value;
    } else if (m_lockNotationValues->isChecked()) {
        m_notationAbsoluteTime = value;
        m_notationTimeSpinBox->setValue(value);
    }
#endif

    m_modified = true;
}

void
EditEvent::slotNotationAbsoluteTimeChanged(int value)
{
    m_notationAbsoluteTime = value;
    m_modified = true;
}

void
EditEvent::slotDurationChanged(int value)
{
    m_duration = value;

#if 0
    if (m_notationGroupBox->isHidden()) {
        m_notationDuration = value;
    } else if (m_lockNotationValues->isChecked()) {
        m_notationDuration = value;
        m_notationDurationSpinBox->setValue(value);
    }
#endif

    m_modified = true;
}

void
EditEvent::slotNotationDurationChanged(int value)
{
    m_notationDuration = value;
    m_modified = true;
}

void
EditEvent::slotPitchChanged(int value)
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
    // !!! sysex?
}

void
EditEvent::slotVelocityChanged(int value)
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
EditEvent::slotMetaChanged(const QString &)
{
    m_modified = true;
}

void
EditEvent::slotLockNotationChanged()
{
    // Enable/disable notation fields as appropriate.
#if 0
    const bool enable = !m_lockNotationValues->isChecked();

    m_notationTimeSpinBox->setEnabled(enable);
    m_notationTimeEditButton->setEnabled(enable);
    m_notationDurationSpinBox->setEnabled(enable);
    m_notationDurationEditButton->setEnabled(enable);
#endif
}

void
EditEvent::slotEditAbsoluteTime()
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
EditEvent::slotEditNotationAbsoluteTime()
{
#if 0
    TimeDialog dialog(this, tr("Edit Event Notation Time"),
                      &m_doc->getComposition(),
                      m_notationTimeSpinBox->value(),
                      true);
    if (dialog.exec() == QDialog::Accepted) {
        m_notationTimeSpinBox->setValue(dialog.getTime());
    }
#endif
}

void
EditEvent::slotEditDuration()
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
EditEvent::slotEditNotationDuration()
{
#if 0
    TimeDialog dialog(this, tr("Edit Notation Duration"),
                      &m_doc->getComposition(),
                      m_notationTimeSpinBox->value(),
                      m_notationDurationSpinBox->value(),
                      1,
                      true);
    if (dialog.exec() == QDialog::Accepted) {
        m_notationDurationSpinBox->setValue(dialog.getTime());
    }
#endif
}

void
EditEvent::slotEditPitch()
{
#if 0
    PitchDialog dialog(this, tr("Edit Pitch"), m_pitchSpinBox->value());
    if (dialog.exec() == QDialog::Accepted) {
        m_pitchSpinBox->setValue(dialog.getPitch());
    }
#endif
}

void
EditEvent::slotSysexLoad()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    const QString pathKey = "load_sysex";
    QString directory = settings.value(pathKey, QDir::homePath()).toString();

    QString name = FileDialog::getOpenFileName(
            this,  // parent
            tr("Load System Exclusive data in File"),  // caption
            directory,  // dir
            tr("System exclusive files") + " (*.syx *.SYX)" + ";;" +
                tr("All files") + " (*)");  // filter

    if (name.isNull())
        return ;

    QFile file(name);
    file.open(QIODevice::ReadOnly);
    std::string s;
    char c;

    // Discard leading bytes up to and including the first SysEx Start (F0).
    while (file.getChar(&c)) {
        if (c == static_cast<char>(MIDI_SYSTEM_EXCLUSIVE))
            break;
    }
    // Copy up to but not including SysEx End (F7).
    while (file.getChar(&c)) {
        if (c == static_cast<char>(MIDI_END_OF_EXCLUSIVE))
            break;
        s += c;
    }

    file.close();

    if (s.empty())
        QMessageBox::critical(this, tr("Rosegarden"), tr("Could not load SysEx file."));

#if 0
    m_metaEdit->setText(strtoqstr(SystemExclusive::toHex(s)));
#endif

    // Write the directory to the settings
    QDir d = QFileInfo(name).dir();
    directory = d.canonicalPath();
    settings.setValue(pathKey, directory);
    settings.endGroup();
}

void
EditEvent::slotSysexSave()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    const QString pathKey = "save_sysex";
    QString directory = settings.value(pathKey, QDir::homePath()).toString();

    QString name = FileDialog::getSaveFileName(
            this,  // parent
            tr("Save System Exclusive data to..."),  // caption
            directory,  // dir
            "",  // defaultName
            tr("System exclusive files") + " (*.syx *.SYX)" + ";;" +
                tr("All files") + " (*)");  // filter
            //tr("*.syx|System exclusive files (*.syx)"));  // filter
    if (name.isNull())
        return;

    QFile file(name);
    file.open(QIODevice::WriteOnly);

    std::string datablock;
    m_event.get<String>(SystemExclusive::DATABLOCK, datablock);
    // Add SysEx and End SysEx status bytes.
    datablock = "F0 " + datablock + " F7";
    datablock = SystemExclusive::toRaw(datablock);
    file.write(datablock.c_str(),
               static_cast<qint64>(datablock.length()));

    file.close();

    // Write the directory to the settings
    QDir d = QFileInfo(name).dir();
    directory = d.canonicalPath();
    settings.setValue(pathKey, directory);
    settings.endGroup();
}


}
