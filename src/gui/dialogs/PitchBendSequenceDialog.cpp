/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PitchBendSequenceDialog]"

#include "PitchBendSequenceDialog.h"

#include "base/ControlParameter.h"
#include "base/MidiTypes.h"  // EventType
#include "base/RealTime.h"
#include "base/Selection.h"  // EventSelection
#include "commands/edit/EventInsertionCommand.h"
#include "commands/edit/EraseCommand.h"
#include "document/CommandHistory.h"
#include "misc/ConfigGroups.h"  // PitchBendSequenceConfigGroup
#include "misc/Debug.h"
#include "misc/Constants.h"  // pi

#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QSettings>
#include <QUrl>
#include <QDesktopServices>
#include <QtGlobal>  // Q_ASSERT_X, etc...

#include <cmath>

namespace Rosegarden
{


enum PresetStyles {
  LinearRamp,
  FastVibratoArmRelease,
  Vibrato,
  EndPresetStyles
};

PitchBendSequenceDialog::PitchBendSequenceDialog(
        QWidget *parent,
        Segment *segment,
        const ControlParameter &control,
        timeT startTime,
        timeT endTime) :
    QDialog(parent),
    m_segment(segment),
    m_controlParameter(control),
    m_startTime(startTime),
    m_endTime(endTime)
{
    Q_ASSERT_X(m_startTime < m_endTime,
               "PitchBendSequenceDialog ctor",
               "Time range invalid.");

    setModal(true);

    setWindowTitle(tr("%1 Sequence").arg(
            QString(m_controlParameter.getName().data())));

    // Main dialog layout
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    // --------------------------------------
    // Replacement mode

    // The replacement modes appear at the top of the dialog because
    // "Just erase old events" disables the rest of the dialog.

    QGroupBox *replacementModeGroup = new QGroupBox(tr("Replacement mode"));
    // ??? Why hbox?  This makes the dialog super-wide.  vbox might be better.
    //     Problem with vbox is that the dialog ends up very tall.  Too tall
    //     for 768 vertical displays.  We could combine Pre bend/ramp,
    //     Bend/Ramp sequence, and Ramp mode into one Ramp/Bend group.
    //     That would reduce the required vertical space.  We should probably
    //     combine all those anyway.
    QHBoxLayout *replacementModeLayout = new QHBoxLayout();

    mainLayout->addWidget(replacementModeGroup);
    replacementModeGroup->setLayout(replacementModeLayout);

    // Replace old events
    m_replaceOldEvents = new QRadioButton(tr("Replace old events"));
    m_replaceOldEvents->setToolTip(
            tr("<qt>Erase existing pitchbends or controllers of this type in this range before adding new ones</qt>"));
    connect(m_replaceOldEvents, &QAbstractButton::clicked,
            this, &PitchBendSequenceDialog::slotReplacementModeChanged);

    // Add new events to old ones
    m_addNewEvents = new QRadioButton(tr("Add new events to old ones"));
    m_addNewEvents->setToolTip(
            tr("<qt>Add new pitchbends or controllers without affecting existing ones.</qt>"));
    connect(m_addNewEvents, &QAbstractButton::clicked,
            this, &PitchBendSequenceDialog::slotReplacementModeChanged);

    // Just erase old events
    m_justErase = new QRadioButton(tr("Just erase old events"));
    m_justErase->setToolTip(
            tr("<qt>Don't add any events, just erase existing pitchbends or controllers of this type in this range.</qt>"));
    connect(m_justErase, &QAbstractButton::clicked,
            this, &PitchBendSequenceDialog::slotReplacementModeChanged);

    replacementModeLayout->setSpacing(20);  // ok for hbox, not vbox
    replacementModeLayout->addWidget(m_replaceOldEvents);
    replacementModeLayout->addWidget(m_addNewEvents);
    replacementModeLayout->addWidget(m_justErase);

    mainLayout->addSpacing(15);

    // --------------------------------------
    // Preset

    QGroupBox *presetBox = new QGroupBox(tr("Preset"));
    // ??? Grid?  Why not HBox?  There's only one row.
    QGridLayout *presetGrid = new QGridLayout;
    presetBox->setLayout(presetGrid);
    presetGrid->setSpacing(5);
    mainLayout->addWidget(presetBox);

    // Preset:
    QLabel *presetLabel = new QLabel(tr("Preset:"));
    presetLabel->setToolTip(
            tr("<qt>Use this saved, user editable setting.</qt>"));
    presetGrid->addWidget(presetLabel, 0, 0);

    m_preset = new QComboBox;
    presetGrid->addWidget(m_preset, 0, 1);

    if (isPitchbend()) {
        m_preset->addItem(tr("Linear ramp"), LinearRamp);
        m_preset->addItem(tr("Fast vibrato arm release"), FastVibratoArmRelease);
        m_preset->addItem(tr("Vibrato"), Vibrato);
        m_numPresetStyles = 3;
    } else {
        m_numPresetStyles = 0;
    }

    // Historically, a bug in the for-loop provided one extra setting.
    // To avoid anyone losing that setting, the "11" is now a feature
    // instead of a bug.
    const int numSavedSettings = 11;
    const int startSavedSettings = m_numPresetStyles;
    const int endSavedSettings = startSavedSettings + numSavedSettings;

    for (int i = startSavedSettings; i < endSavedSettings; ++i) {
        int settingNumber = i + 1 - startSavedSettings;
        m_preset->addItem(tr("Saved setting %1").arg(settingNumber), i);
    }

    // Get the saved preset and select it.
    QSettings settings;
    settings.beginGroup(PitchBendSequenceConfigGroup);
    m_preset->setCurrentIndex(
            settings.value("sequence_preset", startSavedSettings).toInt());

    connect(m_preset,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &PitchBendSequenceDialog::slotPresetChanged);

    // --------------------------------------
    // Pre Ramp/Bend

    // Modulation parameter type.
    enum WhatVaries {
        Pitch,  // Bend/Vibrato
        Volume,  // Ramp/Tremolo
        Other  // Ramp/LFO
    };
    const WhatVaries whatVaries =
        isPitchbend() ? Pitch :
        // Volume
        (m_controlParameter.getControllerValue() == 7) ? Volume :
        // Expression
        (m_controlParameter.getControllerValue() == 11) ? Volume :
        Other;

    const double minSpinboxValue = getMinSpinboxValue();
    const double maxSpinboxValue = getMaxSpinboxValue();
    const int valueSpinboxDecimals = useTrueValues() ? 0 : 2;

    QString prebendText =
        (whatVaries == Pitch) ?
        tr("Pre Bend") :
        tr("Pre Ramp");
    QGroupBox *prebendBox = new QGroupBox(prebendText);
    prebendBox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *prebendGrid = new QGridLayout;
    prebendGrid->setSpacing(5);
    mainLayout->addWidget(prebendBox);

    QString prebendValueText =
        useTrueValues() ?
        tr("Start at value:") :
        tr("Start at value (%):");
    prebendGrid->addWidget(new QLabel(prebendValueText), 0, 0);
    m_startAtValue = new QDoubleSpinBox();
    m_startAtValue->setAccelerated(true);
    m_startAtValue->setMaximum(maxSpinboxValue);
    m_startAtValue->setMinimum(minSpinboxValue);
    m_startAtValue->setDecimals(valueSpinboxDecimals);
    m_startAtValue->setSingleStep(5);
    prebendGrid->addWidget(m_startAtValue, 0 , 1);

    prebendBox->setLayout(prebendGrid);

    QLabel *durationLabel = new QLabel(tr("Wait (%):"));
    durationLabel->
        setToolTip(tr("<qt>How long to wait before starting the bend or ramp, as a percentage of the total time</qt>"));
    prebendGrid->addWidget(durationLabel, 1, 0);
    m_wait = new QDoubleSpinBox();
    m_wait->setAccelerated(true);
    m_wait->setMaximum(100);
    m_wait->setMinimum(0);
    m_wait->setSingleStep(5);
    prebendGrid->addWidget(m_wait, 1 , 1);

    // --------------------------------------
    // Ramp/Bend Sequence

    QString sequenceText =
        (whatVaries == Pitch) ?
        tr("Bend Sequence") :
        tr("Ramp Sequence");
    QGroupBox *sequencebox = new QGroupBox(sequenceText);
    sequencebox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *sequencegrid = new QGridLayout;
    sequencegrid->setSpacing(5);
    mainLayout->addWidget(sequencebox);
    sequencebox->setLayout(sequencegrid);

    QString sequenceDurationText =
        (whatVaries == Pitch) ?
        tr("Bend duration (%):") :
        tr("Ramp duration (%):");
    QLabel *sequenceDurationLabel =
        new QLabel(sequenceDurationText);
    sequenceDurationLabel->
        setToolTip(tr("<qt>How long the bend or ramp lasts, as a percentage of the remaining time</qt>"));
    sequencegrid->addWidget(sequenceDurationLabel, 1, 0);
    m_rampDuration = new QDoubleSpinBox();
    m_rampDuration->setAccelerated(true);
    m_rampDuration->setMaximum(100);
    m_rampDuration->setMinimum(0);
    m_rampDuration->setSingleStep(5);
    sequencegrid->addWidget(m_rampDuration, 1, 1);

    QString sequenceEndValueText =
        useTrueValues() ?
        tr("End value:") :
        tr("End value (%):");
    sequencegrid->addWidget(new QLabel(sequenceEndValueText), 2, 0);
    m_endValue = new QDoubleSpinBox();
    m_endValue->setAccelerated(true);
    m_endValue->setMaximum(maxSpinboxValue);
    m_endValue->setMinimum(minSpinboxValue);
    m_endValue->setDecimals(valueSpinboxDecimals);
    m_endValue->setSingleStep(5);
    sequencegrid->addWidget(m_endValue, 2, 1);

    // --------------------------------------
    // Vibrato/Tremolo/LFO

    QString vibratoBoxText =
        (whatVaries == Pitch)  ? tr("Vibrato") :
        (whatVaries == Volume) ? tr("Tremolo") :
        tr("LFO");
    m_vibrato = new QGroupBox(vibratoBoxText);
    m_vibrato->
        setToolTip(tr("<qt>Low-frequency oscillation for this controller. This is only possible when Ramp mode is linear and <i>Use this many steps</i> is set.</qt>"));
    m_vibrato->setContentsMargins(5, 5, 5, 5);
    QGridLayout *vibratoGrid = new QGridLayout;
    vibratoGrid->setSpacing(5);
    mainLayout->addWidget(m_vibrato);
    m_vibrato->setLayout(vibratoGrid);

    const double maxSpinboxAbsValue =
        std::max (maxSpinboxValue, -minSpinboxValue);

    QString vibratoStartAmplitudeText =
        useTrueValues() ?
        tr("Start amplitude:") :
        tr("Start amplitude (%):");
    vibratoGrid->addWidget(new QLabel(vibratoStartAmplitudeText), 3, 0);
    m_startAmplitude = new QDoubleSpinBox();
    m_startAmplitude->setAccelerated(true);
    m_startAmplitude->setMaximum(maxSpinboxAbsValue);
    m_startAmplitude->setMinimum(0);
    m_startAmplitude->setSingleStep(10);
    vibratoGrid->addWidget(m_startAmplitude, 3, 1);

    QString vibratoEndAmplitudeText =
        useTrueValues() ?
        tr("End amplitude:") :
        tr("End amplitude (%):");
    vibratoGrid->addWidget(new QLabel(vibratoEndAmplitudeText), 4, 0);
    m_endAmplitude = new QDoubleSpinBox();
    m_endAmplitude->setAccelerated(true);
    m_endAmplitude->setMaximum(maxSpinboxAbsValue);
    m_endAmplitude->setMinimum(0);
    m_endAmplitude->setSingleStep(10);
    vibratoGrid->addWidget(m_endAmplitude, 4, 1);

    QLabel * vibratoFrequencyLabel =
        new QLabel(tr("Hertz (Hz):"));
    vibratoFrequencyLabel->
        setToolTip(tr("<qt>Frequency in hertz (cycles per second)</qt>"));
    vibratoGrid->addWidget(vibratoFrequencyLabel, 5, 0);
    m_hertz = new QDoubleSpinBox();
    m_hertz->setAccelerated(true);
    m_hertz->setMaximum(200);
    m_hertz->setMinimum(0.1);
    m_hertz->setSingleStep(1.0);
    m_hertz->setDecimals(2);
    vibratoGrid->addWidget(m_hertz, 5, 1);

    // --------------------------------------
    // Ramp mode

    QGroupBox *rampModeGroupBox = new QGroupBox(tr("Ramp mode"));
    QHBoxLayout *rampModeGroupLayoutBox = new QHBoxLayout();
    mainLayout->addWidget(rampModeGroupBox);
    rampModeGroupBox->setLayout(rampModeGroupLayoutBox);

    m_linear = new QRadioButton(tr("Linear"));
    m_linear->
        setToolTip(tr("<qt>Ramp slopes linearly. Vibrato is possible if <i>Use this many steps</i> is set</qt>"));
    m_logarithmic = new QRadioButton(tr("Logarithmic"));
    m_logarithmic->
        setToolTip(tr("<qt>Ramp slopes logarithmically</qt>"));
    m_halfSine = new QRadioButton(tr("Half sine"));
    m_halfSine->
        setToolTip(tr("<qt>Ramp slopes like one half of a sine wave (trough to peak)</qt>"));
    m_quarterSine = new QRadioButton(tr("Quarter sine"));
    m_quarterSine->
        setToolTip(tr("<qt>Ramp slopes like one quarter of a sine wave (zero to peak)</qt>"));

    rampModeGroupLayoutBox->addWidget(m_linear);
    rampModeGroupLayoutBox->addWidget(m_logarithmic);
    rampModeGroupLayoutBox->addWidget(m_quarterSine);
    rampModeGroupLayoutBox->addWidget(m_halfSine);

    // --------------------------------------
    // How many steps

    QGroupBox *stepSizeStyleGroupBox =
        new QGroupBox(tr("How many steps"));
    QVBoxLayout *stepSizeStyleGroupLayoutBox = new QVBoxLayout();

    /* Stepsize -> SELECT */
    m_useStepSizePercent = new QRadioButton(tr("Use step size (%):"));
    m_useStepSizePercent->
        setToolTip(tr("<qt>Each step in the ramp will be as close to this size as possible. Vibrato is not possible with this setting</qt>"));
    m_useThisManySteps = new QRadioButton(tr("Use this many steps:"));
    m_useThisManySteps->
        setToolTip(tr("<qt>The sequence will have exactly this many steps.  Vibrato is possible if Ramp mode is linear</qt>"));

    /* Stepsize -> direct -> step size */
    m_stepSize = new QDoubleSpinBox();
    m_stepSize->setAccelerated(true);
    m_stepSize->setMaximum(maxSpinboxAbsValue);
    m_stepSize->setMinimum(getSmallestSpinboxStep());
    m_stepSize->setSingleStep(4.0);
    m_stepSize->setDecimals(valueSpinboxDecimals);

    /* Stepsize -> direct */
    QHBoxLayout *stepSizeManualHBox = new QHBoxLayout();
    stepSizeManualHBox->addWidget(m_useStepSizePercent);
    stepSizeManualHBox->addWidget(m_stepSize);

    /* Stepsize -> by count -> Resolution */
    m_stepCount = new QDoubleSpinBox();
    m_stepCount->setAccelerated(true);
    m_stepCount->setMaximum(300);
    m_stepCount->setMinimum(2);
    m_stepCount->setSingleStep(10);
    m_stepCount->setDecimals(0);

    /* Stepsize -> by count */
    QHBoxLayout *stepSizeByCountHBox = new QHBoxLayout();
    stepSizeByCountHBox->addWidget(m_useThisManySteps);
    stepSizeByCountHBox->addWidget(m_stepCount);

    /* Stepsize itself */
    mainLayout->addWidget(stepSizeStyleGroupBox);
    stepSizeStyleGroupBox->setLayout(stepSizeStyleGroupLayoutBox);

    stepSizeStyleGroupLayoutBox->addLayout(stepSizeManualHBox);
    stepSizeStyleGroupLayoutBox->addLayout(stepSizeByCountHBox);

    // --------------------------------------
    // OK/Cancel/Help

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok |
                QDialogButtonBox::Cancel |
                QDialogButtonBox::Help);
    mainLayout->addWidget(buttonBox, 1, nullptr);

    // ??? Use the static_cast<> trick here.  See QComboBox::activated above.
    //     I'm assuming QDialogButtonBox::accepted() is overridden and we
    //     want the one with 0 parameters.
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested,
            this, &PitchBendSequenceDialog::slotHelp);

    // --------------------------------------

    // Default to Replace old events and the last selected preset
    m_replaceOldEvents->setChecked(true);
    slotPresetChanged(m_preset->currentIndex());

    // ??? toggled() is rarely a good idea since it is also emitted when
    //     setChecked() is called which then leads to the need to
    //     block signals in certain situations.  Would clicked() be
    //     a better signal to use here?
    connect(m_linear, &QAbstractButton::toggled,
            this, &PitchBendSequenceDialog::slotRampLinearClicked);

    // We connect all these buttons to slotStepStyleChanged,
    // which will react only to the current selected one.
    // ??? toggled() is rarely a good idea since it is also emitted when
    //     setChecked() is called which then leads to the need to
    //     block signals in certain situations.  Would clicked() be
    //     a better signal to use here?
    connect(m_useStepSizePercent, &QAbstractButton::toggled,
            this, &PitchBendSequenceDialog::slotStepStyleChanged);
    // ??? toggled() is rarely a good idea since it is also emitted when
    //     setChecked() is called which then leads to the need to
    //     block signals in certain situations.  Would clicked() be
    //     a better signal to use here?
    connect(m_useThisManySteps, &QAbstractButton::toggled,
            this, &PitchBendSequenceDialog::slotStepStyleChanged);

}

void
PitchBendSequenceDialog::updateWidgets()
{
    // Due to the use of toggled() we may end up in an endless loop here.
    // This should help confirm that situation.  Remove when all toggled()
    // have been changed to clicked().
    RG_DEBUG << "updateWidgets()";

    // If we're in JustErase mode, disable everything since all we are doing
    // is deleting events.
    if (getReplaceMode() == JustErase) {

        m_preset->setEnabled(false);

        m_startAtValue->setEnabled(false);
        m_wait->setEnabled(false);

        m_rampDuration->setEnabled(false);
        m_endValue->setEnabled(false);

        //m_vibratoBox;
        m_startAmplitude->setEnabled(false);
        m_endAmplitude->setEnabled(false);
        m_hertz->setEnabled(false);

        m_linear->setEnabled(false);
        m_logarithmic->setEnabled(false);
        m_quarterSine->setEnabled(false);
        m_halfSine->setEnabled(false);

        m_useStepSizePercent->setEnabled(false);
        m_stepSize->setEnabled(false);
        m_useThisManySteps->setEnabled(false);
        m_stepCount->setEnabled(false);

        return;
    }

    // Enable everything as appropriate.

    m_preset->setEnabled(true);

    m_startAtValue->setEnabled(true);
    m_wait->setEnabled(true);

    m_rampDuration->setEnabled(true);
    m_endValue->setEnabled(true);

    bool enableVibrato =
            m_linear->isChecked()  &&
            m_useThisManySteps->isChecked();
    //m_vibratoBox;
    m_startAmplitude->setEnabled(enableVibrato);
    m_endAmplitude->setEnabled(enableVibrato);
    m_hertz->setEnabled(enableVibrato);

    m_linear->setEnabled(true);
    m_logarithmic->setEnabled(true);
    m_quarterSine->setEnabled(true);
    m_halfSine->setEnabled(true);

    m_useStepSizePercent->setEnabled(true);
    m_stepSize->setEnabled(m_useStepSizePercent->isChecked());
    m_useThisManySteps->setEnabled(true);
    m_stepCount->setEnabled(m_useThisManySteps->isChecked());
}

void
PitchBendSequenceDialog::slotReplacementModeChanged(bool /*checked*/)
{
    updateWidgets();
}

void
PitchBendSequenceDialog::slotRampLinearClicked(bool /*checked*/)
{
    // ??? Should we handle clicked() instead of toggled() for each radio
    //     button and do this?  slotRampChanged() or something like that?
    updateWidgets();
}

void
PitchBendSequenceDialog::slotStepStyleChanged(bool /*checked*/)
{
    // ??? Should we handle clicked() instead of toggled() for each radio
    //     button?
    updateWidgets();
}

void
PitchBendSequenceDialog::slotPresetChanged(int index) {
    // Get built-in or saved settings for the new preset.
    if (index >= m_numPresetStyles) {
        restorePreset(index);
    } else {
        // Pitchbend.  Handle the presets.
        switch (index) {
        case LinearRamp:
            m_startAtValue->setValue(-20);
            m_wait->setValue(0);

            m_rampDuration->setValue(100);
            m_endValue->setValue(0);

            m_startAmplitude->setValue(0);
            m_endAmplitude->setValue(0);
            m_hertz->setValue(1);

            m_linear->setChecked(true);

            m_useThisManySteps->setChecked(true);
            m_stepCount->setValue(getTimeSpan() * 20);
            break;

        case FastVibratoArmRelease:
            m_startAtValue->setValue(-20);
            m_wait->setValue(5);

            m_rampDuration->setValue(0);
            m_endValue->setValue(0);

            m_startAmplitude->setValue(30);
            m_endAmplitude->setValue(0);
            m_hertz->setValue(14);

            m_linear->setChecked(true);

            m_useThisManySteps->setChecked(true);
            m_stepCount->setValue(getTimeSpan() * 20);
            break;

        case Vibrato:
            m_startAtValue->setValue(0);
            m_wait->setValue(0);

            m_rampDuration->setValue(0);
            m_endValue->setValue(0);

            // ??? There's no guarantee we will end up back at 0.
            //     Probably need to make that part of the algorithm.
            m_startAmplitude->setValue(10);
            m_endAmplitude->setValue(10);
            m_hertz->setValue(6);

            m_linear->setChecked(true);

            m_useThisManySteps->setChecked(true);
            m_stepCount->setValue(getTimeSpan() * 20);
            break;

        default:
            /* This can't be reached, but just in case we're wrong, we
               give it a way to make a valid preset. */
            restorePreset(index);
            break;
        }
    }

    updateWidgets();
}

bool
PitchBendSequenceDialog::
useTrueValues() const
{
    // As opposed to PitchBend::EventType.
    return m_controlParameter.getType() == Controller::EventType;
}

bool PitchBendSequenceDialog::isController() const
{
    return m_controlParameter.getType() == Controller::EventType;
}

bool PitchBendSequenceDialog::isPitchbend() const
{
    return m_controlParameter.getType() == PitchBend::EventType;
}

double
PitchBendSequenceDialog::
valueDeltaToPercent(int valueDelta) const
{
    const int range  = m_controlParameter.getMax() - m_controlParameter.getMin();
    return 100.0 * valueDelta / range;
}
int
PitchBendSequenceDialog::
percentToValueDelta(double percent) const
{
    const int range  = m_controlParameter.getMax() - m_controlParameter.getMin();
    return (percent/100.0) * range;
}

double
PitchBendSequenceDialog::
getMaxSpinboxValue() const
{
    const int rangeAboveDefault = m_controlParameter.getMax() - m_controlParameter.getDefault();
    if (useTrueValues()) {
        return rangeAboveDefault;
    } else {
        return valueDeltaToPercent(rangeAboveDefault * 2);
    }
}
double
PitchBendSequenceDialog::
getMinSpinboxValue() const
{
    /* rangeBelowDefault and return value will be negative or zero. */
    const int rangeBelowDefault = m_controlParameter.getMin() - m_controlParameter.getDefault();
    if (useTrueValues()) {
        return rangeBelowDefault;
    } else {
        return valueDeltaToPercent(rangeBelowDefault * 2);
    }
}

double
PitchBendSequenceDialog::
getSmallestSpinboxStep() const
{
    if (useTrueValues()) {
        return 1;
    } else {
        const int fullRange = percentToValueDelta(200.0);
        const double smallestStep = 1.000001 / fullRange;
        return 100.0 * smallestStep;
    }
}


int
PitchBendSequenceDialog::
spinboxToControlDelta(const QDoubleSpinBox *spinbox) const
{
    if (useTrueValues()) {
        return spinbox->value();
    } else {
        return percentToValueDelta(spinbox->value() / 2);
    }        
}

int
PitchBendSequenceDialog::
spinboxToControl(const QDoubleSpinBox *spinbox) const
{
    int value = spinboxToControlDelta(spinbox) + m_controlParameter.getDefault();
    return m_controlParameter.clamp(value);
}


PitchBendSequenceDialog::ReplaceMode
PitchBendSequenceDialog::getReplaceMode()
{
    return
        m_justErase ->isChecked() ? JustErase :
        m_replaceOldEvents   ->isChecked() ? ReplaceOldEvents   :
        m_addNewEvents   ->isChecked() ? AddNewEvents   :
        ReplaceOldEvents;
}

PitchBendSequenceDialog::RampMode
PitchBendSequenceDialog::getRampMode()
{
    return
        m_linear      ->isChecked() ? Linear       :
        m_logarithmic ->isChecked() ? Logarithmic  :
        m_halfSine    ->isChecked() ? HalfSine     :
        m_quarterSine ->isChecked() ? QuarterSine  :
        Logarithmic;
}

void
PitchBendSequenceDialog::setRampMode(RampMode rampMode)
{
    switch (rampMode) {
    case Linear:
        m_linear      ->setChecked(true);
        break;
    case Logarithmic:
        m_logarithmic ->setChecked(true);
        break;
    case HalfSine:
        m_halfSine    ->setChecked(true);
        break;
    case QuarterSine:
        m_quarterSine ->setChecked(true);
        break;
    default:
        break;
    }
}

PitchBendSequenceDialog::StepSizeCalculation
PitchBendSequenceDialog::getStepSizeCalculation()
{
    return
        m_useStepSizePercent  ->isChecked() ? StepSizePercent  :
        m_useThisManySteps ->isChecked() ? StepCount :
        StepSizePercent;
}

void
PitchBendSequenceDialog::setStepSizeCalculation
(StepSizeCalculation stepSizeCalculation)
{
    switch (stepSizeCalculation) {
    case StepSizePercent:
        m_useStepSizePercent  ->setChecked(true);
        break; 
    case StepCount:
        m_useThisManySteps ->setChecked(true);
        break;
    default:
        break;
    }
}


void
PitchBendSequenceDialog::saveSettings()
{
    const int preset = m_preset->currentIndex();

    // Save the current preset.
    QSettings settings;
    settings.beginGroup(PitchBendSequenceConfigGroup);
    settings.setValue("sequence_preset", preset);

    // If this is a "Saved setting", save it.
    if (preset >= m_numPresetStyles)
        savePreset(preset);
}
void
PitchBendSequenceDialog::savePreset(int preset)
{
    /* A preset is stored in one element in an array.  There is a
       different array for each controller or pitchbend.  */
    QSettings settings;
    settings.beginGroup(PitchBendSequenceConfigGroup);
    settings.beginWriteArray(m_controlParameter.getName().data());
    settings.setArrayIndex(preset);
    settings.setValue("pre_bend_value", m_startAtValue->value());
    settings.setValue("pre_bend_duration_value", m_wait->value());
    settings.setValue("sequence_ramp_duration", m_rampDuration->value());
    settings.setValue("sequence_ramp_end_value", m_endValue->value());
    settings.setValue("vibrato_start_amplitude", m_startAmplitude->value());
    settings.setValue("vibrato_end_amplitude", m_endAmplitude->value());
    settings.setValue("vibrato_frequency", m_hertz->value());
    settings.setValue("step_count", m_stepCount->value());
    settings.setValue("step_size", m_stepSize->value());
    settings.setValue("ramp_mode", getRampMode());
    settings.setValue("step_size_calculation", getStepSizeCalculation());
    settings.endArray();
    settings.endGroup();
}

void
PitchBendSequenceDialog::restorePreset(int preset)
{
    /* A preset is stored in one element in an array.  There is a
       different array for each controller or pitchbend.  */
    QSettings settings;
    settings.beginGroup(PitchBendSequenceConfigGroup);
    settings.beginReadArray(m_controlParameter.getName().data());
    settings.setArrayIndex(preset);
    m_startAtValue->setValue(settings.value("pre_bend_value", 0).toFloat());
    m_wait->setValue(settings.value("pre_bend_duration_value", 0).toFloat());
    m_rampDuration->setValue(settings.value("sequence_ramp_duration", 100).toFloat());
    m_endValue->setValue(settings.value("sequence_ramp_end_value", 0).toFloat());
    m_startAmplitude->setValue(settings.value("vibrato_start_amplitude", 0).toFloat());
    m_endAmplitude->setValue(settings.value("vibrato_end_amplitude", 0).toFloat());
    m_hertz->setValue(settings.value("vibrato_frequency", 10).toFloat());
    m_stepCount->setValue(settings.value("step_count", 40).toInt());
    m_stepSize->setValue(settings.value("step_size", 2.0).toFloat());

    setRampMode
        (RampMode
         (settings.value("ramp_mode", Logarithmic).toInt()));
    setStepSizeCalculation
        (StepSizeCalculation
         (settings.value("step_size_calculation", StepSizePercent).toInt()));

    settings.endArray();
    settings.endGroup();
}

    
void
PitchBendSequenceDialog::accept()
{
    /* The user has finished the dialog, other than aborting. */

    // We don't enable "OK" if the interval isn't sensible, so
    // something's badly wrong if this test fails.
    Q_ASSERT_X(m_startTime < m_endTime, "accept",
             "got a zero or negative time interval");

    /* Save current settings.  They'll be the defaults next time. */
    saveSettings();

    // TRANSLATORS: The arg value will be either a controller name or
    // PitchBend, so the resulting text is like "PitchBend Sequence",
    // "Expression Sequence", etc.
    QString controllerName(m_controlParameter.getName().data());
    QString commandName(tr("%1 Sequence").arg(controllerName));
    MacroCommand *macro = new MacroCommand(commandName);

    // In Replace and JustErase modes, erase the events in the time range.
    if (getReplaceMode() != AddNewEvents) {
        EventSelection *selection = new EventSelection(*m_segment);
        // For each event in the time range
        for (Segment::const_iterator i = m_segment->findTime(m_startTime);
             i != m_segment->findTime(m_endTime);
             ++i) {
            Event *e = *i;
            // If this is a relevant event, add it to the selection.
            if (m_controlParameter.matches(e)) {
                selection->addEvent(e, false);
            }
        }

        // Only perform the erase if there is something in the selection.
        // For some reason, if we perform the erase with an empty selection,
        // we end up with the segment expanded to the beginning of the
        // composition.
        if (selection->getAddedEvents() != 0)
        {
            // Erase the events.
            // (EraseCommand takes ownership of "selection".)
            macro->addCommand(new EraseCommand(*selection));
        }
    }

    // In Replace and OnlyAdd modes, add the requested controller events.
    if (getReplaceMode() != JustErase) {
        if ((getRampMode() == Linear) &&
            (getStepSizeCalculation() == StepCount)) {
            addLinearCountedEvents(macro);
        } else {
            addStepwiseEvents(macro);
        }
    }

    CommandHistory::getInstance()->addCommand(macro);

    QDialog::accept();
}

double
PitchBendSequenceDialog::getTimeSpan() const
{
    const Composition *composition = m_segment->getComposition();
    const RealTime realTimeDifference =
        composition->getRealTimeDifference(m_startTime, m_endTime);
    static const RealTime oneSecond(1,0);
    // ??? Why divide by 1?  Seems like that should do nothing.
    //     What does it really do?
    const double elapsedSeconds = realTimeDifference / oneSecond;
    return elapsedSeconds;
}

int
PitchBendSequenceDialog::numVibratoCycles()
{
    const int vibratoFrequency  = m_hertz->value();
    const double totalCyclesExact =
        vibratoFrequency * getTimeSpan();
    // We round so that the interval gets an exact number of cycles.
    const int totalCycles = int(totalCyclesExact + 0.5);

    // Since the user wanted vibrato, provide at least one cycle.
    if (totalCycles > 1) { return totalCycles; }
    else { return 1; }
}

void
PitchBendSequenceDialog::addLinearCountedEvents(MacroCommand *macro)
{
    /* Ramp calculations. */
    const int startValue = spinboxToControl(m_startAtValue);
    const int endValue   = spinboxToControl(m_endValue);
    const int valueChange = endValue - startValue;

    // numSteps doesn't include the initial event in the
    // total.  Eg if we ramp from 92 to 100 as {92, 96, 100}, we
    // have numSteps = 2.
    int numSteps = m_stepCount->value();
    if (numSteps < 1) { numSteps = 1; }
    const int steps = numSteps;

    /* Compute values used to step thru multiple timesteps. */ 
    const timeT fullDuration = m_endTime - m_startTime;
    const timeT prerampDuration =
        (m_wait->value() * fullDuration)/100;
    const timeT sequenceStartTime = m_startTime + prerampDuration;
    const timeT sequenceDuration = m_endTime - sequenceStartTime;
    const timeT rampDuration =
        (m_rampDuration->value() * sequenceDuration)/100;
    const timeT rampEndTime = sequenceStartTime + rampDuration;
    
    const int totalCycles = numVibratoCycles();
    const float stepsPerCycle  = float(steps) / float (totalCycles);
    const int vibratoSA = spinboxToControlDelta(m_startAmplitude);
    const int vibratoEA = spinboxToControlDelta(m_endAmplitude);


    /* Always put an event at the start of the sequence.  */
    Event *event = m_controlParameter.newEvent(m_startTime, startValue);
    
    macro->addCommand(new EventInsertionCommand (*m_segment, event));

    for ( int i = 1 ; i < steps ; i++) {
        const timeT elapsedTime = (timeT) i * sequenceDuration/(timeT) steps;
        timeT eventTime = sequenceStartTime + elapsedTime;
        if (eventTime >= m_endTime) { eventTime = m_endTime; }

        int value = endValue;
        if (eventTime >= rampEndTime) {
            value = endValue;
        } else {
            value = startValue + (valueChange*elapsedTime/rampDuration);
        }

        // The division by pi is done only to bring it into line with
        // amplitude's historical meaning in this dialog.
        const float amplitudeRatio =
            sin(float(i) * 2.0 * pi / float(stepsPerCycle)) / pi;
        
        const int amplitude = (vibratoEA - vibratoSA)*i/steps+ vibratoSA;

        value = value + int(amplitudeRatio * amplitude);
        value = m_controlParameter.clamp(value);
        Event *event = m_controlParameter.newEvent(eventTime, value);
        macro->addCommand(new EventInsertionCommand (*m_segment, event));

        /* Keep going if we are adding vibrato events, because those
           are inserted even after the ramp. */
        if ((eventTime >= rampEndTime) &&
            (vibratoEA == 0) &&
            (vibratoSA == 0))
            { break; }
    }
}

void
PitchBendSequenceDialog::addStepwiseEvents(MacroCommand *macro)
{
    // Needed when rampMode is logarithmic. 
    static const float epsilon = 0.01;

    /* Ramp calculations. */
    const int startValue = spinboxToControl(m_startAtValue);
    const int endValue   = spinboxToControl(m_endValue);
    const int valueChange = endValue - startValue;
    
    // numSteps is one less than the number of ramp events we
    // place.  Eg if we ramp from 92 to 100 as {92, 96, 100}, we have
    // numSteps = 2.
    int numSteps;
    switch (getStepSizeCalculation()) {
    case StepCount:
        numSteps = m_stepCount->value();
        break;

    default:
        // Default shouldn't happen, but we'll just let it fall
        // thru to the base case.
    case StepSizePercent:
        {
            const int rawStepSize = spinboxToControlDelta(m_stepSize);
            if (rawStepSize == 0) { return; }
            numSteps = fabs(float(valueChange) / float(rawStepSize) + 0.5);
            break;
        }
    }

    if (numSteps < 1) { numSteps = 1; }

    // Step size is floating-point so we can find exactly correct
    // fractional values and then round each one to the nearest
    // integer.  Since we want it to exactly divide the interval, we
    // recalculate it even if StepSizeDirect provided it
    const float stepSize = float(valueChange) / float(numSteps);

    /* Compute values used to step thru multiple timesteps. */
    const timeT fullDuration = m_endTime - m_startTime;
    const timeT prerampDuration =
        (m_wait->value() * fullDuration)/100;
    const timeT sequenceStartTime = m_startTime + prerampDuration;
    const timeT sequenceDuration = m_endTime - sequenceStartTime;
    const timeT rampDuration =
        (m_rampDuration->value() * sequenceDuration)/100;
    const timeT rampEndTime = sequenceStartTime + rampDuration;
    const RampMode rampMode = getRampMode();
    
    /* Always put an event at the start of the sequence.  */
    Event *event = m_controlParameter.newEvent(m_startTime, startValue);
    
    macro->addCommand(new EventInsertionCommand (*m_segment, event));

    // Remember the most recent value so we can avoid inserting it
    // twice.
    int lastValue = startValue;
    
    // Don't loop if we are not changing value.  It's wasteful and for
    // some settings it causes a divide-by-zero, which on some systems
    // leads to events inserted at a very large negative value
    // resulting in a weird behaviour of rthe GUI.
    if (valueChange != 0) {
        for (int i = 1 ; i < numSteps ; ++i) {

            /** Figure out the event's value. **/

            // We first calculate an exact float value, then round it
            // to int.  The loss of precision vs later use as a float
            // is deliberate: we want it to be the exact integer that
            // we will use.
            int value = startValue + (stepSize * i + 0.5);
            value = m_controlParameter.clamp(value);

            // Skip events that wouldn't change anything or that reach
            // the end prematurely.
            if ((value == lastValue) || (value == endValue)) {
                continue;
            } else {
                lastValue = value;
            }

            /** Figure out the time of the event. **/
            // timeRatio is when to place the event, between the start
            // of the time interval (0.0) and the end (1.0).  Each
            // branch of "switch" sets timeRatio's value.
            float timeRatio; 
            switch (rampMode) {
            case QuarterSine: {
                /* For a quarter-sine, range is 0 to pi/2, giving 0 to 1

                   value = startValue + sin(pi * ratio/2) * valueChange
                 
                   so to get time as a ratio of ramp time:

                   ratio = 2 sin^-1((value - startValue)/valueChange)/pi

                */
                const float valueRatio =
                    float(value - startValue)/float(valueChange);
                timeRatio = 2.0 * asin(valueRatio) / pi;
                break;
            }
            
            case HalfSine: {
                /* For a half-sine, range is -pi/2 to pi/2, giving -1 to 1.

                   value = startValue + (sin(pi * ratio - pi/2)/2 + 0.5) * valueChange

                   Using sin(x-pi/2) = -cos(x)

                   value = startValue + (-cos(pi * ratio)/2 + 0.5) * valueChange

                   so to get time as a ratio of ramp time:

                   ratio = arccos (1.0 - 2 ((value - startValue)/valueChange))/ pi

                */
                const float valueRatio =
                    float(value - startValue)/float(valueChange);
                timeRatio = (acos(1.0 - 2 * valueRatio)) / pi;
                break;
            }
            case Logarithmic: {
                const float denominator =
                    (log(endValue + epsilon) - log(startValue + epsilon));
                // Now it should be impossible for denominator to be
                // exactly zero, but since that once caused a serious
                // bug let's always check it (If it's not exactly 0.0
                // it wouldn't cause a divide-by-zero)
                Q_ASSERT_X(denominator != 0.0, "addStepwiseEvents",
                           "got a denominator of exactly zero");
                timeRatio = (
                             (log(startValue + epsilon + i * stepSize) - log(startValue + epsilon))
                             / denominator);
                
                break;
            }

            default: // Fall thru to the simple case.
            case Linear: {
                timeRatio = float(i) / float(numSteps);
            }
                break;
            }
            const timeT eventTime = sequenceStartTime + (timeRatio * rampDuration);

            Event *event = m_controlParameter.newEvent(eventTime, value);

            macro->addCommand(new EventInsertionCommand (*m_segment, event));
            if (eventTime >= rampEndTime) { break; }
        }
    }
    if (valueChange != 0) {
        /* If we have changed value at all, place an event for the
           final value.  Its time is one less than end-time so that we
           are only writing into the time interval we were given.  */
        Event *finalEvent =
            m_controlParameter.newEvent(m_endTime - 1, endValue);
        macro->addCommand(new EventInsertionCommand (*m_segment,
                                                     finalEvent));
    }
}

void
PitchBendSequenceDialog::slotHelp()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:pitchBendSequenceDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.  This URL points to a transitional page
    // that relates only to this branch.  If or when this branch is
    // merged, it should replace the main-branch page
    // "http://rosegardenmusic.com/wiki/doc:pitchBendSequenceDialog-en" 
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:pitchbendsequencedialog-controllerbranch-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}

