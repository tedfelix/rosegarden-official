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

#define RG_MODULE_STRING "[EditEvent]"
#define RG_NO_DEBUG_PRINT

#include "EditEvent.h"

#include "EventWidget.h"

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
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTableWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QLayout>
#include <QSettings>


namespace Rosegarden
{


// ??? This seems really reusable, however, it also seems like these
//     sub orderings aren't actually used where they need to be used.
//     E.g. a search on Controller::EventSubOrdering turns up far fewer
//     places than expected.  Shouldn't it be used whenever a
//     controller Event is created?  Shouldn't this "if" be in Event's
//     ctors?
static int getSubOrdering(std::string eventType)
{
    if (eventType == Indication::EventType) {
        return Indication::EventSubOrdering;
    } else if (eventType == Clef::EventType) {
        return Clef::EventSubOrdering;
    } else if (eventType == ::Rosegarden::Key::EventType) {
        return ::Rosegarden::Key::EventSubOrdering;
    } else if (eventType == Text::EventType) {
        return Text::EventSubOrdering;
    } else if (eventType == Note::EventRestType) {
        return Note::EventRestSubOrdering;
    } else if (eventType == PitchBend::EventType) {
        return PitchBend::EventSubOrdering;
    } else if (eventType == Controller::EventType) {
        return Controller::EventSubOrdering;
    } else if (eventType == KeyPressure::EventType) {
        return KeyPressure::EventSubOrdering;
    } else if (eventType == ChannelPressure::EventType) {
        return ChannelPressure::EventSubOrdering;
    } else if (eventType == ProgramChange::EventType) {
        return ProgramChange::EventSubOrdering;
    } else if (eventType == SystemExclusive::EventType) {
        return SystemExclusive::EventSubOrdering;
    }

    return 0;
}

#if 0
        // ??? Move this to ControllerWidget when it exists.  Then promote it
        //     up someplace more common so ELE and others can use it.
        // ??? Why not just load these into a combo box?  Then there will be
        //     no need for update trickery.
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


EditEvent::EditEvent(QWidget *parent, const Event &event, bool inserting) :
    QDialog(parent),
    m_event(event)
{
    setModal(true);
    setWindowTitle(inserting ? tr("Insert Event").toStdString().c_str() :
                               tr("Edit Event").toStdString().c_str());

    QVBoxLayout *mainLayout = new QVBoxLayout(this);


    // Properties Group

    QGroupBox *propertiesGroup = new QGroupBox(tr("Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);
    mainLayout->addWidget(propertiesGroup);

    int row{0};

    // Event type
    propertiesLayout->addWidget(
            new QLabel(tr("Event type:"), propertiesGroup), row, 0);

    // If the user is inserting a new Event, provide them with a
    // combo box for selecting the event type.
    if (inserting) {

        m_typeCombo = new QComboBox(propertiesGroup);
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
        propertiesLayout->addWidget(m_typeCombo, row, 1);

    } else {  // Display event type read-only.

        m_typeLabel = new QLabel(propertiesGroup);
        propertiesLayout->addWidget(m_typeLabel, row, 1);

    }

    ++row;

    // Absolute time
    m_timeLabel = new QLabel(tr("Absolute time:"), propertiesGroup);
    propertiesLayout->addWidget(m_timeLabel, row, 0);
    m_timeSpinBox = new QSpinBox(propertiesGroup);
    m_timeSpinBox->setMinimum(INT_MIN);
    m_timeSpinBox->setMaximum(INT_MAX);
    m_timeSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    propertiesLayout->addWidget(m_timeSpinBox, row, 1);

    m_timeEditButton = new QPushButton(tr("edit"), propertiesGroup);
    connect(m_timeEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditAbsoluteTime);
    propertiesLayout->addWidget(m_timeEditButton, row, 2);

    // Event Widget
    if (inserting) {
        // For insert mode we need a QWidget with a QStackedLayout and each
        // of the event widgets loaded into that.
//        m_eventWidgetStack = EventWidgetStack::create(this);
//        mainLayout->addWidget(m_eventWidgetStack);
    } else {  // editing
        // For edit mode we only need the Event widget for the current
        // event type.
        // ??? Widget is not appearing because the window is too small.
        //     Also the Widget is compressed and not stretching to fill
        //     the space horizontally.  Probably has to do with the fact
        //     that it is its own widget.
        //     How do we create a QWidget that plays by the rules?
        //     1. Be sure to set a minimum size or else the widget will
        //        not take up any space at all.
        m_eventWidget = EventWidget::create(this, event);
        mainLayout->addWidget(m_eventWidget);
    }


    // Advanced Properties Group

    QGroupBox *advancedPropertiesGroup =
            new QGroupBox(tr("Advanced Properties"), this);
    advancedPropertiesGroup->setContentsMargins(5, 5, 5, 5);

    QGridLayout *advancedPropertiesLayout =
            new QGridLayout(advancedPropertiesGroup);
    advancedPropertiesLayout->setSpacing(5);
    mainLayout->addWidget(advancedPropertiesGroup);

    row = 0;

    // Sub Ordering
    advancedPropertiesLayout->addWidget(
            new QLabel(tr("Sub-ordering: ")), row, 0);

    m_subOrdering = new QSpinBox(advancedPropertiesGroup);
    m_subOrdering->setRange(-100, 100);
    m_subOrdering->setSingleStep(1);
    // ??? Is this ok for insert mode?
    m_subOrdering->setValue(event.getSubOrdering());
    advancedPropertiesLayout->addWidget(m_subOrdering, row, 1);

    ++row;

    // Property table.

    m_propertyTable = new QTableWidget(advancedPropertiesGroup);
    // Copy properties from m_event to the table.
    updatePropertyTable();
    advancedPropertiesLayout->addWidget(m_propertyTable, row, 0, 1, 2);

#if 0
    // Pitch
    m_pitchLabel = new QLabel(tr("Pitch:"), propertiesGroup);
    propertiesLayout->addWidget(m_pitchLabel, 3, 0);
    m_pitchSpinBox = new QSpinBox(propertiesGroup);
    m_pitchEditButton = new QPushButton(tr("edit"), propertiesGroup);
    propertiesLayout->addWidget(m_pitchSpinBox, 3, 1);
    propertiesLayout->addWidget(m_pitchEditButton, 3, 2);

    connect(m_pitchSpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotPitchChanged);
    connect(m_pitchEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditPitch);

    m_pitchSpinBox->setMinimum(MidiMinValue);
    m_pitchSpinBox->setMaximum(MidiMaxValue);

    m_controllerLabel = new QLabel(tr("Controller name:"), propertiesGroup);
    m_controllerLabelValue = new QLabel(tr("<none>"), propertiesGroup);
    m_controllerLabelValue->setAlignment( Qt::AlignRight );

    propertiesLayout->addWidget(m_controllerLabel, 4, 0);
    propertiesLayout->addWidget(m_controllerLabelValue, 4, 1);

    m_velocityLabel = new QLabel(tr("Velocity:"), propertiesGroup);
    propertiesLayout->addWidget(m_velocityLabel, 5, 0);
    m_velocitySpinBox = new QSpinBox(propertiesGroup);
    propertiesLayout->addWidget(m_velocitySpinBox, 5, 1);

    connect(m_velocitySpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotVelocityChanged);

    m_velocitySpinBox->setMinimum(MidiMinValue);
    m_velocitySpinBox->setMaximum(MidiMaxValue);

    m_metaLabel = new QLabel(tr("Meta string:"), propertiesGroup);
    propertiesLayout->addWidget(m_metaLabel, 6, 0);
    m_metaEdit = new LineEdit(propertiesGroup);
    propertiesLayout->addWidget(m_metaEdit, 6, 1);

    m_sysexLoadButton = new QPushButton(tr("Load data"), propertiesGroup);
    propertiesLayout->addWidget(m_sysexLoadButton, 6, 2);
    m_sysexSaveButton = new QPushButton(tr("Save data"), propertiesGroup);
    propertiesLayout->addWidget(m_sysexSaveButton, 4, 2);

    propertiesGroup->setLayout(layout);

    connect(m_metaEdit, &QLineEdit::textChanged,
            this, &EditEvent::slotMetaChanged);
    connect(m_sysexLoadButton, &QAbstractButton::released,
            this, &EditEvent::slotSysexLoad);
    connect(m_sysexSaveButton, &QAbstractButton::released,
            this, &EditEvent::slotSysexSave);


    // Notation Properties Group

    m_notationGroupBox = new QGroupBox( tr("Notation Properties"), vbox );
    m_notationGroupBox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *npLayout = new QGridLayout(m_notationGroupBox);
    npLayout->setSpacing(5);
    mainLayout->addWidget(m_notationGroupBox);

    m_lockNotationValues = new QCheckBox(tr("Lock to changes in performed values"), m_notationGroupBox);
    npLayout->addWidget(m_lockNotationValues, 0, 0, 0- 0+1, 2-0+ 1);
    m_lockNotationValues->setChecked(true);

    connect(m_lockNotationValues, &QAbstractButton::released,
            this, &EditEvent::slotLockNotationChanged);

    m_notationTimeLabel = new QLabel(tr("Notation time:"), m_notationGroupBox);
    npLayout->addWidget(m_notationTimeLabel, 1, 0);
    m_notationTimeSpinBox = new QSpinBox(m_notationGroupBox);
    m_notationTimeSpinBox->setMinimum(INT_MIN);
    m_notationTimeSpinBox->setMaximum(INT_MAX);
    m_notationTimeSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_notationTimeEditButton = new QPushButton(tr("edit"), m_notationGroupBox);
    npLayout->addWidget(m_notationTimeSpinBox, 1, 1);
    npLayout->addWidget(m_notationTimeEditButton, 1, 2);

    connect(m_notationTimeSpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotNotationAbsoluteTimeChanged);
    connect(m_notationTimeEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditNotationAbsoluteTime);

    m_notationDurationLabel = new QLabel(tr("Notation duration:"), m_notationGroupBox);
    npLayout->addWidget(m_notationDurationLabel, 2, 0);
    m_notationDurationSpinBox = new QSpinBox(m_notationGroupBox);
    m_notationDurationSpinBox->setMinimum(0);
    m_notationDurationSpinBox->setMaximum(INT_MAX);
    m_notationDurationSpinBox->setSingleStep(Note(Note::Shortest).getDuration());
    m_notationDurationEditButton = new QPushButton(tr("edit"), m_notationGroupBox);
    npLayout->addWidget(m_notationDurationSpinBox, 2, 1);
    npLayout->addWidget(m_notationDurationEditButton, 2, 2);

    m_notationGroupBox->setLayout(layout);

    connect(m_notationDurationSpinBox,
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &EditEvent::slotNotationDurationChanged);
    connect(m_notationDurationEditButton, &QAbstractButton::released,
            this, &EditEvent::slotEditNotationDuration);

#endif

    updateWidgets();

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void
EditEvent::updateWidgets()
{
    // ??? This routine needs to go away.  Move init to the ctor.
    // ??? The only "update" we are going to need will be flipping the
    //     widget stack when the type changes.  That will be handled by
    //     slotEventTypeChanged().

    // Block Signals

    // ??? Get rid of these.  Use the signal that doesn't fire on
    //     non-user input.
    if (m_typeCombo) {
        m_typeCombo->blockSignals(true);
    }
    m_timeSpinBox->blockSignals(true);
#if 0
    m_durationSpinBox->blockSignals(true);
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
    m_timeLabel->show();
    m_timeSpinBox->show();
    m_timeEditButton->show();
    m_timeSpinBox->setValue(m_event.getAbsoluteTime());

#if 0
    // Duration
    m_durationLabel->setText(tr("Duration:"));
    m_durationLabel->show();
    m_durationSpinBox->show();
    m_durationEditButton->show();
    m_durationSpinBox->setValue(m_event.getDuration());
    m_duration = m_event.getDuration();
#endif

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

#if 0
    m_durationSpinBox->blockSignals(false);
    m_notationTimeSpinBox->blockSignals(false);
    m_notationDurationSpinBox->blockSignals(false);
    m_pitchSpinBox->blockSignals(false);
    m_velocitySpinBox->blockSignals(false);
    m_metaEdit->blockSignals(false);
    slotLockNotationChanged();
#endif

}

timeT
EditEvent::getAbsoluteTime() const
{
    return m_timeSpinBox->value();
}

Event
EditEvent::getEvent()
{
    // ??? I suspect calling this routine twice is fatal since it modifies
    //     m_event.  Need to fix that.  Or at least fail.  Maybe make m_event
    //     const.

    // If we are inserting a new Event...
    if (m_typeCombo) {
        // Create a new default Event of the right type.
        // Just need the type, but there is no type-only ctor.
        m_event = Event(qstrtostr(m_typeCombo->currentText()), m_timeSpinBox->value());
    }

    Event event(m_event, m_timeSpinBox->value());

    // Let the widget make its changes.
    //if (m_eventWidgetStack)  // inserting
    //    m_eventWidgetStack->updateEvent(event);
    if (m_eventWidget)  // editing
        m_eventWidget->updateEvent(event);

    // Changes from the Advanced Properties.
    event.setSubOrdering(m_subOrdering->value());

    // ??? Set the remaining properties from the table.

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
    // ??? Does QComboBox::currentText() work here?  It's simpler.
    std::string type = qstrtostr(m_typeCombo->itemText(value));

    // Make sure the sub-ordering is appropriate.
    m_subOrdering->setValue(getSubOrdering(type));

    updateWidgets();

#if 0
    // update whatever pitch and velocity correspond to
    if (!m_pitchSpinBox->isHidden())
        slotPitchChanged(m_pitchSpinBox->value());
    if (!m_velocitySpinBox->isHidden())
        slotVelocityChanged(m_velocitySpinBox->value());
#endif
}

#if 0
void
EditEvent::slotAbsoluteTimeChanged(int /*value*/)
{
    m_absoluteTime = value;

    if (m_notationGroupBox->isHidden()) {
        m_notationAbsoluteTime = value;
    } else if (m_lockNotationValues->isChecked()) {
        m_notationAbsoluteTime = value;
        m_notationTimeSpinBox->setValue(value);
    }
}
#endif
#if 0
void
EditEvent::slotNotationAbsoluteTimeChanged(int value)
{
    m_notationAbsoluteTime = value;
}
#endif
#if 0
void
EditEvent::slotDurationChanged(int value)
{
    m_duration = value;

    if (m_notationGroupBox->isHidden()) {
        m_notationDuration = value;
    } else if (m_lockNotationValues->isChecked()) {
        m_notationDuration = value;
        m_notationDurationSpinBox->setValue(value);
    }
}
#endif
#if 0
void
EditEvent::slotNotationDurationChanged(int value)
{
    m_notationDuration = value;
}
#endif
#if 0
void
EditEvent::slotPitchChanged(int value)
{
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
#endif

#if 0
void
EditEvent::slotVelocityChanged(int value)
{
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
#endif
#if 0
void
EditEvent::slotMetaChanged(const QString &)
{
}
#endif
#if 0
void
EditEvent::slotLockNotationChanged()
{
    // Enable/disable notation fields as appropriate.
    const bool enable = !m_lockNotationValues->isChecked();

    m_notationTimeSpinBox->setEnabled(enable);
    m_notationTimeEditButton->setEnabled(enable);
    m_notationDurationSpinBox->setEnabled(enable);
    m_notationDurationEditButton->setEnabled(enable);
}
#endif

void
EditEvent::slotEditAbsoluteTime()
{
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    TimeDialog dialog(this, tr("Edit Event Time"),
                      &composition,
                      m_timeSpinBox->value(),
                      true);
    if (dialog.exec() == QDialog::Accepted)
        m_timeSpinBox->setValue(dialog.getTime());
}

#if 0
void
EditEvent::slotEditPitch()
{
    PitchDialog dialog(this, tr("Edit Pitch"), m_pitchSpinBox->value());
    if (dialog.exec() == QDialog::Accepted) {
        m_pitchSpinBox->setValue(dialog.getPitch());
    }
}
#endif
#if 0
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
#endif
void EditEvent::addProperty(const PropertyName &name)
{
    // Add a row to the table
    const int row = m_propertyTable->rowCount();
    m_propertyTable->insertRow(row);

    // Go with bold for persistent properties.
    const bool bold = m_event.isPersistent(name);
    QFont boldFont;

    int col{0};

    // Name
    QTableWidgetItem *nameItem = new QTableWidgetItem(name.getName().c_str());
    if (bold) {
        boldFont = nameItem->font();
        boldFont.setBold(true);
        nameItem->setFont(boldFont);
    }
    m_propertyTable->setItem(row, col++, nameItem);

    // Type
    QTableWidgetItem *item = new QTableWidgetItem(
            m_event.getPropertyTypeAsString(name).c_str());
    if (bold)
        item->setFont(boldFont);
    m_propertyTable->setItem(row, col++, item);

    // Value
    item = new QTableWidgetItem(m_event.getAsString(name).c_str());
    if (bold)
        item->setFont(boldFont);
    m_propertyTable->setItem(row, col++, item);
}

void EditEvent::updatePropertyTable()
{
    // See EventListEditor.

    // Hide the vertical header
    m_propertyTable->verticalHeader()->hide();

    QStringList columnNames;
    columnNames << tr("Name");
    columnNames << tr("Type");
    columnNames << tr("Value");
    m_propertyTable->setColumnCount(columnNames.size());
    m_propertyTable->setHorizontalHeaderLabels(columnNames);

    // Add persistent properties to the table.

    // Get the property filter.
    std::set<PropertyName> propertyFilter;
    //if (m_eventWidgetStack)
    //    propertyFilter = m_eventWidgetStack->getPropertyFilter();
    if (m_eventWidget)
        propertyFilter = m_eventWidget->getPropertyFilter();

    m_propertyTable->setRowCount(0);

    Event::PropertyNames propertyNames = m_event.getPersistentPropertyNames();

    // For each property, add to table.
    for (const PropertyName &propertyName : propertyNames) {
        // Skip any that need filtering.
        if (propertyFilter.find(propertyName) != propertyFilter.end())
            continue;
        addProperty(propertyName);
    }

    // Add non-persistent properties to the table.

    propertyNames = m_event.getNonPersistentPropertyNames();

    // For each property, add to table.
    for (const PropertyName &propertyName : propertyNames) {
        // Skip any that need filtering.
        if (propertyFilter.find(propertyName) != propertyFilter.end())
            continue;
        addProperty(propertyName);
    }
}


}
