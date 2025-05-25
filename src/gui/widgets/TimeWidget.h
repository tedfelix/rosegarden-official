
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

#ifndef RG_TIMEWIDGET_H
#define RG_TIMEWIDGET_H

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
class LineEdit;


/// Absolute time and duration editing widget.
/**
 * This can be seen by double-clicking on a note (in notation or the matrix)
 * and then clicking the "Edit" button next to the absolute time or duration
 * fields.  That will launch the TimeDialog which consists mainly of a
 * TimeWidget.
 */
class TimeWidget : public QGroupBox
{
    Q_OBJECT

public:

    /// Constructor for absolute time widget.
    TimeWidget(const QString &title,
               QWidget *parent,
               Composition *composition, // for bar/beat/msec
               timeT initialTime,
               bool editable = true,
               bool constrainToCompositionDuration = true);

    /// Constructor for duration widget.
    /**
     * startTime is the absolute time at which this duration begins.
     *
     * startTime is needed to get the correct bar counts based on the current
     * time signature.  E.g. in 4/4, 3840 is one bar, in 2/4, 3840 is two bars.
     */
    TimeWidget(const QString &title,
               QWidget *parent,
               Composition *composition, // for bar/beat/msec
               timeT startTime,
               timeT initialDuration,
               timeT minimumDuration,
               bool editable = true,
               bool constrainToCompositionDuration = true);

    /// Get the time in MIDI clocks.
    timeT getTime();
    /// Get the time in seconds.
    RealTime getRealTime();

//signals:
    // ??? No one appears to ever connect to this signal.
    //void timeChanged(timeT);
    // ??? No one appears to ever connect to this signal.
    //void realTimeChanged(RealTime);

public slots:

    // ??? This is never used as a slot.  Rename and move to public.
    void slotSetTime(timeT);

    void slotResetToDefault();

private slots:

    // ??? This is never used as a slot.  Move to private.
    void slotSetRealTime(RealTime);

    void slotNoteChanged(int);

    /// Restart the update delay timer and connect it for m_timeT.
    void slotTimeTChanged(int);

    /// Stop the delay timer and call slotSetTime(int)
    void slotTimeTUpdate();

    void slotBarBeatOrFractionChanged(int);

    /// Determine realtime based on Sec or msec update and repopulate boxes.
    void slotSecOrMSecChanged(int);

    /// Restart the update delay timer and connect it to m_msec.
    void slotMSecChanged(int);

    /// Stop the delay timer and call slotSecOrMSecChanged(int)
    void slotMSecUpdate();

private:

    Composition *m_composition;

    /// true for duration mode, false for absolute time mode.
    const bool m_isDuration;
    bool m_constrainToCompositionDuration;
    /// For durations, this lets us know where we are in the Composition.
    timeT m_startTime;
    /// Initial time in case we need to reset.
    const timeT m_defaultTime;
    /// When editing a duration, this is the smallest allowed.
    const timeT m_minimumDuration;

    /// The time or duration that is currently displayed on the widget.
    timeT m_time;

    // Widgets

    QComboBox *m_note;
    /// Time field for editable absolute time mode.
    QSpinBox *m_timeSpin;

    /// Measure/Measures field when editable.
    QSpinBox *m_measureSpin;
    /// Measure/Measures field when not editable.
    /**
     * ??? Why LineEdit instead of QLabel?
     * ??? Why do we need a member if we can't edit it?
     */
    LineEdit *m_barLabel;

    /// Beat/Beats field when editable.
    QSpinBox *m_beatSpin;
    /// Beat/Beats field when not editable.
    /**
     * ??? Why LineEdit instead of QLabel?
     * ??? Why do we need a member if we can't edit it?
     */
    LineEdit *m_beatLabel;

    /// 64ths field when editable.
    QSpinBox *m_fractionSpin;
    /// 64ths field when not editable.
    /**
     * ??? Why LineEdit instead of QLabel?
     * ??? Why do we need a member if we can't edit it?
     */
    LineEdit *m_fractionLabel;
    /// Time sig field that appears after 64ths (4/4 time).
    QLabel *m_timeSig;

    /// Seconds field when editable.
    QSpinBox *m_secondsSpin;
    /// Seconds field when not editable.
    /**
     * ??? Why LineEdit instead of QLabel?
     * ??? Why do we need a member if we can't edit it?
     */
    LineEdit *m_secondsLabel;
    /// msec field when editable.
    QSpinBox *m_msecSpin;
    /// msec field when not editable.
    /**
     * ??? Why LineEdit instead of QLabel?
     * ??? Why do we need a member if we can't edit it?
     */
    LineEdit *m_msecLabel;

    // Duration only.
    QLabel *m_tempo;

    /// Timer to fire off a delayed update.
    /**
     * To see this in action, use the up/down arrows in the spin box to change
     * the Time field and keep an eye on the msecs field.  It will update 1.5
     * seconds later.  Only the Time and msec fields use this.  All others
     * update immediately.
     *
     * ??? Why?  Why not just update it immediately?  I suspect this is to
     *     prevent the manic updating of all the fields while the user is
     *     typing into one.  E.g. if you double-click in the time field and
     *     slowly type in 52406, nothing will update until 1.5 seconds after
     *     you finish.  What is odd is that this is only implemented for the
     *     Time and msec fields.  Seems like it should be implemented for
     *     all fields, or not implemented at all.
     */
    QTimer *m_delayUpdateTimer;

    void init(bool editable);
    /// Copy from m_time to the widgets.
    void populate();
    /// Return a rounded msec reading from the given realTime argument.
    static int getRoundedMSec(RealTime rt);

    std::vector<timeT> m_noteDurations;
};


}

#endif
