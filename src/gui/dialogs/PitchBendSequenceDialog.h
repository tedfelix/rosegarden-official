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
    /// Enable/disable widgets for JustErase mode.
    /**
     * Replacement mode > Just erase old events
     *
     * ??? This should all be handled in a single updateWidgets().
     */
    void slotJustEraseClicked(bool checked);

    /// Fill in values when the preset is changed.
    void slotPresetChanged(int);

    /// Enable/Disable widgets for Linear mode.
    /**
     * Ramp mode > Linear
     *
     * ??? This should all be handled in a single updateWidgets().
     */
    void slotRampLinearClicked(bool checked);

    /// Enable/disable widgets based on "How many steps" mode.
    /**
     * How many steps > Use step size (%)
     * How many steps > Use this many steps
     *
     * ??? This should all be handled in a single updateWidgets().
     */
    void slotStepStyleChanged(bool checked);

    void slotHelp();

private:
    Segment *m_segment;

    const ControlParameter &m_controlParameter;
    /// Use value or percent.
    /**
     * true: We're working with a controller like volume and we need to
     *       display control change values, not percents.
     * false: Pitchbend only.  Display percent.
     *
     * ??? Confusing.  Maybe just simplify to isPitchbend() or usePercent()
     *     both of which have the opposite meaning to this?
     * ??? rename: useValue()?
     */
    bool useTrueValues() const;

    const timeT m_startTime;
    const timeT m_endTime;
    /// ??? rename: getTimeSpan()
    double getElapsedSeconds() const;


    // Helpers for working with spinbox values.

    /// Convert pitch bend percent to a value delta.
    int percentToValueDelta(double percent) const;
    /// Get control value delta consistently even in percent (pitchbend) mode.
    /**
     * ??? rename: spinboxToValueDelta()?
     */
    int spinboxToControlDelta(const QDoubleSpinBox *spinbox) const;
    /// Get control value consistently even in percent (pitchbend) mode.
    /**
     * ??? rename: spinboxToValue()?
     */
    int spinboxToControl(const QDoubleSpinBox *spinbox) const;

    /// Convert pitch bend value delta to percent.
    double valueDeltaToPercent(int valueDelta) const;
    double getMinSpinboxValue() const;
    double getMaxSpinboxValue() const;
    double getSmallestSpinboxStep() const;


    /// Enable vibrato box only in Linear and StepCount modes.
    /**
     * ??? rename: updateVibratoWidgets()
     * ??? Fold this into an updateWidgets().
     */
    void maybeEnableVibratoFields();


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
    void saveSettings();
    void savePreset(int preset);
    void restorePreset(int preset);

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
    enum RampMode {
      Linear,
      Logarithmic,
      HalfSine,
      QuarterSine,
    };
    void setRampMode(RampMode rampMode);
    RampMode getRampMode();

    // How many steps

    // Use step size (%)
    QRadioButton *m_useStepSizePercent;
    QDoubleSpinBox *m_stepSize;
    // Use this many steps
    QRadioButton *m_useThisManySteps;
    QDoubleSpinBox *m_stepCount;
    enum StepSizeCalculation {  // ??? rename: StepMode
      StepSizeDirect,  // Use step size (%).  ??? rename: StepSizePercent
      StepSizeByCount,  // Use this many steps.  ??? rename: StepCount
    };
    void setStepSizeCalculation(StepSizeCalculation stepSizeCalculation);
    StepSizeCalculation getStepSizeCalculation();

    /// Generate EventInsertionCommand objects for the Linear/Step Size By Count case.
    void addLinearCountedEvents(MacroCommand *macro);
    /// Generate EventInsertionCommand objects for all other cases.
    void addStepwiseEvents(MacroCommand *macro);

};


}

#endif
