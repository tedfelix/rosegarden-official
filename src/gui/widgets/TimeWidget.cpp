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

#define RG_MODULE_STRING "[TimeWidget]"

#include "TimeWidget.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPixmap>
#include <QSpinBox>
#include <QString>
#include <QGridLayout>
#include <QTimer>


namespace Rosegarden
{


namespace
{
    constexpr int UPDATE_DELAY_TIME = 1500;  // msecs
}


TimeWidget::TimeWidget(const QString &title,
                       QWidget *parent,
                       Composition *composition,
                       timeT initialTime,
                       bool constrainToCompositionDuration) :
    QGroupBox(title, parent),
    m_composition(composition),
    m_isDuration(false),
    m_constrainToCompositionDuration(constrainToCompositionDuration),
    m_startTime(0),
    m_defaultTime(initialTime),
    m_minimumDuration(0),  // Unused in absolute time mode.
    m_time(initialTime)
{
    init();
}

TimeWidget::TimeWidget(const QString& title,
                       QWidget *parent,
                       Composition *composition,
                       timeT startTime,
                       timeT initialDuration,
                       timeT minimumDuration,
                       bool constrainToCompositionDuration) :
    QGroupBox(title, parent),
    m_composition(composition),
    m_isDuration(true),
    m_constrainToCompositionDuration(constrainToCompositionDuration),
    m_startTime(startTime),
    m_defaultTime(initialDuration),
    m_minimumDuration(minimumDuration),
    m_time(initialDuration)
{
    init();
}

void
TimeWidget::init()
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(5);
    int row{0};

    QLabel *labelWidget = nullptr;

    // Duration Mode
    if (m_isDuration) {

        // Note
        labelWidget = new QLabel(tr("Note:"), this);
        labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(labelWidget, row, 0);

        m_noteCombo = new QComboBox(this);
        m_noteDurations.push_back(0);
        m_noteCombo->addItem(tr("<inexact>"));

        // Divisors of a breve.
        const std::vector<int> divisors = {
            1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128
        };

        // For each divisor, and an entry to m_noteCombo.
        for (const int divisor : divisors) {

            const timeT duration =
                    Note(Note::Breve).getDuration() / divisor;

            // If not breve or hemidemi, not a triplet, add dotted duration.
            if (divisor > 1  &&  divisor < 128  &&  (divisor % 3) != 0) {
                const timeT dottedDuration = duration * 3 / 2;
                m_noteDurations.push_back(dottedDuration);
                // Ignored
                timeT error = 0;
                const QString text = NotationStrings::makeNoteMenuLabel(
                        dottedDuration, false, error);
                const QPixmap pixmap = NotePixmapFactory::makeNoteMenuPixmap(
                        dottedDuration, error);

                m_noteCombo->addItem(pixmap, text);
            }

            m_noteDurations.push_back(duration);
            // Ignored
            timeT error = 0;
            const QString text = NotationStrings::makeNoteMenuLabel(
                    duration, false, error);
            const QPixmap pixmap = NotePixmapFactory::makeNoteMenuPixmap(
                    duration, error);

            m_noteCombo->addItem(pixmap, text);
        }

        connect(m_noteCombo,
                    static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                this, &TimeWidget::slotNoteChanged);
        layout->addWidget(m_noteCombo, row, 1, 1, 3);

        // Units
        labelWidget = new QLabel(tr("Units:"), this);
        labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(labelWidget, row, 4);

        m_timeSpin = new QSpinBox(this);
        m_timeSpin->setSingleStep(Note(Note::Shortest).getDuration());
        connect(m_timeSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget::slotTimeTChanged);
        layout->addWidget(m_timeSpin, row, 5);

    } else {  // Absolute Time Mode

        // Only duration mode has the note combo.  Always check m_noteCombo
        // before using.
        m_noteCombo = nullptr;

        // Time
        labelWidget = new QLabel(tr("Time:"), this);
        labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(labelWidget, row, 0);

        m_timeSpin = new QSpinBox(this);
        m_timeSpin->setSingleStep(Note(Note::Shortest).getDuration());
        connect(m_timeSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget::slotTimeTChanged);
        layout->addWidget(m_timeSpin, row, 1);
        layout->addWidget(new QLabel(tr("units"), this), row, 2);

    }

    ++row;

    // Measure/Measures
    labelWidget = new QLabel(
            m_isDuration ? tr("Measures:") : tr("Measure:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 0);

    m_measureSpin = new QSpinBox(this);
    if (m_isDuration)
        m_measureSpin->setMinimum(0);
    connect(m_measureSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget::slotBarBeatOrFractionChanged);
    layout->addWidget(m_measureSpin, row, 1);

    // Beat/Beats
    labelWidget = new QLabel(m_isDuration ? tr("beats:") : tr("beat:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 2);

    m_beatSpin = new QSpinBox(this);
    m_beatSpin->setMinimum(1);
    connect(m_beatSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget::slotBarBeatOrFractionChanged);
    layout->addWidget(m_beatSpin, row, 3);

    // 64ths
    labelWidget = new QLabel(tr("%1:").arg(NotationStrings::getShortNoteName(
            Note(Note::Shortest),  // note
            true)),  // plural
            this);  // parent
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 4);

    m_fractionSpin = new QSpinBox(this);
    m_fractionSpin->setMinimum(1);
    connect(m_fractionSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget::slotBarBeatOrFractionChanged);
    layout->addWidget(m_fractionSpin, row, 5);

    // Time Signature (e.g. 4/4 time)
    m_timeSig = new QLabel(this);
    layout->addWidget(m_timeSig, row, 6);

    ++row;

    // Seconds
    labelWidget = new QLabel(tr("Seconds:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 0);

    m_secondsSpin = new QSpinBox(this);
    if (m_isDuration)
        m_secondsSpin->setMinimum(0);
    connect(m_secondsSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget::slotSecOrMSecChanged);
    layout->addWidget(m_secondsSpin, row, 1);

    // msec
    labelWidget = new QLabel(tr("msec:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 2);

    m_msecSpin = new QSpinBox(this);
    m_msecSpin->setMinimum(0);
    m_msecSpin->setSingleStep(10);
    connect(m_msecSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget::slotMSecChanged);
    layout->addWidget(m_msecSpin, row, 3);

    if (m_isDuration) {
        m_tempo = new QLabel(this);
        layout->addWidget(m_tempo, row, 6);
    } else {
        m_tempo = nullptr;
    }

    populate();

    m_delayUpdateTimer = new QTimer(this);
    m_delayUpdateTimer->setSingleShot(true);
}

void
TimeWidget::populate()
{
    // Update all widgets from m_time and m_startTime.

    // ??? Get rid of these blockSignals() calls!

    if (m_noteCombo)
        m_noteCombo->blockSignals(true);

    m_timeSpin->blockSignals(true);
    m_measureSpin->blockSignals(true);
    m_beatSpin->blockSignals(true);
    m_fractionSpin->blockSignals(true);
    m_secondsSpin->blockSignals(true);
    m_msecSpin->blockSignals(true);

    // Duration mode.
    if (m_isDuration) {

        // Limit duration to the Composition end.
        // ??? Shouldn't we only do this if m_constrainToCompositionDuration?
        //     Otherwise we might be cutting off a valid value.
        if (m_time + m_startTime > m_composition->getEndMarker())
            m_time = m_composition->getEndMarker() - m_startTime;

        // Units

        // ??? Should we really allow durations of 0?  That causes problems
        //     in other parts of rg.
        m_timeSpin->setMinimum(0);

        if (m_constrainToCompositionDuration)
            m_timeSpin->setMaximum(m_composition->getEndMarker() - m_startTime);
        else
            m_timeSpin->setMaximum(INT_MAX);

        m_timeSpin->setValue(m_time);

        // ??? We're in duration mode.  Don't we always have a m_noteCombo
        //     in that case?  This might be a leftover read-only check.
        //     Remove this "if".
        if (m_noteCombo) {
            m_noteCombo->setCurrentIndex(0);
            // Set the note combo to the appropriate value if found.
            for (size_t i = 0; i < m_noteDurations.size(); ++i) {
                if (m_time == m_noteDurations[i]) {
                    m_noteCombo->setCurrentIndex(i);
                    break;
                }
            }
        }

        // Measures

        // The bar/beat etc timings are considered to be times of a note
        // starting at the start of a bar, in the time signature in effect
        // at m_startTime.

        int bars;
        int beats;
        int hemidemis;
        int remainder;
        m_composition->getMusicalTimeForDuration(
                m_startTime, m_time, bars, beats, hemidemis, remainder);
        const TimeSignature timeSig =
                m_composition->getTimeSignatureAt(m_startTime);

        m_measureSpin->setMinimum(0);

        if (m_constrainToCompositionDuration)
            m_measureSpin->setMaximum(
                    m_composition->getBarNumber(m_composition->getEndMarker()) -
                    m_composition->getBarNumber(m_startTime));
        else
            m_measureSpin->setMaximum(9999);

        m_measureSpin->setValue(bars);

        // Beats
        m_beatSpin->setMinimum(0);
        m_beatSpin->setMaximum(timeSig.getBeatsPerBar() - 1);
        m_beatSpin->setValue(beats);

        // 64ths
        m_fractionSpin->setMinimum(0);
        m_fractionSpin->setMaximum(
                timeSig.getBeatDuration() / Note(Note::Shortest).getDuration() - 1);
        m_fractionSpin->setValue(hemidemis);

        // Time Signature
        m_timeSig->setText(tr("(%1/%2 time)")
			   .arg(timeSig.getNumerator())
                           .arg(timeSig.getDenominator()));

        // Seconds

        const timeT endTime = m_startTime + m_time;

        // Duration in seconds.
        RealTime realTime = m_composition->getRealTimeDifference(
                m_startTime, endTime);

        m_secondsSpin->setMinimum(0);
        if (m_constrainToCompositionDuration) {
            m_secondsSpin->setMaximum(m_composition->getRealTimeDifference(
                    m_startTime, m_composition->getEndMarker()).sec);
        } else {
            m_secondsSpin->setMaximum(9999);
        }
        m_secondsSpin->setValue(realTime.sec);

        // msec
        m_msecSpin->setMinimum(0);
        m_msecSpin->setMaximum(999);

        // Round value instead of direct read from rt.nsec.
        // Causes cycle of rounding between msec and units
        // which creates odd typing behavior.
        m_msecSpin->setValue(getRoundedMSec(realTime));

        // Tempo

        // Does the tempo change over the duration?
        const bool change =
                (m_composition->getTempoChangeNumberAt(endTime) !=
                 m_composition->getTempoChangeNumberAt(m_startTime));

        // imprecise -- better to work from tempoT directly
        const double tempo = m_composition->getTempoQpm(
                m_composition->getTempoAtTime(m_startTime));

        // ??? lround()?
        const int qpmCents = int(tempo * 100.0);
        int bpmCents = qpmCents;
        // Crotchet is not the beat?
        if (timeSig.getBeatDuration() != Note(Note::Crotchet).getDuration()) {
            // ??? lround()?
            bpmCents = int(tempo * 100.0 *
                       Note(Note::Crotchet).getDuration() /
                       timeSig.getBeatDuration());
        }

        // If the tempo changes over the duration...
        // ??? All we seem to be doing here is adding the word "starting" to
        //     the front.  We can do that more simply.
        // ??? Formatting looks wrong.  E.g. this will display 100.01 as 100.1.
        //     Recommend just formatting the double:
        //       arg(qpmCents / 100.0, 0, 'f', 2)
        if (change) {
            if (bpmCents != qpmCents) {
                m_tempo->setText(tr("(starting %1.%2 qpm, %3.%4 bpm)")
                                 .arg(qpmCents / 100)
                                 .arg(qpmCents % 100)
                                 .arg(bpmCents / 100)
                                 .arg(bpmCents % 100));
            } else {
                m_tempo->setText(tr("(starting %1.%2 bpm)")
                                 .arg(bpmCents / 100)
                                 .arg(bpmCents % 100));
            }
        } else {  // Tempo is fixed over the duration.
            if (bpmCents != qpmCents) {
                m_tempo->setText(tr("(%1.%2 qpm, %3.%4 bpm)")
                                 .arg(qpmCents / 100)
                                 .arg(qpmCents % 100)
                                 .arg(bpmCents / 100)
                                 .arg(bpmCents % 100));
            } else {
                m_tempo->setText(tr("(%1.%2 bpm)")
                                 .arg(bpmCents / 100)
                                 .arg(bpmCents % 100));
            }
        }

    } else {  // Absolute Time mode

        // Past the end?  Constrain.
        if (m_constrainToCompositionDuration  &&
            m_time > m_composition->getEndMarker())
            m_time = m_composition->getEndMarker();

        // Past the beginning?  Constrain.
        if (m_constrainToCompositionDuration  &&
            m_time < m_composition->getStartMarker())
            m_time = m_composition->getStartMarker();

        // Time

        if (m_constrainToCompositionDuration) {
            m_timeSpin->setMinimum(m_composition->getStartMarker());
            m_timeSpin->setMaximum(m_composition->getEndMarker());
        } else {
            m_timeSpin->setMinimum(INT_MIN);
            m_timeSpin->setMaximum(INT_MAX);
        }

        m_timeSpin->setValue(m_time);

        int bar;
        int beat;
        int hemidemis;
        int remainder;
        m_composition->getMusicalTimeForAbsoluteTime(
                m_time, bar, beat, hemidemis, remainder);

        const TimeSignature timeSig =
                m_composition->getTimeSignatureAt(m_time);

        m_measureSpin->setMinimum(INT_MIN);
        if (m_constrainToCompositionDuration) {
            m_measureSpin->setMaximum(m_composition->getBarNumber
                               (m_composition->getEndMarker()));
        } else {
            m_measureSpin->setMaximum(9999);
        }
        m_measureSpin->setValue(bar + 1);

        m_beatSpin->setMinimum(1);
        m_beatSpin->setMaximum(timeSig.getBeatsPerBar());
        m_beatSpin->setValue(beat);

        m_fractionSpin->setMinimum(0);
        m_fractionSpin->setMaximum(timeSig.getBeatDuration() /
                                Note(Note::Shortest).
                                getDuration() - 1);
        m_fractionSpin->setValue(hemidemis);

        m_timeSig->setText(tr("(%1/%2 time)")
	                   .arg(timeSig.getNumerator())
                           .arg(timeSig.getDenominator()));

        RealTime rt = m_composition->getElapsedRealTime(m_time);

        m_secondsSpin->setMinimum(INT_MIN);
        if (m_constrainToCompositionDuration) {
            m_secondsSpin->setMaximum(m_composition->getElapsedRealTime
                               (m_composition->getEndMarker()).sec);
        } else {
            m_secondsSpin->setMaximum(9999);
        }
        m_secondsSpin->setValue(rt.sec);

        if (m_time >= 0)
        {
            m_msecSpin->setMinimum(0);
            m_msecSpin->setMaximum(999);
        }
        else
        {
            m_msecSpin->setMinimum(-999);
            m_msecSpin->setMaximum(0);
        }

        // Round value instead of direct read from rt.msec
        // Causes cycle of rounding between msec and units
        // which creates odd typing behavior
        m_msecSpin->setValue(getRoundedMSec(rt));
    }

    if (m_noteCombo)
        m_noteCombo->blockSignals(false);

    m_timeSpin->blockSignals(false);
    m_measureSpin->blockSignals(false);
    m_beatSpin->blockSignals(false);
    m_fractionSpin->blockSignals(false);
    m_secondsSpin->blockSignals(false);
    m_msecSpin->blockSignals(false);
}

timeT
TimeWidget::getTime()
{
    return m_time;
}

int
TimeWidget::getRoundedMSec(RealTime rt)
{
    double msecDouble = rt.usec() / 1000.0;
    // ??? I think lround() is the same as this.  Check how it
    //     handles negatives.
    //       return lround(msecDouble);
    return rt.sec > 0 ? floor(msecDouble + 0.5) : ceil(msecDouble - 0.5);

}

void
TimeWidget::setTime(timeT t)
{
    if (m_isDuration  &&  t < m_minimumDuration)
        t = m_minimumDuration;

    m_time = t;

    populate();
}

void
TimeWidget::setRealTime(RealTime realTime)
{
    if (m_isDuration) {
        RealTime startRT = m_composition->getElapsedRealTime(m_startTime);
        // ??? Yikes!  A duration of zero is a serious problem.  Didn't I
        //     fix this years ago?  Did I miss this one?
        if (realTime >= RealTime::zero()) {
            setTime(m_composition->getElapsedTimeForRealTime(startRT + realTime) - m_startTime);
        } else {
            RG_DEBUG << "WARNING: TimeWidget::slotSetRealTime: rt must be >0 for duration widget (was " << realTime << ")";
        }
    } else {
        setTime(m_composition->getElapsedTimeForRealTime(realTime));
    }
}

void
TimeWidget::slotResetToDefault()
{
    setTime(m_defaultTime);
}

void
TimeWidget::slotNoteChanged(int n)
{
    if (n > 0) {
        setTime(m_noteDurations[n]);
    }
}

void
TimeWidget::slotTimeTChanged(int t)
{
    RG_DEBUG << "slotTimeTChanged: t is " << t << ", value is " << m_timeSpin->value();

    // The Time field has changed.  Either from the user typing into it or
    // from the up/down arrows on the spin box.

    // No timer?  Bail.
    if (!m_delayUpdateTimer)
        return;

    // Avoid duplicate connections.
    // ??? Isn't there a flag we can pass that will do that.  Yes.
    //     Qt::UniqueConnection
    disconnect(m_timeSpin, &QAbstractSpinBox::editingFinished,
               this, &TimeWidget::slotTimeTUpdate);
    // Since we are in the middle of a change to Time, we want to
    // be informed if the user tabs out of the field.
    connect(m_timeSpin, &QAbstractSpinBox::editingFinished,
            this, &TimeWidget::slotTimeTUpdate);

    // No need to monitor the user tabbing out of the msec field.  The
    // Time field is in play now.
    // This prevents an update if the user clicks on the msec field then
    // tabs out of it without making any changes.
    disconnect(m_msecSpin, &QAbstractSpinBox::editingFinished,
               this, &TimeWidget::slotMSecUpdate);

    // Delay Update Timer.
    //   - Connect it to slotTimeTUpdate().
    //   - Restart it.

    // Stop the timer since we are going to restart it for the new connection.
    // ??? Necessary?
    m_delayUpdateTimer->stop();

    // Disconnect the timer from everything to avoid duplicate connections
    // and to make sure we will only be connected to slotTimeTUpdate().
    m_delayUpdateTimer->disconnect();

    // Connect the timer to slotTimeTUpdate().
    connect(m_delayUpdateTimer, &QTimer::timeout,
            this, &TimeWidget::slotTimeTUpdate);

    m_delayUpdateTimer->start(UPDATE_DELAY_TIME);
}

void
TimeWidget::slotTimeTUpdate()
{
    // May have fired already, but stop it in case called when widget lost
    // focus.
    m_delayUpdateTimer->stop();

    // Perform an immediate update.
    setTime(m_timeSpin->value());
}

void
TimeWidget::slotBarBeatOrFractionChanged(int)
{
    int bar = m_measureSpin->value();
    int beat = m_beatSpin->value();
    int fraction = m_fractionSpin->value();

    if (m_isDuration) {
        setTime(m_composition->getDurationForMusicalTime
                    (m_startTime, bar, beat, fraction, 0));

    } else {
        setTime(m_composition->getAbsoluteTimeForMusicalTime
                    (bar, beat, fraction, 0));
    }
}

void
TimeWidget::slotSecOrMSecChanged(int)
{
    int sec = m_secondsSpin->value();
    int msec = m_msecSpin->value();

    setRealTime(RealTime(sec, msec * 1000000));
}

void
TimeWidget::slotMSecChanged(int)
{
    // ??? See slotTimeTChanged() which has more helpful comments and sync this
    //     with that.

    m_delayUpdateTimer->stop();

    // Disconnect any signals from timer
    m_delayUpdateTimer->disconnect();

    // Disconnect signals from m_timeT
    disconnect(m_timeSpin, &QAbstractSpinBox::editingFinished,
               this, &TimeWidget::slotTimeTUpdate);

    // Disconnect signals from m_msec
    disconnect(m_msecSpin, &QAbstractSpinBox::editingFinished,
               this, &TimeWidget::slotMSecUpdate);
    connect(m_msecSpin, &QAbstractSpinBox::editingFinished,
            this, &TimeWidget::slotMSecUpdate);

    connect(m_delayUpdateTimer, &QTimer::timeout,
            this, &TimeWidget::slotMSecUpdate);

    m_delayUpdateTimer->start(UPDATE_DELAY_TIME);
}

void
TimeWidget::slotMSecUpdate()
{
    // May have fired already, but stop it in case called when widget lost
    // focus.
    m_delayUpdateTimer->stop();

    // Perform an immediate update.
    slotSecOrMSecChanged(0); // Doesn't matter the value of argument.
}


}
