
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

/// Though called PitchBendSequenceDialog for historical reasons, this
/// dialog deals with either pitchbend events or controller events,
/// according the "m_control" parameter.  It inserts, erases, or
/// replaces a series of such events in the a given segment.  It now
/// supplies the functionality for several menu items.

#ifndef RG_PITCHBENDSEQUENCEDIALOG_H
#define RG_PITCHBENDSEQUENCEDIALOG_H

#include <QDialog>

#include "base/Event.h"

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QRadioButton;

namespace Rosegarden
{

class ControlParameter;
class MacroCommand;
class Segment;

// @authors: Jani (original author?)
// Tom Breton (Tehom)
// Tim Munro
class PitchBendSequenceDialog : public QDialog
{
    Q_OBJECT

    enum PresetStyles {
      LinearRamp,
      FastVibratoArmRelease,
      Vibrato,
      EndBuiltIns,
    };
    enum ReplaceMode {
      OnlyAdd,   // Only add new events.
      Replace,   // Replace old controller events here with new ones.
      OnlyErase, // Just erase old controller events here.
    };
    enum RampMode {
      Linear,
      Logarithmic,
      HalfSine,
      QuarterSine,
    };
    enum StepSizeCalculation {
      StepSizeDirect,
      StepSizeByCount,
    };

 public:
    PitchBendSequenceDialog(QWidget *parent, Segment *segment,
			    const ControlParameter &control,
                            timeT startTime, timeT endTime);

public slots:
    virtual void accept();
    void slotSequencePresetChanged(int);
    void slotHelpRequested();

protected slots:
    void slotOnlyEraseClicked(bool checked);
    void slotLinearRampClicked(bool /*checked*/);
    void slotStepSizeStyleChanged(bool checked);
    
protected:
    /** Methods dealing with transforming to or from spinbox values **/

    bool useTrueValues(void) const;
    int spinboxToControl(const QDoubleSpinBox *spinbox) const;
    int spinboxToControlDelta(const QDoubleSpinBox *spinbox) const;
    double getMaxSpinboxValue(void) const;
    double getMinSpinboxValue(void) const;
    double getSmallestSpinboxStep(void) const;
    double valueDeltaToPercent(int valueDelta) const;
    int percentToValueDelta(double) const;
    double getElapsedSeconds(void);
    int numVibratoCycles(void);

    /** Methods dealing with setting and reading radiobutton groups **/

    ReplaceMode getReplaceMode(void);
    void setRampMode(RampMode rampMode);
    RampMode getRampMode(void);
    void setStepSizeCalculation(StepSizeCalculation stepSizeCalculation);
    StepSizeCalculation getStepSizeCalculation(void);

    /** Methods to help manage which widgets are enabled **/

    void maybeEnableVibratoFields(void);

    /** Methods dealing with saving/restoring presets **/

    void saveSettings(void);
    void savePreset(int preset);
    void restorePreset(int preset);
    
    /** Methods filling the macrocommand with commands **/

    void addLinearCountedEvents(MacroCommand *macro);
    void addStepwiseEvents(MacroCommand *macro);

    /**** Data members ****/
    
    Segment *m_segment;
    const ControlParameter &m_control;

    // How many of the built-in presets we are accepting.  In
    // practice, either 0 or EndBuiltIns.
    const int  m_numBuiltins;
  
    QDoubleSpinBox *m_prebendValue;
    QDoubleSpinBox *m_prebendDuration;
    QDoubleSpinBox *m_sequenceRampDuration;
    QDoubleSpinBox *m_sequenceEndValue;
    QDoubleSpinBox *m_stepSize;
    QDoubleSpinBox *m_resolution;
    QDoubleSpinBox *m_vibratoStartAmplitude;
    QDoubleSpinBox *m_vibratoEndAmplitude;
    QDoubleSpinBox *m_vibratoFrequency;
    QGroupBox      *m_vibratoBox;

    QComboBox *m_sequencePreset;

    /** ReplaceMode group **/

    QRadioButton *m_radioOnlyAdd;
    QRadioButton *m_radioReplace;
    QRadioButton *m_radioOnlyErase;

    /** StepSizeCalculation group **/

    QRadioButton *m_radioStepSizeDirect;
    QRadioButton *m_radioStepSizeByCount;

    /** RampMode group **/

    QRadioButton *m_radioRampLinear;
    QRadioButton *m_radioRampLogarithmic;
    QRadioButton *m_radioRampHalfSine;
    QRadioButton *m_radioRampQuarterSine;
    
    timeT m_startTime;
    timeT m_endTime;

};


}

#endif
