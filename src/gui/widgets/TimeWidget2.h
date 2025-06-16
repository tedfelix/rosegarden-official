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

#ifndef RG_TIMEWIDGET2_H
#define RG_TIMEWIDGET2_H

#include "base/RealTime.h"
#include "base/TimeT.h"

#include <QGroupBox>

class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QString;
class QTimer;
class QWidget;

#include <vector>


namespace Rosegarden
{


class Composition;


/// Absolute time and duration editing widget.
/**
 * This can be seen by double-clicking on a note (in notation or the matrix)
 * and then clicking the "Edit" button next to the absolute time or duration
 * fields.  That will launch the TimeDialog which consists mainly of a
 * TimeWidget2.
 */
class TimeWidget2 : public QGroupBox
{
    Q_OBJECT

public:

    /// Constructor for absolute time mode.
    /**
     * When is constrainToCompositionDuration false?
     * EditViewBase::slotSetSegmentStartTime() results in
     * constrainToCompositionDuration set to false.  This would be a good
     * test case for that.  There might be others that send a false
     * via TimeDialog for more test cases.
     * RosegardenMainWindow::slotSetSegmentStartTimes() looks like another.
     */
    TimeWidget2(const QString &title,
                QWidget *parent,
                Composition *composition, // for bar/beat/msec
                timeT initialTime,
                bool constrainToCompositionDuration);

    /// Constructor for duration mode.
    /**
     * startTime is the absolute time at which this duration begins.
     * startTime is needed to get the correct bar counts based on the current
     * time signature.  E.g. in 4/4, 3840 is one bar, in 2/4, 3840 is two bars.
     *
     * When is constrainToCompositionDuration false?
     * RosegardenMainWindow::slotRescaleSelection() results in
     * constrainToCompositionDuration set to false.  This would be a good
     * test case for that.  TimeDialog's clients might do the same.  Might
     * want to trace them back as well for more test cases.
     */
    TimeWidget2(const QString &title,
                QWidget *parent,
                Composition *composition, // for bar/beat/msec
                timeT startTime,
                timeT initialDuration,
                timeT minimumDuration,
                bool constrainToCompositionDuration);

    void setTime(timeT);
    /// Get the time in MIDI ticks.
    timeT getTime()  { return m_time; }

public slots:

    void slotResetToDefault();

private slots:

    /// Update m_time and the other fields.
    void slotNoteChanged(int);

    /// Update m_time and the other fields.
    void slotTicksChanged(int t);

    /// Update m_time and the other fields.
    void slotMeasureBeatOrFractionChanged(int);

    /// Update m_time and the other fields.
    void slotSecondsOrMSecChanged(int);

    void slotLimitClicked(bool);

private:

    Composition *m_composition;

    /// true for duration mode, false for absolute time mode.
    const bool m_isDuration;
    const bool m_constrainToCompositionDuration;
    /// For durations, this lets us know where we are in the Composition.
    const timeT m_startTime;
    /// Initial time in case we need to reset.
    const timeT m_defaultTime;
    /// When editing a duration, this is the smallest allowed.
    const timeT m_minimumDuration;

    /// The time or duration that is currently displayed on the widget.
    timeT m_time;
    /// Set m_time based on RealTime.
    void setRealTime(RealTime realTime);

    // Widgets

    /// Note field for duration mode.
    QComboBox *m_noteCombo;
    std::vector<timeT> m_noteDurations;
    /// Update the note widget from m_time.
    void updateNote();

    /// Ticks field.
    QSpinBox *m_ticksSpin;

    /// Update the ticks widget from m_time.
    void updateTicks();

    /// Measure/Measures field.
    QSpinBox *m_measureSpin;

    /// Beat/Beats field.
    QSpinBox *m_beatSpin;

    /// 64ths field.
    QSpinBox *m_fractionSpin;

    /// Time sig field that appears after 64ths (4/4 time).
    QLabel *m_timeSig;

    /// Update the measure, beat and 64ths widgets from m_time.
    void updateMeasureBeat64();

    /// Seconds field.
    QSpinBox *m_secondsSpin;
    /// msec field.
    QSpinBox *m_msecSpin;

    /// Update the seconds and msec fields from m_time.
    void updateSecondsMsec();

    /// Tempo field (e.g. 120.0 bpm) in the lower right in duration mode.
    QLabel *m_tempo;
    /// Update m_tempo from m_time.
    void updateTempo();

    /// Out Of Range message area.
    /**
     * Trying to set ranges on the various spin boxes based on limits results
     * in all sorts of frustrating complications for the user and the code.
     * This approach allows the user to enter whatever they want and easily
     * fix it if it is out of range.
     */
    QLabel *m_limitMessage;
    QPushButton *m_limitButton;
    timeT m_limit;
    void updateLimitWarning();

    /// Init code shared by the two ctors.
    void init();
    /// Copy from m_time to the widgets.
    void updateWidgets();

};


}

#endif
