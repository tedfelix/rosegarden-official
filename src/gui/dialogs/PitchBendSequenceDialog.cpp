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

    mainLayout->addWidget(replacementModeGroup);

    // Replace old events
    m_replaceOldEvents = new QRadioButton(tr("Replace old events"));
    m_replaceOldEvents->setToolTip(
            tr("<qt>Erase existing pitchbends or controllers of this type in this range before adding new ones</qt>"));
    connect(m_replaceOldEvents, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotReplacementModeChanged);

    // Add new events to old ones
    m_addNewEvents = new QRadioButton(tr("Add new events to old ones"));
    m_addNewEvents->setToolTip(
            tr("<qt>Add new pitchbends or controllers without affecting existing ones.</qt>"));
    connect(m_addNewEvents, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotReplacementModeChanged);

    // Just erase old events
    m_justErase = new QRadioButton(tr("Just erase old events"));
    m_justErase->setToolTip(
            tr("<qt>Don't add any events, just erase existing pitchbends or controllers of this type in this range.</qt>"));
    connect(m_justErase, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotReplacementModeChanged);

    // ??? Why hbox?  This makes the dialog super-wide.  vbox might be better.
    //     Problem with vbox is that the dialog ends up very tall.  Too tall
    //     for 768 vertical displays.  We could combine Pre bend/ramp,
    //     Bend/Ramp sequence, and Ramp mode into one Ramp/Bend group.
    //     That would reduce the required vertical space.  We should probably
    //     combine all those anyway.
    QHBoxLayout *replacementModeLayout = new QHBoxLayout(replacementModeGroup);
    replacementModeLayout->setSpacing(20);  // ok for hbox, not vbox
    replacementModeLayout->addWidget(m_replaceOldEvents);
    replacementModeLayout->addWidget(m_addNewEvents);
    replacementModeLayout->addWidget(m_justErase);

    mainLayout->addSpacing(15);

    // --------------------------------------
    // Preset

    QGroupBox *presetBox = new QGroupBox(tr("Preset"));
    // ??? Grid?  Why not HBox?  There's only one row.
    QGridLayout *presetLayout = new QGridLayout(presetBox);
    presetLayout->setSpacing(5);
    mainLayout->addWidget(presetBox);

    // Preset:
    QLabel *presetLabel = new QLabel(tr("Preset:"));
    presetLabel->setToolTip(
            tr("<qt>Use this saved, user editable setting.</qt>"));
    presetLayout->addWidget(presetLabel, 0, 0);

    m_preset = new QComboBox;
    presetLayout->addWidget(m_preset, 0, 1);

    if (isPitchbend()) {
        m_preset->addItem(tr("Linear ramp"), LinearRamp);
        m_preset->addItem(tr("Fast vibrato arm release"), FastVibratoArmRelease);
        m_preset->addItem(tr("Vibrato"), Vibrato);
        m_numPresetStyles = 3;
    } else {
        m_numPresetStyles = 0;
    }

    // Save a separate preset for each kind of controller.
    m_presetKey = QString(m_controlParameter.getName().data()) + "_preset";

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
            settings.value(m_presetKey, startSavedSettings).toInt());

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
    // Whole numbers for controllers, two decimals for pitchbend percent.
    const int valueSpinboxDecimals = useValue() ? 0 : 2;

    QString prebendText =
        (whatVaries == Pitch) ?
        tr("Pre Bend") :
        tr("Pre Ramp");
    QGroupBox *prebendBox = new QGroupBox(prebendText);
    prebendBox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *prebendLayout = new QGridLayout(prebendBox);
    prebendLayout->setSpacing(5);
    mainLayout->addWidget(prebendBox);

    // Start at value
    QString prebendValueText =
        useValue() ?
        tr("Start at value:") :
        tr("Start at value (%):");
    prebendLayout->addWidget(new QLabel(prebendValueText), 0, 0);

    m_startAtValue = new QDoubleSpinBox();
    // Allow up/down arrow keys.
    m_startAtValue->setAccelerated(true);
    m_startAtValue->setDecimals(valueSpinboxDecimals);
    m_startAtValue->setMinimum(minSpinboxValue);
    m_startAtValue->setMaximum(maxSpinboxValue);
    m_startAtValue->setSingleStep(5);
    prebendLayout->addWidget(m_startAtValue, 0 , 1);

    // Wait
    QLabel *durationLabel = new QLabel(tr("Wait (%):"));
    durationLabel->
        setToolTip(tr("<qt>How long to wait before starting the bend or ramp, as a percentage of the total time</qt>"));
    prebendLayout->addWidget(durationLabel, 1, 0);

    m_wait = new QDoubleSpinBox();
    m_wait->setAccelerated(true);
    m_wait->setMinimum(0);
    m_wait->setMaximum(100);
    m_wait->setSingleStep(5);
    prebendLayout->addWidget(m_wait, 1 , 1);

    // --------------------------------------
    // Ramp/Bend Sequence

    QString sequenceText =
        (whatVaries == Pitch) ?
        tr("Bend Sequence") :
        tr("Ramp Sequence");
    QGroupBox *sequenceBox = new QGroupBox(sequenceText);
    sequenceBox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *sequenceLayout = new QGridLayout(sequenceBox);
    sequenceLayout->setSpacing(5);
    mainLayout->addWidget(sequenceBox);

    // Ramp duration
    QString rampDurationText =
        (whatVaries == Pitch) ?
        tr("Bend duration (%):") :
        tr("Ramp duration (%):");
    QLabel *rampDurationLabel = new QLabel(rampDurationText);
    rampDurationLabel->
        setToolTip(tr("<qt>How long the bend or ramp lasts, as a percentage of the remaining time</qt>"));
    sequenceLayout->addWidget(rampDurationLabel, 1, 0);

    m_rampDuration = new QDoubleSpinBox();
    m_rampDuration->setAccelerated(true);
    m_rampDuration->setMinimum(0);
    m_rampDuration->setMaximum(100);
    m_rampDuration->setSingleStep(5);
    sequenceLayout->addWidget(m_rampDuration, 1, 1);

    // End value
    QString endValueText =
        useValue() ?
        tr("End value:") :
        tr("End value (%):");
    sequenceLayout->addWidget(new QLabel(endValueText), 2, 0);

    m_endValue = new QDoubleSpinBox();
    m_endValue->setAccelerated(true);
    m_endValue->setDecimals(valueSpinboxDecimals);
    m_endValue->setMinimum(minSpinboxValue);
    m_endValue->setMaximum(maxSpinboxValue);
    m_endValue->setSingleStep(5);
    sequenceLayout->addWidget(m_endValue, 2, 1);

    // --------------------------------------
    // Vibrato/Tremolo/LFO

    QString vibratoText =
        (whatVaries == Pitch)  ? tr("Vibrato") :
        (whatVaries == Volume) ? tr("Tremolo") :
        tr("LFO");
    m_vibrato = new QGroupBox(vibratoText);
    m_vibrato->
        setToolTip(tr("<qt>Low-frequency oscillation for this controller. This is only possible when Ramp mode is linear and <i>Use this many steps</i> is set.</qt>"));
    m_vibrato->setContentsMargins(5, 5, 5, 5);
    QGridLayout *vibratoLayout = new QGridLayout(m_vibrato);
    vibratoLayout->setSpacing(5);
    mainLayout->addWidget(m_vibrato);

    // Up to 200% for pitchbend, 127 for other controllers.
    const double maxSpinboxAbsValue = isPitchbend() ? 200 : 127;

    // Start amplitude
    QString startAmplitudeText =
        useValue() ?
        tr("Start amplitude:") :
        tr("Start amplitude (%):");
    vibratoLayout->addWidget(new QLabel(startAmplitudeText), 3, 0);

    m_startAmplitude = new QDoubleSpinBox();
    m_startAmplitude->setAccelerated(true);
    m_startAmplitude->setMinimum(0);
    m_startAmplitude->setMaximum(maxSpinboxAbsValue);
    m_startAmplitude->setSingleStep(10);
    vibratoLayout->addWidget(m_startAmplitude, 3, 1);

    // End amplitude
    QString endAmplitudeText =
        useValue() ?
        tr("End amplitude:") :
        tr("End amplitude (%):");
    vibratoLayout->addWidget(new QLabel(endAmplitudeText), 4, 0);

    m_endAmplitude = new QDoubleSpinBox();
    m_endAmplitude->setAccelerated(true);
    m_endAmplitude->setMinimum(0);
    m_endAmplitude->setMaximum(maxSpinboxAbsValue);
    m_endAmplitude->setSingleStep(10);
    vibratoLayout->addWidget(m_endAmplitude, 4, 1);

    // Hertz
    QLabel *hertzLabel = new QLabel(tr("Hertz (Hz):"));
    hertzLabel->
        setToolTip(tr("<qt>Frequency in hertz (cycles per second)</qt>"));
    vibratoLayout->addWidget(hertzLabel, 5, 0);

    m_hertz = new QDoubleSpinBox();
    m_hertz->setAccelerated(true);
    m_hertz->setDecimals(2);
    m_hertz->setMinimum(0.1);
    m_hertz->setMaximum(200);
    m_hertz->setSingleStep(1.0);
    vibratoLayout->addWidget(m_hertz, 5, 1);

    // --------------------------------------
    // Ramp mode

    QGroupBox *rampModeGroupBox = new QGroupBox(tr("Ramp mode"));
    mainLayout->addWidget(rampModeGroupBox);

    // Linear
    m_linear = new QRadioButton(tr("Linear"));
    m_linear->
        setToolTip(tr("<qt>Ramp slopes linearly. Vibrato is possible if <i>Use this many steps</i> is set</qt>"));
    connect(m_linear, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotRampModeChanged);

    // Logarithmic
    m_logarithmic = new QRadioButton(tr("Logarithmic"));
    m_logarithmic->
        setToolTip(tr("<qt>Ramp slopes logarithmically</qt>"));
    connect(m_logarithmic, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotRampModeChanged);

    // Half sine
    m_halfSine = new QRadioButton(tr("Half sine"));
    m_halfSine->
        setToolTip(tr("<qt>Ramp slopes like one half of a sine wave (trough to peak)</qt>"));
    connect(m_halfSine, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotRampModeChanged);

    // Quarter sine
    m_quarterSine = new QRadioButton(tr("Quarter sine"));
    m_quarterSine->
        setToolTip(tr("<qt>Ramp slopes like one quarter of a sine wave (zero to peak)</qt>"));
    connect(m_quarterSine, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotRampModeChanged);

    QHBoxLayout *rampModeLayout = new QHBoxLayout(rampModeGroupBox);
    rampModeLayout->addWidget(m_linear);
    rampModeLayout->addWidget(m_logarithmic);
    rampModeLayout->addWidget(m_quarterSine);
    rampModeLayout->addWidget(m_halfSine);

    // --------------------------------------
    // How many steps

    QGroupBox *howManyStepsGroup =
        new QGroupBox(tr("How many steps"));
    mainLayout->addWidget(howManyStepsGroup);
    QGridLayout *howManyStepsLayout = new QGridLayout(howManyStepsGroup);

    // Use step size
    m_useStepSizePercent = new QRadioButton(tr("Use step size (%):"));
    m_useStepSizePercent->
        setToolTip(tr("<qt>Each step in the ramp will be as close to this size as possible. Vibrato is not possible with this setting</qt>"));
    howManyStepsLayout->addWidget(m_useStepSizePercent, 0, 0);
    connect(m_useStepSizePercent, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotStepModeChanged);

    m_stepSize = new QDoubleSpinBox();
    m_stepSize->setAccelerated(true);
    m_stepSize->setDecimals(valueSpinboxDecimals);
    m_stepSize->setMinimum(getMinStepSize());
    m_stepSize->setMaximum(maxSpinboxAbsValue / 2);
    m_stepSize->setSingleStep(4.0);
    howManyStepsLayout->addWidget(m_stepSize, 0, 1);

    // Use this many steps
    m_useThisManySteps = new QRadioButton(tr("Use this many steps:"));
    m_useThisManySteps->
        setToolTip(tr("<qt>The sequence will have exactly this many steps.  Vibrato is possible if Ramp mode is linear</qt>"));
    howManyStepsLayout->addWidget(m_useThisManySteps, 1, 0);
    connect(m_useThisManySteps, &QRadioButton::clicked,
            this, &PitchBendSequenceDialog::slotStepModeChanged);

    m_stepCount = new QDoubleSpinBox();
    m_stepCount->setAccelerated(true);
    m_stepCount->setDecimals(0);
    m_stepCount->setMinimum(2);
    m_stepCount->setMaximum(300);
    m_stepCount->setSingleStep(10);
    howManyStepsLayout->addWidget(m_stepCount, 1, 1);

    // --------------------------------------
    // OK/Cancel/Help

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok |
                QDialogButtonBox::Cancel |
                QDialogButtonBox::Help);
    mainLayout->addWidget(buttonBox, 1, nullptr);

    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &PitchBendSequenceDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested,
            this, &PitchBendSequenceDialog::slotHelp);

    // --------------------------------------

    // Default to Replace old events and the last selected preset
    m_replaceOldEvents->setChecked(true);
    slotPresetChanged(m_preset->currentIndex());

}

void
PitchBendSequenceDialog::updateWidgets()
{
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
PitchBendSequenceDialog::slotRampModeChanged(bool /*checked*/)
{
    updateWidgets();
}

void
PitchBendSequenceDialog::slotStepModeChanged(bool /*checked*/)
{
    updateWidgets();
}

void
PitchBendSequenceDialog::slotPresetChanged(int index) {

    if (isController()) {
        restorePreset(index);
        updateWidgets();
        return;
    }

    // Pitchbend.  Handle the presets.

    // ??? Would it make any sense to move this into restorePreset()?
    //     That would reduce this routine to the above.  That gets
    //     rid of the "if" and the double calls to updateWidgets().
    //     Then we could pull this out into a function that restorePreset()
    //     can call.  restoreBuiltInPreset(index).  That might look a
    //     little better.

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

        m_startAmplitude->setValue(20);
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

        m_startAmplitude->setValue(6);
        m_endAmplitude->setValue(6);
        m_hertz->setValue(6);

        m_linear->setChecked(true);

        m_useThisManySteps->setChecked(true);
        m_stepCount->setValue(getTimeSpan() * 20);
        break;

    default:
        restorePreset(index);
        break;
    }

    updateWidgets();
}

bool
PitchBendSequenceDialog::useValue() const
{
    // As opposed to PitchBend::EventType.
    return m_controlParameter.getType() == Controller::EventType;
}

bool
PitchBendSequenceDialog::isController() const
{
    return m_controlParameter.getType() == Controller::EventType;
}

bool
PitchBendSequenceDialog::isPitchbend() const
{
    return m_controlParameter.getType() == PitchBend::EventType;
}

double
PitchBendSequenceDialog::valueDeltaToPercent(int valueDelta) const
{
    return 100.0 * valueDelta / m_controlParameter.getRange();
}

int
PitchBendSequenceDialog::percentToValueDelta(double percent) const
{
    return percent / 100.0 * m_controlParameter.getRange();
}

double
PitchBendSequenceDialog::getMaxSpinboxValue() const
{
    const int rangeAboveDefault =
            m_controlParameter.getMax() - m_controlParameter.getDefault();

    if (useValue())
        return rangeAboveDefault;
    else  // pitchbend uses double percent (-100 to 100)
        return valueDeltaToPercent(rangeAboveDefault * 2);
}

double
PitchBendSequenceDialog::getMinSpinboxValue() const
{
    // rangeBelowDefault and return value will be negative or zero.
    const int rangeBelowDefault =
            m_controlParameter.getMin() - m_controlParameter.getDefault();

    if (useValue())
        return rangeBelowDefault;
    else  // pitchbend uses double percent (-100 to 100)
        return valueDeltaToPercent(rangeBelowDefault * 2);
}

double
PitchBendSequenceDialog::getMinStepSize() const
{
    if (useValue())
        return 1;

    // Pitchbend

#if 1
    // Simplified math.

    // Compute the (double) percentage for a change of 100 in pitchbend value.
    // Pitchbend uses double percent (-100 to 100).
    // ??? Why the .0001 epsilon?  What effect does that have later?
    //     We're working with incredibly inaccurate % here, so not
    //     sure how the epsilon would make any difference.  Need to
    //     do some experiments.
    // ??? This essentially returns 0.00305195 which cannot actually
    //     be displayed in the field.  So it appears as 0.00.
    //     100.0001 / 32766 = 0.00305195
    return 100.0001 / percentToValueDelta(200.0);
#else
    // Older version with intermediate calculations.

    // Pitchbend uses double percent (-100 to 100)
    const int fullRange = percentToValueDelta(200.0);
    const double smallestStep = 1.000001 / fullRange;
    // Return percent for a change of pitchbend by 100.
    return 100 * smallestStep;
#endif
}

int
PitchBendSequenceDialog::
spinboxToValueDelta(const QDoubleSpinBox *spinbox) const
{
    if (useValue())
        return spinbox->value();
    else  // pitchbend uses double percent (-100 to 100)
        return percentToValueDelta(spinbox->value() / 2);
}

int
PitchBendSequenceDialog::
spinboxToValue(const QDoubleSpinBox *spinbox) const
{
    int value =
            spinboxToValueDelta(spinbox) + m_controlParameter.getDefault();

    return m_controlParameter.clamp(value);
}

PitchBendSequenceDialog::ReplaceMode
PitchBendSequenceDialog::getReplaceMode()
{
    return
        m_justErase->isChecked() ? JustErase :
        m_replaceOldEvents->isChecked() ? ReplaceOldEvents :
        m_addNewEvents->isChecked() ? AddNewEvents :
        ReplaceOldEvents;
}

PitchBendSequenceDialog::RampMode
PitchBendSequenceDialog::getRampMode()
{
    return
        m_linear->isChecked() ? Linear :
        m_logarithmic->isChecked() ? Logarithmic :
        m_halfSine->isChecked() ? HalfSine :
        m_quarterSine->isChecked() ? QuarterSine :
        Logarithmic;
}

void
PitchBendSequenceDialog::setRampMode(RampMode rampMode)
{
    switch (rampMode) {
    case Linear:
        m_linear->setChecked(true);
        break;
    case Logarithmic:
        m_logarithmic->setChecked(true);
        break;
    case HalfSine:
        m_halfSine->setChecked(true);
        break;
    case QuarterSine:
        m_quarterSine->setChecked(true);
        break;
    default:
        break;
    }
}

PitchBendSequenceDialog::StepMode
PitchBendSequenceDialog::getStepMode() const
{
    return
        m_useStepSizePercent->isChecked() ? StepSizePercent :
        m_useThisManySteps->isChecked() ? StepCount :
        StepSizePercent;
}

void
PitchBendSequenceDialog::setStepMode(StepMode stepMode)
{
    switch (stepMode) {
    case StepSizePercent:
        m_useStepSizePercent->setChecked(true);
        break;
    case StepCount:
        m_useThisManySteps->setChecked(true);
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
    settings.setValue(m_presetKey, preset);

    // If this is a "Saved setting", save it.
    if (preset >= m_numPresetStyles)
        savePreset(preset);
}

void
PitchBendSequenceDialog::savePreset(int preset)
{
    QSettings settings;
    settings.beginGroup(PitchBendSequenceConfigGroup);

    // A preset is stored in one element in an array.  There is a
    // different array for each controller or pitchbend.  E.g. for
    // the first preset for the Volume CC, we would have entries
    // like this:
    //
    //     Volume\1\pre_bend_value
    //     Volume\1\pre_bend_duration_value
    //     ...

    settings.beginWriteArray(m_controlParameter.getName().data());
    settings.setArrayIndex(preset);

    settings.setValue("pre_bend_value", m_startAtValue->value());
    settings.setValue("pre_bend_duration_value", m_wait->value());
    settings.setValue("sequence_ramp_duration", m_rampDuration->value());
    settings.setValue("sequence_ramp_end_value", m_endValue->value());

    settings.setValue("vibrato_start_amplitude", m_startAmplitude->value());
    settings.setValue("vibrato_end_amplitude", m_endAmplitude->value());
    settings.setValue("vibrato_frequency", m_hertz->value());

    settings.setValue("ramp_mode", getRampMode());

    settings.setValue("step_size_calculation", getStepMode());
    settings.setValue("step_size", m_stepSize->value());
    settings.setValue("step_count", m_stepCount->value());
}

void
PitchBendSequenceDialog::restorePreset(int preset)
{
    QSettings settings;
    settings.beginGroup(PitchBendSequenceConfigGroup);

    settings.beginReadArray(m_controlParameter.getName().data());
    settings.setArrayIndex(preset);

    m_startAtValue->setValue(settings.value("pre_bend_value", 0).toDouble());
    m_wait->setValue(settings.value("pre_bend_duration_value", 0).toDouble());
    m_rampDuration->setValue(
            settings.value("sequence_ramp_duration", 100).toDouble());
    m_endValue->setValue(
            settings.value("sequence_ramp_end_value", 0).toDouble());

    m_startAmplitude->setValue(
            settings.value("vibrato_start_amplitude", 0).toDouble());
    m_endAmplitude->setValue(
            settings.value("vibrato_end_amplitude", 0).toDouble());
    m_hertz->setValue(settings.value("vibrato_frequency", 10).toDouble());

    setRampMode(RampMode(settings.value("ramp_mode", Logarithmic).toInt()));

    setStepMode(StepMode(
            settings.value("step_size_calculation", StepSizePercent).toInt()));
    m_stepCount->setValue(settings.value("step_count", 40).toInt());
    m_stepSize->setValue(settings.value("step_size", 2.0).toDouble());
}

void
PitchBendSequenceDialog::accept()
{
    // Save current settings.  They'll be the defaults next time.
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
            if (m_controlParameter.matches(e))
                selection->addEvent(e, false);
        }

        // If there is something in the selection, add the EraseCommand.
        // EraseCommand takes ownership of selection, so there is no need
        // to delete.
        // For some reason, if we perform the erase with an empty selection,
        // we end up with the segment expanded to the beginning of the
        // composition.
        if (selection->getAddedEvents() != 0)
            macro->addCommand(new EraseCommand(*selection));
        else
            delete selection;
    }

    // In Replace and OnlyAdd modes, add the requested controller events.
    if (getReplaceMode() != JustErase) {
        if ((getRampMode() == Linear) &&
            (getStepMode() == StepCount)) {
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

    return realTimeDifference.toSeconds();
}

int
PitchBendSequenceDialog::numVibratoCycles()
{
    const double totalCyclesExact = m_hertz->value() * getTimeSpan();

    // We round so that the interval gets an exact number of cycles.
    const int totalCycles = lround(totalCyclesExact);

    // Since the user wanted vibrato, provide at least one cycle.
    if (totalCycles < 1)
        return 1;

    return totalCycles;
}

void
PitchBendSequenceDialog::addLinearCountedEvents(MacroCommand *macro)
{
    // numSteps doesn't include the initial event in the
    // total.  Eg if we ramp from 92 to 100 as {92, 96, 100}, we
    // have numSteps = 2.
    int numSteps = m_stepCount->value();
    if (numSteps < 1)
        numSteps = 1;

    const timeT fullDuration = m_endTime - m_startTime;
    const timeT waitTime = m_wait->value() * fullDuration / 100;

    const timeT sequenceStartTime = m_startTime + waitTime;
    const timeT sequenceDuration = m_endTime - sequenceStartTime;

    const timeT rampDuration =
        m_rampDuration->value() * sequenceDuration / 100;
    const timeT rampEndTime = sequenceStartTime + rampDuration;

    // Ramp
    const int startValue = spinboxToValue(m_startAtValue);
    const int endValue = spinboxToValue(m_endValue);
    const int valueChange = endValue - startValue;

    // LFO/vibrato
    // ??? Although an attempt is made here to ensure we have a whole
    //     number of LFO cycles, there is no attempt made to land on
    //     0 at the end.  So we end up off by quite a bit.
    const int totalCycles = numVibratoCycles();
    const double stepsPerCycle = static_cast<double>(numSteps) / totalCycles;
    const int vibratoSA = spinboxToValueDelta(m_startAmplitude);
    const int vibratoEA = spinboxToValueDelta(m_endAmplitude);

    // Add the first Event to the MacroCommand.
    macro->addCommand(new EventInsertionCommand(
            *m_segment,
            m_controlParameter.newEvent(m_startTime, startValue)));

    // For each step
    for (int i = 1; i < numSteps; ++i) {
        const timeT elapsedTime = i * sequenceDuration / numSteps;
        timeT eventTime = sequenceStartTime + elapsedTime;
        if (eventTime >= m_endTime)
            eventTime = m_endTime;

        int value = 0;
        // If we are in the ramp, compute the ramp value.
        if (eventTime < rampEndTime)
            value = startValue + valueChange * elapsedTime / rampDuration;
        else  // ramp has ended
            value = endValue;

        // Modulation Amplitude (Vibrato/Tremolo/LFO Modulation)
        // Divide by 2 for peak-to-peak.  So, 200% total peak-to-peak for
        // pitchbend, and 127 total peak-to-peak for controllers.
        const double amplitudeRatio = sin(2.0 * pi * i / stepsPerCycle) / 2;

        // Modulation Ramp Amplitude
        const int amplitude =
                (vibratoEA - vibratoSA) * i / numSteps + vibratoSA;

        // Add in the modulation.
        value = value + lround(amplitudeRatio * amplitude);
        value = m_controlParameter.clamp(value);

        // Add the event to the MacroCommand.
        macro->addCommand(new EventInsertionCommand(
                *m_segment,
                m_controlParameter.newEvent(eventTime, value)));

        // If we're past the ramp end time, and there is no vibrato, bail.
        // We keep going if we are adding vibrato events, because those
        // are inserted even after the ramp.
        if (eventTime >= rampEndTime  &&
            vibratoEA == 0  &&  vibratoSA == 0)
            break;
    }
}

void
PitchBendSequenceDialog::addStepwiseEvents(MacroCommand *macro)
{
    // Needed when rampMode is logarithmic. 
    static const float epsilon = 0.01;

    /* Ramp calculations. */
    const int startValue = spinboxToValue(m_startAtValue);
    const int endValue   = spinboxToValue(m_endValue);
    const int valueChange = endValue - startValue;
    
    // numSteps is one less than the number of ramp events we
    // place.  Eg if we ramp from 92 to 100 as {92, 96, 100}, we have
    // numSteps = 2.
    int numSteps;
    switch (getStepMode()) {
    case StepCount:
        numSteps = m_stepCount->value();
        break;

    default:
        // Default shouldn't happen, but we'll just let it fall
        // thru to the base case.
    case StepSizePercent:
        {
            const int rawStepSize = spinboxToValueDelta(m_stepSize);
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
    
    // Add the first Event to the MacroCommand.
    macro->addCommand(new EventInsertionCommand(
            *m_segment,
            m_controlParameter.newEvent(m_startTime, startValue)));

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

            // Add the event to the MacroCommand.
            macro->addCommand(new EventInsertionCommand(
                    *m_segment,
                    m_controlParameter.newEvent(eventTime, value)));

            if (eventTime >= rampEndTime) { break; }
        }
    }

    // If the value changed
    if (valueChange != 0) {
        // Add an event for the final value.  Its time is one less than
        // end-time so that we are only writing into the time interval we
        // were given.
        macro->addCommand(new EventInsertionCommand(
                *m_segment,
                m_controlParameter.newEvent(m_endTime - 1, endValue)));
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

