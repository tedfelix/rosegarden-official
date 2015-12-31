/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PitchBendSequenceDialog.h"
#include "base/ControlParameter.h"
#include "base/MidiTypes.h"
#include "base/RealTime.h"
#include "base/Selection.h"
#include "commands/edit/EventInsertionCommand.h"
#include "commands/edit/EraseCommand.h"
#include "document/CommandHistory.h"
#include "document/Command.h"
#include "misc/ConfigGroups.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QSettings>
#include <QCloseEvent>
#include <QUrl>
#include <QDesktopServices>
#include <QtGlobal>

#include <cmath>

namespace Rosegarden
{

PitchBendSequenceDialog::PitchBendSequenceDialog(QWidget *parent,
    Segment *segment, const ControlParameter &control, timeT startTime, timeT endTime) :
    QDialog(parent),
    m_segment(segment),
    m_control(control),
    m_numBuiltins((control.getName() == control.getPitchBend().getName()) ?
                  EndBuiltIns :
                  0),
    m_startTime(startTime),
    m_endTime(endTime)
{
    setModal(true);
    bool sensible = m_startTime < m_endTime;

    QString controllerName(control.getName().data());
    setWindowTitle(tr("%1 Sequence").arg(controllerName));

    QSettings settings;
    settings.beginGroup(PitchBendSequenceConfigGroup);

    
    QWidget* vbox = dynamic_cast<QWidget*>( this );
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vbox->setLayout(vboxLayout);

    if (sensible) {
        enum WhatVaries {
            Pitch, Volume, Other
        };
        WhatVaries whatVaries =
            (m_control.getType() == PitchBend::EventType) ? Pitch :
            (m_control.getControllerValue() == 7)         ? Volume :
            (m_control.getControllerValue() == 11)        ? Volume :
            Other;
        const double maxSpinboxValue = getMaxSpinboxValue();
        const double minSpinboxValue = getMinSpinboxValue();
        const double maxSpinboxAbsValue =
            std::max (maxSpinboxValue, -minSpinboxValue);
        const int valueSpinboxDecimals = useTrueValues() ? 0 : 2;

        const int numSavedSettings = 10;
        const int startSavedSettings = m_numBuiltins;
        const int endSavedSettings   = startSavedSettings + numSavedSettings;
        
        /** The replace-mode group comes first because one of its settings
            (OnlyErase) invalidates the rest of the dialog. **/
        QGroupBox *replaceModeGroupBox = new QGroupBox(tr("Replacement mode"));
        QVBoxLayout *replaceModeGroupLayoutBox = new QVBoxLayout();

        vboxLayout->addWidget(replaceModeGroupBox);
        replaceModeGroupBox->setLayout(replaceModeGroupLayoutBox);

        m_radioReplace = new QRadioButton(tr("Replace old events"));
        m_radioReplace->setToolTip(tr("<qt>Erase existing pitchbends or controllers of this type in this range before adding new ones</qt>"));
        
        m_radioOnlyAdd = new QRadioButton(tr("Add new events to old ones"));
        m_radioOnlyAdd->setToolTip(tr("<qt>Add new pitchbends or controllers without affecting existing ones.</qt>"));

        m_radioOnlyErase = new QRadioButton(tr("Just erase old events"));
        m_radioOnlyErase->setToolTip(tr("<qt>Don't add any events, just erase existing pitchbends or controllers of this type in this range.</qt>"));

        
        QHBoxLayout *replaceModeBox = new QHBoxLayout();
        replaceModeGroupLayoutBox->addLayout(replaceModeBox);
        replaceModeBox->addStretch(0);
        replaceModeBox->addWidget(m_radioReplace);
        replaceModeBox->addWidget(m_radioOnlyAdd);
        replaceModeBox->addWidget(m_radioOnlyErase);

        vboxLayout->addSpacing(15);

        // Preset can change what's shown, which normally stretches
        // the layout and thus moves preset out from under the user's
        // mouse.  To avoid this, we call `vboxLayout->addStretch'
        // generously after this point in the layout and never call it
        // before this point.

        QGroupBox *presetBox = new QGroupBox(tr("Preset"));
        QGridLayout *presetGrid = new QGridLayout;
        presetBox->setLayout(presetGrid);
        presetGrid->setSpacing(5);
        vboxLayout->addWidget(presetBox);
        QLabel *presetLabel = new QLabel(tr("Preset:"));
        presetLabel->
            setToolTip(tr("<qt>Use this saved, user editable setting.</qt>"));
        presetGrid->addWidget(presetLabel, 0, 0);
        m_sequencePreset = new QComboBox;
        presetGrid->addWidget(m_sequencePreset, 0, 1);

        // Hack: We psychically know that this adds the right number
        // of builtin presets in the right places.
        if (m_numBuiltins > 0) {
            m_sequencePreset->addItem(tr("Linear ramp"), LinearRamp);
            m_sequencePreset->addItem(tr("Fast vibrato arm release"), FastVibratoArmRelease);
            m_sequencePreset->addItem(tr("Vibrato"), Vibrato);
        }
        
        for (int i = startSavedSettings; i <= endSavedSettings; ++i)
            {
                int apparentIndex = i + 1 - startSavedSettings;
                m_sequencePreset->addItem(tr("Saved setting %1").arg(apparentIndex), i);
            }
        m_sequencePreset->setCurrentIndex(settings.value("sequence_preset", int(startSavedSettings)).toInt());

        vboxLayout->addStretch(15);

        QString prebendText =
            (whatVaries == Pitch) ?
            tr("Pre Bend") :
            tr("Pre Ramp");
        QGroupBox *prebendBox = new QGroupBox(prebendText);
        prebendBox->setContentsMargins(5, 5, 5, 5);
        QGridLayout *prebendGrid = new QGridLayout;
        prebendGrid->setSpacing(5);
        vboxLayout->addWidget(prebendBox);

        QString prebendValueText =
            useTrueValues() ?
            tr("Start at value:") :
            tr("Start at value (%):");
        prebendGrid->addWidget(new QLabel(prebendValueText), 0, 0);
        m_prebendValue = new QDoubleSpinBox();
        m_prebendValue->setAccelerated(true);
        m_prebendValue->setMaximum(maxSpinboxValue);
        m_prebendValue->setMinimum(minSpinboxValue);
        m_prebendValue->setDecimals(valueSpinboxDecimals);
        m_prebendValue->setSingleStep(5);
        prebendGrid->addWidget(m_prebendValue, 0 , 1);
    
        prebendBox->setLayout(prebendGrid);

        QLabel *durationLabel = new QLabel(tr("Wait (%):"));
        durationLabel->
            setToolTip(tr("<qt>How long to wait before starting the bend or ramp, as a percentage of the total time</qt>"));
        prebendGrid->addWidget(durationLabel, 1, 0);
        m_prebendDuration = new QDoubleSpinBox();
        m_prebendDuration->setAccelerated(true);
        m_prebendDuration->setMaximum(100);
        m_prebendDuration->setMinimum(0);
        m_prebendDuration->setSingleStep(5);
        prebendGrid->addWidget(m_prebendDuration, 1 , 1);

        vboxLayout->addStretch(15);

        QString sequenceText =
            (whatVaries == Pitch) ?
            tr("Bend Sequence") :
            tr("Ramp Sequence");
        QGroupBox *sequencebox = new QGroupBox(sequenceText);
        sequencebox->setContentsMargins(5, 5, 5, 5);
        QGridLayout *sequencegrid = new QGridLayout;
        sequencegrid->setSpacing(5);
        vboxLayout->addWidget(sequencebox);
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
        m_sequenceRampDuration = new QDoubleSpinBox();
        m_sequenceRampDuration->setAccelerated(true);
        m_sequenceRampDuration->setMaximum(100);
        m_sequenceRampDuration->setMinimum(0);
        m_sequenceRampDuration->setSingleStep(5);
        sequencegrid->addWidget(m_sequenceRampDuration, 1, 1);

        QString sequenceEndValueText =
            useTrueValues() ?
            tr("End value:") :
            tr("End value (%):");
        sequencegrid->addWidget(new QLabel(sequenceEndValueText), 2, 0);
        m_sequenceEndValue = new QDoubleSpinBox();
        m_sequenceEndValue->setAccelerated(true);
        m_sequenceEndValue->setMaximum(maxSpinboxValue);
        m_sequenceEndValue->setMinimum(minSpinboxValue);
        m_sequenceEndValue->setDecimals(valueSpinboxDecimals);
        m_sequenceEndValue->setSingleStep(5);
        sequencegrid->addWidget(m_sequenceEndValue, 2, 1);

        vboxLayout->addStretch(15);

        /*** Sub-group vibrato ****/

        QString vibratoBoxText =
            (whatVaries == Pitch)  ? tr("Vibrato") :
            (whatVaries == Volume) ? tr("Tremolo") :
            tr("LFO");
        m_vibratoBox = new QGroupBox(vibratoBoxText);
        m_vibratoBox->
            setToolTip(tr("<qt>Low-frequency oscillation for this controller. This is only possible when Ramp mode is linear and <i>Use this many steps</i> is set.</qt>"));
        m_vibratoBox->setContentsMargins(5, 5, 5, 5);
        QGridLayout *vibratoGrid = new QGridLayout;
        vibratoGrid->setSpacing(5);
        vboxLayout->addWidget(m_vibratoBox);
        m_vibratoBox->setLayout(vibratoGrid);

        QString vibratoStartAmplitudeText =
            useTrueValues() ?
            tr("Start amplitude:") :
            tr("Start amplitude (%):");
        vibratoGrid->addWidget(new QLabel(vibratoStartAmplitudeText), 3, 0);
        m_vibratoStartAmplitude = new QDoubleSpinBox();
        m_vibratoStartAmplitude->setAccelerated(true);
        m_vibratoStartAmplitude->setMaximum(maxSpinboxAbsValue);
        m_vibratoStartAmplitude->setMinimum(0);
        m_vibratoStartAmplitude->setSingleStep(10);
        vibratoGrid->addWidget(m_vibratoStartAmplitude, 3, 1);

        QString vibratoEndAmplitudeText =
            useTrueValues() ?
            tr("End amplitude:") :
            tr("End amplitude (%):");
        vibratoGrid->addWidget(new QLabel(vibratoEndAmplitudeText), 4, 0);
        m_vibratoEndAmplitude = new QDoubleSpinBox();
        m_vibratoEndAmplitude->setAccelerated(true);
        m_vibratoEndAmplitude->setMaximum(maxSpinboxAbsValue);
        m_vibratoEndAmplitude->setMinimum(0);
        m_vibratoEndAmplitude->setSingleStep(10);
        vibratoGrid->addWidget(m_vibratoEndAmplitude, 4, 1);

        QLabel * vibratoFrequencyLabel =
            new QLabel(tr("Hertz (Hz):"));
        vibratoFrequencyLabel->
            setToolTip(tr("<qt>Frequency in hertz (cycles per second)</qt>"));
        vibratoGrid->addWidget(vibratoFrequencyLabel, 5, 0);
        m_vibratoFrequency = new QDoubleSpinBox();
        m_vibratoFrequency->setAccelerated(true);
        m_vibratoFrequency->setMaximum(200);
        m_vibratoFrequency->setMinimum(0.1);
        m_vibratoFrequency->setSingleStep(1.0);
        m_vibratoFrequency->setDecimals(2);
        vibratoGrid->addWidget(m_vibratoFrequency, 5, 1);
        
        vboxLayout->addStretch(15);

        /*** Sub-group the contour of the ramp ****/
        
        QGroupBox *rampModeGroupBox = new QGroupBox(tr("Ramp mode"));
        QHBoxLayout *rampModeGroupLayoutBox = new QHBoxLayout();
        vboxLayout->addWidget(rampModeGroupBox);
        rampModeGroupBox->setLayout(rampModeGroupLayoutBox);

        m_radioRampLinear = new QRadioButton(tr("Linear"));
        m_radioRampLinear->
            setToolTip(tr("<qt>Ramp slopes linearly. Vibrato is possible if <i>Use this many steps</i> is set</qt>"));
        m_radioRampLogarithmic = new QRadioButton(tr("Logarithmic"));
        m_radioRampLogarithmic->
            setToolTip(tr("<qt>Ramp slopes logarithmically</qt>"));
        m_radioRampHalfSine = new QRadioButton(tr("Half sine"));
        m_radioRampHalfSine->
            setToolTip(tr("<qt>Ramp slopes like one half of a sine wave (trough to peak)</qt>"));
        m_radioRampQuarterSine = new QRadioButton(tr("Quarter sine"));
        m_radioRampQuarterSine->
            setToolTip(tr("<qt>Ramp slopes like one quarter of a sine wave (zero to peak)</qt>"));

        rampModeGroupLayoutBox->addWidget(m_radioRampLinear);
        rampModeGroupLayoutBox->addWidget(m_radioRampLogarithmic);
        rampModeGroupLayoutBox->addWidget(m_radioRampQuarterSine);
        rampModeGroupLayoutBox->addWidget(m_radioRampHalfSine);
        
        vboxLayout->addStretch(15);

        /*** Sub-group how we set step size or step count ****/

        QGroupBox *stepSizeStyleGroupBox =
            new QGroupBox(tr("How many steps"));
        QVBoxLayout *stepSizeStyleGroupLayoutBox = new QVBoxLayout();

        /* Stepsize -> SELECT */
        m_radioStepSizeDirect = new QRadioButton(tr("Use step size (%):"));
        m_radioStepSizeDirect->
            setToolTip(tr("<qt>Each step in the ramp will be as close to this size as possible. Vibrato is not possible with this setting</qt>"));
        m_radioStepSizeByCount = new QRadioButton(tr("Use this many steps:"));
        m_radioStepSizeByCount->
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
        stepSizeManualHBox->addWidget(m_radioStepSizeDirect);
        stepSizeManualHBox->addWidget(m_stepSize);

        /* Stepsize -> by count -> Resolution */        
        m_resolution = new QDoubleSpinBox();
        m_resolution->setAccelerated(true);
        m_resolution->setMaximum(300);
        m_resolution->setMinimum(2);
        m_resolution->setSingleStep(10);
        m_resolution->setDecimals(0);

        /* Stepsize -> by count */        
        QHBoxLayout *stepSizeByCountHBox = new QHBoxLayout();
        stepSizeByCountHBox->addWidget(m_radioStepSizeByCount);
        stepSizeByCountHBox->addWidget(m_resolution);
        
        /* Stepsize itself */        
        vboxLayout->addWidget(stepSizeStyleGroupBox);
        stepSizeStyleGroupBox->setLayout(stepSizeStyleGroupLayoutBox);

        stepSizeStyleGroupLayoutBox->addLayout(stepSizeManualHBox);
        stepSizeStyleGroupLayoutBox->addLayout(stepSizeByCountHBox);

        vboxLayout->addStretch(15);

        
        slotSequencePresetChanged(m_sequencePreset->currentIndex());
        // the above called slotStepSizeStyleChanged() which called
        // maybeEnableVibratoFields()
        m_radioReplace->setChecked(true);

        connect(m_sequencePreset, SIGNAL(activated(int)), this,
                SLOT(slotSequencePresetChanged(int)));
        connect(m_radioOnlyErase, SIGNAL(toggled(bool)),
                this, SLOT(slotOnlyEraseClicked(bool)));
        connect(m_radioRampLinear, SIGNAL(toggled(bool)),
                this, SLOT(slotLinearRampClicked(bool)));
        
        // We connect all these buttons to slotStepSizeStyleChanged,
        // which will react only to the current selected one.
        connect(m_radioStepSizeDirect, SIGNAL(toggled(bool)),
                this, SLOT(slotStepSizeStyleChanged(bool)));
        connect(m_radioStepSizeByCount, SIGNAL(toggled(bool)),
                this, SLOT(slotStepSizeStyleChanged(bool)));
    } else {
        vboxLayout->addWidget(new QLabel(tr("Invalid end time. Have you selected some events?")));
    }
    QFlags<QDialogButtonBox::StandardButton> flags =
        sensible ?
        (QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
         QDialogButtonBox::Help) :
        (QDialogButtonBox::Cancel | QDialogButtonBox::Help);
        
    QDialogButtonBox *buttonBox = new QDialogButtonBox(flags);
    vboxLayout->addWidget(buttonBox, 1, 0);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));
}

void
PitchBendSequenceDialog::slotOnlyEraseClicked(bool checked)
{
    // (De)activate controls that are meaningless for "OnlyErase"
    m_prebendValue->setEnabled(!checked);
    m_prebendDuration->setEnabled(!checked);
    m_sequenceRampDuration->setEnabled(!checked);
    m_sequenceEndValue->setEnabled(!checked);
    m_sequencePreset->setEnabled(!checked);

    m_radioStepSizeDirect->setEnabled(!checked);
    m_radioStepSizeByCount->setEnabled(!checked);

    m_radioRampLinear->setEnabled(!checked);
    m_radioRampLogarithmic->setEnabled(!checked);
    m_radioRampHalfSine->setEnabled(!checked);
    m_radioRampQuarterSine->setEnabled(!checked);

    // Handle these specially because they can also be disabled by
    // step size style or unchecking linear ramp.
    if (checked) {
        m_resolution->setEnabled(false);
        m_stepSize->setEnabled(false);
        m_vibratoStartAmplitude->setEnabled(false);
        m_vibratoEndAmplitude->setEnabled(false);
        m_vibratoFrequency->setEnabled(false);
    } else {
        slotStepSizeStyleChanged(true);
        //maybeEnableVibratoFields();
    }
}

void
PitchBendSequenceDialog::maybeEnableVibratoFields(void)
{
    bool enable =
        m_radioRampLinear->isChecked() &&
        m_radioStepSizeByCount->isChecked();
    
    m_vibratoBox->setVisible(enable);
}

void
PitchBendSequenceDialog::slotLinearRampClicked(bool /*checked*/)
{
    maybeEnableVibratoFields();
}

void
PitchBendSequenceDialog::slotStepSizeStyleChanged(bool checked)
{
    // We get multiple signals here for each radio change, one for
    // each radio button.  Since we only want to react once, return
    // immediately except on the single checked button.
    if (!checked) { return; }
    
    m_stepSize->setEnabled(m_radioStepSizeDirect->isChecked());
    m_resolution->setEnabled(m_radioStepSizeByCount->isChecked());
    maybeEnableVibratoFields();
}

void
PitchBendSequenceDialog::slotSequencePresetChanged(int index) {
    // Get built-in or saved settings for the new preset.
    if (index >= m_numBuiltins) {
        restorePreset(index);
    } else {
        switch (index) {
        case LinearRamp:
            m_prebendDuration->setValue(0);
            m_sequenceRampDuration->setValue(100);
            m_vibratoStartAmplitude->setValue(0);
            m_vibratoEndAmplitude->setValue(0);
            m_radioRampLinear->setChecked(true);
            break;
        case FastVibratoArmRelease:
            m_prebendValue->setValue(-20);
            m_prebendDuration->setValue(5);
            m_sequenceRampDuration->setValue(0);
            m_sequenceEndValue->setValue(0);
            m_vibratoStartAmplitude->setValue(30);
            m_vibratoEndAmplitude->setValue(0);
            m_vibratoFrequency->setValue(14);
            m_resolution->setValue(getElapsedSeconds() * 20);
            m_radioRampLinear->setChecked(true);
            m_radioStepSizeByCount->setChecked(true);
            break;
        case Vibrato:
            m_prebendValue->setValue(0);
            m_prebendDuration->setValue(0);
            m_sequenceRampDuration->setValue(0);
            m_sequenceEndValue->setValue(0);
            m_vibratoStartAmplitude->setValue(10);
            m_vibratoEndAmplitude->setValue(10);
            m_vibratoFrequency->setValue(6);
            m_resolution->setValue(getElapsedSeconds() * 20);
            m_radioRampLinear->setChecked(true);
            m_radioStepSizeByCount->setChecked(true);
            break;
        default:
            /* This can't be reached, but just in case we're wrong, we
               give it a way to make a valid preset. */
            restorePreset(index);
            break;
        }
    }
    slotStepSizeStyleChanged(true);
    // the above called maybeEnableVibratoFields();
}

bool
PitchBendSequenceDialog::
useTrueValues(void) const
{
    return m_control.getType() == Controller::EventType;
}

double
PitchBendSequenceDialog::
valueDeltaToPercent(int valueDelta) const
{
    const int range  = m_control.getMax() - m_control.getMin();
    return 100.0 * valueDelta / range;
}
int
PitchBendSequenceDialog::
percentToValueDelta(double percent) const
{
    const int range  = m_control.getMax() - m_control.getMin();
    return (percent/100.0) * range;
}

double
PitchBendSequenceDialog::
getMaxSpinboxValue(void) const
{
    const int rangeAboveDefault = m_control.getMax() - m_control.getDefault();
    if (useTrueValues()) {
        return rangeAboveDefault;
    } else {
        return valueDeltaToPercent(rangeAboveDefault * 2);
    }
}
double
PitchBendSequenceDialog::
getMinSpinboxValue(void) const
{
    /* rangeBelowDefault and return value will be negative or zero. */
    const int rangeBelowDefault = m_control.getMin() - m_control.getDefault();
    if (useTrueValues()) {
        return rangeBelowDefault;
    } else {
        return valueDeltaToPercent(rangeBelowDefault * 2);
    }
}

double
PitchBendSequenceDialog::
getSmallestSpinboxStep(void) const
{
    if (useTrueValues()) {
        return 1;
    } else {
        const int fullRange = percentToValueDelta(200.0);
        const float smallestStep = 1.000001 / float(fullRange);
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
    int value = spinboxToControlDelta(spinbox) + m_control.getDefault();
    return m_control.clamp(value);
}


PitchBendSequenceDialog::ReplaceMode
PitchBendSequenceDialog::getReplaceMode(void)
{
    return
        m_radioOnlyErase ->isChecked() ? OnlyErase :
        m_radioReplace   ->isChecked() ? Replace   :
        m_radioOnlyAdd   ->isChecked() ? OnlyAdd   :
        Replace;
}

PitchBendSequenceDialog::RampMode
PitchBendSequenceDialog::getRampMode(void)
{
    return
        m_radioRampLinear      ->isChecked() ? Linear       :
        m_radioRampLogarithmic ->isChecked() ? Logarithmic  :
        m_radioRampHalfSine    ->isChecked() ? HalfSine     :
        m_radioRampQuarterSine ->isChecked() ? QuarterSine  :
        Logarithmic;
}

void
PitchBendSequenceDialog::setRampMode(RampMode rampMode)
{
    switch (rampMode) {
    case Linear:
        m_radioRampLinear      ->setChecked(true);
        break;
    case Logarithmic:
        m_radioRampLogarithmic ->setChecked(true);
        break;
    case HalfSine:
        m_radioRampHalfSine    ->setChecked(true);
        break;
    case QuarterSine:
        m_radioRampQuarterSine ->setChecked(true);
        break;
    default:
        break;
    }
}

PitchBendSequenceDialog::StepSizeCalculation
PitchBendSequenceDialog::getStepSizeCalculation(void)
{
    return
        m_radioStepSizeDirect  ->isChecked() ? StepSizeDirect  : 
        m_radioStepSizeByCount ->isChecked() ? StepSizeByCount :
        StepSizeDirect;
}

void
PitchBendSequenceDialog::setStepSizeCalculation
(StepSizeCalculation stepSizeCalculation)
{
    switch (stepSizeCalculation) {
    case StepSizeDirect:
        m_radioStepSizeDirect  ->setChecked(true);
        break; 
    case StepSizeByCount:
        m_radioStepSizeByCount ->setChecked(true);
        break;
    default:
        break;
    }
}


void
PitchBendSequenceDialog::saveSettings()
{
    const int preset = m_sequencePreset->currentIndex();
    
    /* Only one setting (sequence_preset) is global; the other
       settings pertain to one specific preset. */

    {  // Put "settings" in its own scope to prevent accidental use
        // after endGroup.
        QSettings settings;
        settings.beginGroup(PitchBendSequenceConfigGroup);
        settings.setValue("sequence_preset", preset);
        settings.endGroup();
    }

    if (preset >= m_numBuiltins) {
        savePreset(preset);
    }
}
void
PitchBendSequenceDialog::savePreset(int preset)
{
    /* A preset is stored in one element in an array.  There is a
       different array for each controller or pitchbend.  */
    QSettings settings;
    settings.beginGroup(PitchBendSequenceConfigGroup);
    settings.beginWriteArray(m_control.getName().data());
    settings.setArrayIndex(preset);
    settings.setValue("pre_bend_value", m_prebendValue->value());
    settings.setValue("pre_bend_duration_value", m_prebendDuration->value());
    settings.setValue("sequence_ramp_duration", m_sequenceRampDuration->value());
    settings.setValue("sequence_ramp_end_value", m_sequenceEndValue->value());
    settings.setValue("vibrato_start_amplitude", m_vibratoStartAmplitude->value());
    settings.setValue("vibrato_end_amplitude", m_vibratoEndAmplitude->value());
    settings.setValue("vibrato_frequency", m_vibratoFrequency->value());
    settings.setValue("step_count", m_resolution->value());
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
    settings.beginReadArray(m_control.getName().data());
    settings.setArrayIndex(preset);
    m_prebendValue->setValue(settings.value("pre_bend_value", 0).toFloat());
    m_prebendDuration->setValue(settings.value("pre_bend_duration_value", 0).toFloat());
    m_sequenceRampDuration->setValue(settings.value("sequence_ramp_duration", 100).toFloat());
    m_sequenceEndValue->setValue(settings.value("sequence_ramp_end_value", 0).toFloat());
    m_vibratoStartAmplitude->setValue(settings.value("vibrato_start_amplitude", 0).toFloat());
    m_vibratoEndAmplitude->setValue(settings.value("vibrato_end_amplitude", 0).toFloat());
    m_vibratoFrequency->setValue(settings.value("vibrato_frequency", 10).toFloat());
    m_resolution->setValue(settings.value("step_count", 40).toInt());
    m_stepSize->setValue(settings.value("step_size", 2.0).toFloat());

    setRampMode
        (RampMode
         (settings.value("ramp_mode", Logarithmic).toInt()));
    setStepSizeCalculation
        (StepSizeCalculation
         (settings.value("step_size_calculation", StepSizeDirect).toInt()));

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
    // Pitchbend, so the resulting text is like "Pitchbend Sequence",
    // "Expression Sequence", etc.
    QString controllerName(m_control.getName().data());
    QString commandName(tr("%1 Sequence").arg(controllerName));
    MacroCommand *macro = new MacroCommand(commandName);

    if (getReplaceMode() != OnlyAdd) {
        // Selection initially contains no event, and we add all the
        // relevant ones.  
        EventSelection *selection = new EventSelection(*m_segment);
        for (Segment::const_iterator i = m_segment->findTime(m_startTime);
             i != m_segment->findTime(m_endTime);
             ++i) {
            Event *e = *i;
            if (m_control.matches(e)) {
                selection->addEvent(e, false);
            }
        }

        // EraseCommand takes ownership of "selection".
        macro->addCommand(new EraseCommand(*selection));
    }

    if (getReplaceMode() != OnlyErase) {
        if ((getRampMode() == Linear) &&
            (getStepSizeCalculation() == StepSizeByCount)) {
            addLinearCountedEvents(macro);
        } else {
            addStepwiseEvents(macro);
        }
    }
    CommandHistory::getInstance()->addCommand(macro);

    QDialog::accept();
}

double
PitchBendSequenceDialog::getElapsedSeconds(void)
{
    const Composition *composition = m_segment->getComposition();
    const RealTime realTimeDifference =
        composition->getRealTimeDifference(m_startTime, m_endTime);
    static const RealTime oneSecond(1,0);
    const double elapsedSeconds = realTimeDifference / oneSecond;
    return elapsedSeconds;
}

int
PitchBendSequenceDialog::numVibratoCycles(void)
{
    const int vibratoFrequency  = m_vibratoFrequency->value();
    const float totalCyclesExact =
        float(vibratoFrequency) * getElapsedSeconds();
    // We round so that the interval gets an exact number of cycles.
    const int totalCycles = int(totalCyclesExact + 0.5);

    // Since the user wanted vibrato, provide at least one cycle.
    if (totalCycles > 1) { return totalCycles; }
    else { return 1; }
}

void
PitchBendSequenceDialog::addLinearCountedEvents(MacroCommand *macro)
{
    static const float pi = acos(0.0) * 2.0;
    
    /* Ramp calculations. */
    const int startValue = spinboxToControl(m_prebendValue);
    const int endValue   = spinboxToControl(m_sequenceEndValue);
    const int valueChange = endValue - startValue;

    // numSteps doesn't include the initial event in the
    // total.  Eg if we ramp from 92 to 100 as {92, 96, 100}, we
    // have numSteps = 2.
    int numSteps = m_resolution->value();
    if (numSteps < 1) { numSteps = 1; }
    const int steps = numSteps;

    /* Compute values used to step thru multiple timesteps. */ 
    const timeT fullDuration = m_endTime - m_startTime;
    const timeT prerampDuration =
        (m_prebendDuration->value() * fullDuration)/100;
    const timeT sequenceStartTime = m_startTime + prerampDuration;
    const timeT sequenceDuration = m_endTime - sequenceStartTime;
    const timeT rampDuration =
        (m_sequenceRampDuration->value() * sequenceDuration)/100;
    const timeT rampEndTime = sequenceStartTime + rampDuration;
    
    const int totalCycles = numVibratoCycles();
    const float stepsPerCycle  = float(steps) / float (totalCycles);
    const int vibratoSA = spinboxToControlDelta(m_vibratoStartAmplitude);
    const int vibratoEA = spinboxToControlDelta(m_vibratoEndAmplitude);


    /* Always put an event at the start of the sequence.  */
    Event *event = m_control.newEvent(m_startTime, startValue);
    
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
        value = m_control.clamp(value);
        Event *event = m_control.newEvent(eventTime, value);
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
    static const float pi = acos(0.0) * 2.0;
    // Needed when rampMode is logarithmic. 
    static const float epsilon = 0.01;

    /* Ramp calculations. */
    const int startValue = spinboxToControl(m_prebendValue);
    const int endValue   = spinboxToControl(m_sequenceEndValue);
    const int valueChange = endValue - startValue;
    
    // numSteps is one less than the number of ramp events we
    // place.  Eg if we ramp from 92 to 100 as {92, 96, 100}, we have
    // numSteps = 2.
    int numSteps;
    switch (getStepSizeCalculation()) {
    case StepSizeByCount:
        numSteps = m_resolution->value();
        break;

    default:
        // Default shouldn't happen, but we'll just let it fall
        // thru to the base case.
    case StepSizeDirect:
        {
            const int rawStepSize = spinboxToControlDelta(m_stepSize);
            if (rawStepSize == 0) { return; }
            numSteps = abs(float(valueChange) / float(rawStepSize) + 0.5);
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
        (m_prebendDuration->value() * fullDuration)/100;
    const timeT sequenceStartTime = m_startTime + prerampDuration;
    const timeT sequenceDuration = m_endTime - sequenceStartTime;
    const timeT rampDuration =
        (m_sequenceRampDuration->value() * sequenceDuration)/100;
    const timeT rampEndTime = sequenceStartTime + rampDuration;
    const RampMode rampMode = getRampMode();
    
    /* Always put an event at the start of the sequence.  */
    Event *event = m_control.newEvent(m_startTime, startValue);
    
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
            value = m_control.clamp(value);

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

            Event *event = m_control.newEvent(eventTime, value);

            macro->addCommand(new EventInsertionCommand (*m_segment, event));
            if (eventTime >= rampEndTime) { break; }
        }
    }
    if (valueChange != 0) {
        /* If we have changed value at all, place an event for the
           final value.  Its time is one less than end-time so that we
           are only writing into the time interval we were given.  */
        Event *finalEvent =
            m_control.newEvent(m_endTime - 1, endValue);
        macro->addCommand(new EventInsertionCommand (*m_segment,
                                                     finalEvent));
    }
}

void
PitchBendSequenceDialog::slotHelpRequested()
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

