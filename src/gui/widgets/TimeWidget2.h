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

    void slotNoteChanged(int);

    /// Restart the update delay timer and connect it for m_timeT.
    void slotTicksChanged(int t);

    /// Stop the delay timer and call slotSetTime(int)
    void slotTicksUpdate();

    void slotMeasureBeatOrFractionChanged(int);

    /// Determine realtime based on Sec or msec update and repopulate boxes.
    void slotSecondsOrMSecChanged(int);

    /// Restart the update delay timer and connect it to m_msec.
    void slotMSecChanged(int);

    /// Stop the delay timer and call slotSecOrMSecChanged(int)
    void slotMSecUpdate();

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

    /// Ticks field.
    QSpinBox *m_ticksSpin;

    /// Measure/Measures field.
    QSpinBox *m_measureSpin;

    /// Beat/Beats field.
    QSpinBox *m_beatSpin;

    /// 64ths field.
    QSpinBox *m_fractionSpin;

    void updateMeasureBeat64();

    /// Time sig field that appears after 64ths (4/4 time).
    QLabel *m_timeSig;

    /// Seconds field.
    QSpinBox *m_secondsSpin;
    /// msec field.
    QSpinBox *m_msecSpin;

    /// Tempo field (e.g. 120.0 bpm) in the lower right in duration mode.
    QLabel *m_tempo;

    /// Timer to fire off a delayed update.
    /**
     * To see this in action, use the up/down arrows in the spin box to change
     * the Ticks field and keep an eye on the msecs field.  It will update 1.5
     * seconds later.  Only the Ticks and msec fields use this.  All others
     * update immediately.
     *
     * ??? Why?  Why not just update it immediately?  I suspect this is to
     *     prevent the manic updating of all the fields while the user is
     *     typing into one.  E.g. if you double-click in the ticks field and
     *     slowly type in 52406, nothing will update until 1.5 seconds after
     *     you finish.  What is odd is that this is only implemented for the
     *     Ticks and msec fields.  Seems like it should be implemented for
     *     all fields, or not implemented at all.  E.g. if you enter a large
     *     measure number right now, you get the manic updates.
     * ??? If there is a minimumDuration, this timer is being used to update
     *     the offending field when it goes too low.  We should handle this
     *     with limits in the spin boxes, not with an update on a timer.
     *     This will be difficult with the msec field since its limit would
     *     need to depend on the Seconds field being 0.
     */
    //QTimer *m_delayUpdateTimer;

    /// Init code shared by the two ctors.
    void init();
    /// Copy from m_time to the widgets.
    void updateWidgets();
    /// Return a rounded msec reading from the given realTime argument.
    static int getRoundedMSec(RealTime rt);

};


}

#endif
