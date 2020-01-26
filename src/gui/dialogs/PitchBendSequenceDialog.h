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

#ifndef RG_PITCHBENDSEQUENCEDIALOG_H
#define RG_PITCHBENDSEQUENCEDIALOG_H

#include <QDialog>

#include "base/TimeT.h"

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QRadioButton;

namespace Rosegarden
{

class ControlParameter;
class MacroCommand;
class Segment;

/// Dialog for inserting a controller sequence.
/**
 * This class provides the various controller sequence dialogs, including:
 *
 *   - PitchBend Sequence
 *   - Volume Sequence
 *   - Pan Sequence
 *   - etc...
 *
 * Though called PitchBendSequenceDialog for historical reasons, this
 * dialog deals with either pitchbend events or controller events
 * based on m_controlParameter.  It inserts, erases, or
 * replaces a series of such events in a given segment.  It now
 * supplies the functionality for several menu items.
 *
 * Used by both the NotationView and the MatrixView.
 *
 * ??? Rename: ControllerSequenceDialog
 *
 * @authors Jani (original author?),
 *          Tom Breton (Tehom),
 *          Tim Munro
 */
class PitchBendSequenceDialog : public QDialog
{
    Q_OBJECT

public:
    PitchBendSequenceDialog(
            QWidget *parent,
            Segment *segment,
            const ControlParameter &controlParameter,
            timeT startTime,
            timeT endTime);

public slots:
    // QDialog override
    void accept() override;

private slots:
    void slotJustEraseClicked(bool checked);
    void slotRampLinearClicked(bool checked);
    void slotStepSizeStyleChanged(bool checked);
    void slotSequencePresetChanged(int);

    void slotHelp();

private:
    Segment *m_segment;
    const ControlParameter &m_controlParameter;
    const timeT m_startTime;
    const timeT m_endTime;
    double getElapsedSeconds() const;

    /** Methods dealing with transforming to or from spinbox values **/

    bool useTrueValues() const;
    int spinboxToControl(const QDoubleSpinBox *spinbox) const;
    int spinboxToControlDelta(const QDoubleSpinBox *spinbox) const;
    double getMaxSpinboxValue() const;
    double getMinSpinboxValue() const;
    double getSmallestSpinboxStep() const;
    double valueDeltaToPercent(int valueDelta) const;
    int percentToValueDelta(double) const;

    /** Methods dealing with setting and reading radiobutton groups **/

    enum RampMode {
      Linear,
      Logarithmic,
      HalfSine,
      QuarterSine,
    };
    void setRampMode(RampMode rampMode);
    RampMode getRampMode();

    enum StepSizeCalculation {
      StepSizeDirect,
      StepSizeByCount,
    };
    void setStepSizeCalculation(StepSizeCalculation stepSizeCalculation);
    StepSizeCalculation getStepSizeCalculation();

    /** Methods to help manage which widgets are enabled **/

    void maybeEnableVibratoFields();

    /** Methods dealing with saving/restoring presets **/

    void saveSettings();
    void savePreset(int preset);
    void restorePreset(int preset);

    // Replacement Mode

    QRadioButton *m_radioReplace;  // Replace old events
    QRadioButton *m_radioOnlyAdd;  // Add new events to old ones
    QRadioButton *m_justErase;  // Just erase old events
    enum ReplaceMode {
      OnlyAdd,   // Only add new events.
      Replace,   // Replace old controller events here with new ones.
      JustErase, // Just erase old events
    };
    ReplaceMode getReplaceMode();

    // Preset

    // How many of the PresetStyles we are accepting.  In
    // practice, either 0 for non-pitchbend controllers or EndBuiltIns
    // (all three) for the pitchbend controller.
    const int m_numPresetStyles;
    QComboBox *m_sequencePreset;  // Preset

    // Pre Bend

    QDoubleSpinBox *m_prebendValue;  // Start at Value
    QDoubleSpinBox *m_prebendDuration;  // Wait

    // Bend Sequence

    QDoubleSpinBox *m_sequenceRampDuration;  // Bend Duration
    QDoubleSpinBox *m_sequenceEndValue;  // End Value

    // Vibrato

    QGroupBox *m_vibratoBox;
    QDoubleSpinBox *m_vibratoStartAmplitude;  // Start Amplitude
    QDoubleSpinBox *m_vibratoEndAmplitude;  // End Amplitude
    QDoubleSpinBox *m_vibratoFrequency;  // Hertz
    int numVibratoCycles();

    // Ramp mode

    QRadioButton *m_rampLinear;
    QRadioButton *m_radioRampLogarithmic;
    QRadioButton *m_radioRampQuarterSine;
    QRadioButton *m_radioRampHalfSine;

    // How many steps

    QRadioButton *m_radioStepSizeDirect;  // Use step size
    QDoubleSpinBox *m_stepSize;
    QRadioButton *m_radioStepSizeByCount;  // Use this many steps
    QDoubleSpinBox *m_resolution;


    /// Generate EventInsertionCommand objects for the Linear/Step Size By Count case.
    void addLinearCountedEvents(MacroCommand *macro);
    /// Generate EventInsertionCommand objects for all other cases.
    void addStepwiseEvents(MacroCommand *macro);

};


}

#endif
