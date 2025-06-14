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

#define RG_MODULE_STRING "[TimeWidget2]"
#define RG_NO_DEBUG_PRINT

#include "TimeWidget2.h"

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


TimeWidget2::TimeWidget2(const QString &title,
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
    RG_DEBUG << "ctor for time mode";
    RG_DEBUG << "  m_constrainToCompositionDuration: " << m_constrainToCompositionDuration;

    init();
}

TimeWidget2::TimeWidget2(const QString &title,
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
    RG_DEBUG << "ctor for duration mode";
    RG_DEBUG << "  m_constrainToCompositionDuration: " << m_constrainToCompositionDuration;

    init();
}

void
TimeWidget2::init()
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
                this, &TimeWidget2::slotNoteChanged);
        layout->addWidget(m_noteCombo, row, 1, 1, 3);

        // Ticks
        labelWidget = new QLabel(tr("Ticks:"), this);
        labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(labelWidget, row, 4);

        m_ticksSpin = new QSpinBox(this);
        m_ticksSpin->setSingleStep(Note(Note::Shortest).getDuration());
        connect(m_ticksSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget2::slotTicksChanged);
        // ??? What abouyt QAbstractSpinBox::editingFinished()?  I suspect we
        //     need this to properly handle typing in the field and tabbing
        //     away.  Need to check this for all fields.
        layout->addWidget(m_ticksSpin, row, 5);

        ++row;

    } else {  // Absolute Time Mode

        // Only duration mode has the note combo.  Always check m_noteCombo
        // before using.
        m_noteCombo = nullptr;

    }

    // Measure/Measures
    labelWidget = new QLabel(
            m_isDuration ? tr("Measures:") : tr("Measure:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 0);

    m_measureSpin = new QSpinBox(this);
    if (m_isDuration)
        m_measureSpin->setMinimum(0);
    connect(m_measureSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget2::slotMeasureBeatOrFractionChanged);
    layout->addWidget(m_measureSpin, row, 1);

    // Beat/Beats
    labelWidget = new QLabel(m_isDuration ? tr("beats:") : tr("beat:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 2);

    m_beatSpin = new QSpinBox(this);
    m_beatSpin->setMinimum(1);
    connect(m_beatSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget2::slotMeasureBeatOrFractionChanged);
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
            this, &TimeWidget2::slotMeasureBeatOrFractionChanged);
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
            this, &TimeWidget2::slotSecondsOrMSecChanged);
    layout->addWidget(m_secondsSpin, row, 1);

    // msec
    labelWidget = new QLabel(tr("msec:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 2);

    m_msecSpin = new QSpinBox(this);
    m_msecSpin->setMinimum(0);
    m_msecSpin->setSingleStep(10);
    connect(m_msecSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget2::slotMSecChanged);
    layout->addWidget(m_msecSpin, row, 3);

    if (m_isDuration) {
        m_tempo = new QLabel(this);
        layout->addWidget(m_tempo, row, 6);
    } else {
        m_tempo = nullptr;
    }

    ++row;

    if (!m_isDuration) {

        // Ticks
        labelWidget = new QLabel(tr("Ticks:"), this);
        labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(labelWidget, row, 0);

        m_ticksSpin = new QSpinBox(this);
        m_ticksSpin->setSingleStep(Note(Note::Shortest).getDuration());
        connect(m_ticksSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget2::slotTicksChanged);
        layout->addWidget(m_ticksSpin, row, 1);
        layout->addWidget(new QLabel(tr("ticks"), this), row, 2);

    }

    updateWidgets();
}

void
TimeWidget2::updateWidgets()
{
    // Update all widgets from m_time and m_startTime.

    // ??? Split this big if into two routines?

    // Duration mode.
    if (m_isDuration) {

        // Limit duration to the Composition end.
        // ??? Shouldn't we only do this if m_constrainToCompositionDuration?
        //     Otherwise we might be cutting off a valid value.
        if (m_time + m_startTime > m_composition->getEndMarker())
            m_time = m_composition->getEndMarker() - m_startTime;

        // Ticks

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_ticksSpin->blockSignals(true);

        // ??? Shouldn't this be set to m_minimumDuration?
        // ??? Should we really allow durations of 0?  That causes problems
        //     in other parts of rg.  Actually, I suspect this never allows
        //     0 since the eventual update puts it back to 1.
        //     V RescaleDialog ctor - Handles it ok.  Becomes tiny.  Must
        //       catch it further down.
        //       - Zero is not valid here.
        //     V EditViewBase::slotSetSegmentDuration()
        //       - Zero is not valid here.
        //     V NoteWidget Duration catches this in its own spin box.
        //       - Zero is not valid here.
        //     -> NoteWidget notation duration?  Allows 0, but doesn't crash.
        //       - Zero is not valid here.
        //     -> RestWidget duration.  Zero is allowed in RestWidget.
        //       - Zero is not valid here.
        //     - RMW::slotCreateAnacrusis().  Can't do smaller than 60 ticks.
        //       - Looks like a minimum has been set.
        //       - Zero is not valid here.
        //     - RMW::slotInsertRange()
        //     - RMW::slotSetSegmentDurations()
        //     - TriggerSegmentManager::slotAdd()
        m_ticksSpin->setMinimum(0);
        m_ticksSpin->setMaximum(INT_MAX);

        m_ticksSpin->setValue(m_time);

        m_ticksSpin->blockSignals(false);

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

#if 0
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

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_measureSpin->blockSignals(true);

        m_measureSpin->setMinimum(0);
        m_measureSpin->setMaximum(9999);

        m_measureSpin->setValue(bars);

        m_measureSpin->blockSignals(false);

        // Beats
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_beatSpin->blockSignals(true);
        m_beatSpin->setMinimum(0);
        m_beatSpin->setMaximum(timeSig.getBeatsPerBar() - 1);
        m_beatSpin->setValue(beats);
        m_beatSpin->blockSignals(false);

        // 64ths
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_fractionSpin->blockSignals(true);
        m_fractionSpin->setMinimum(0);
        m_fractionSpin->setMaximum(
                timeSig.getBeatDuration() / Note(Note::Shortest).getDuration() - 1);
        m_fractionSpin->setValue(hemidemis);
        m_fractionSpin->blockSignals(false);
#endif

        updateMeasureBeat64();

        // Seconds

        // ??? This should take into account m_minimumDuration.  But it would
        //     need to coordinate with m_msecSpin.

        const timeT endTime = m_startTime + m_time;

        // Duration in seconds.
        RealTime realTime = m_composition->getRealTimeDifference(
                m_startTime, endTime);

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_secondsSpin->blockSignals(true);
        m_secondsSpin->setMinimum(0);
        // ??? This will be a problem for ticks since it can't go this high.
        m_secondsSpin->setMaximum(INT_MAX);
        m_secondsSpin->setValue(realTime.sec);
        m_secondsSpin->blockSignals(false);

        // msec

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_msecSpin->blockSignals(true);
        m_msecSpin->setMinimum(0);
        m_msecSpin->setMaximum(999);

        // Round value instead of direct read from rt.nsec.
        // Causes cycle of rounding between msec and ticks
        // which creates odd typing behavior.
        m_msecSpin->setValue(getRoundedMSec(realTime));
        m_msecSpin->blockSignals(false);

        // Tempo

        // ??? Tempo is a special field that never changes.  We just need
        //     to do this on init and leave it alone.  Same might be said for
        //     the m_timeSig field.  Might be able to just update that once
        //     at init.

        // Does the tempo change over the duration?  If so we'll include
        // the qualifier "starting".
        const bool change =
                (m_composition->getTempoChangeNumberAt(endTime) !=
                 m_composition->getTempoChangeNumberAt(m_startTime));
        QString starting;
        if (change)
            starting = tr("starting") + " ";  // As in "starting bpm".

        const TimeSignature timeSig =
                m_composition->getTimeSignatureAt(m_startTime);
        const double tempo = m_composition->getTempoQpm(
                m_composition->getTempoAtTime(m_startTime));
        QString qpm = QString::number(tempo, 'f', 2);

        QString text;
        // Crotchet is the beat?
        if (timeSig.getBeatDuration() == Note(Note::Crotchet).getDuration()) {
            // qpm is bpm
            text = "(" + starting + qpm + " " + tr("bpm") + ")";
        } else {
            const double bpmDouble = tempo *
                    Note(Note::Crotchet).getDuration() /
                    timeSig.getBeatDuration();
            const QString bpm = QString::number(bpmDouble, 'f', 2);

            // Had to move away from QString::arg() because it was dropping
            // characters when I did things like %1%2.  Bizarre.
            text = "(" + starting + qpm + " " + tr("qpm") + ", " + bpm + " " +
                    tr("bpm") + ")";
        }

        m_tempo->setText(text);

    } else {  // Absolute Time mode

        // Past the end?  Constrain.
        if (m_constrainToCompositionDuration  &&
            m_time >= m_composition->getEndMarker())
            m_time = m_composition->getEndMarker() - 1;

        // Past the beginning?  Constrain.
        if (m_constrainToCompositionDuration  &&
            m_time < m_composition->getStartMarker())
            m_time = m_composition->getStartMarker();

        // Measure

        int bar;
        int beat;
        int hemidemis;
        int remainder;
        m_composition->getMusicalTimeForAbsoluteTime(
                m_time, bar, beat, hemidemis, remainder);

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_measureSpin->blockSignals(true);
        m_measureSpin->setMinimum(INT_MIN);
        m_measureSpin->setMaximum(INT_MAX);
        m_measureSpin->setValue(bar + 1);
        m_measureSpin->blockSignals(false);

        // Beat
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_beatSpin->blockSignals(true);
        m_beatSpin->setMinimum(1);
        const TimeSignature timeSig =
                m_composition->getTimeSignatureAt(m_time);
        m_beatSpin->setMaximum(timeSig.getBeatsPerBar());
        m_beatSpin->setValue(beat);
        m_beatSpin->blockSignals(false);

        // 64ths
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_fractionSpin->blockSignals(true);
        m_fractionSpin->setMinimum(0);
        m_fractionSpin->setMaximum(timeSig.getBeatDuration() /
                                Note(Note::Shortest).
                                getDuration() - 1);
        m_fractionSpin->setValue(hemidemis);
        m_fractionSpin->blockSignals(false);

        // Time Signature
        m_timeSig->setText(tr("(%1/%2 time)")
	                   .arg(timeSig.getNumerator())
                           .arg(timeSig.getDenominator()));

        // Seconds

        RealTime realTime = m_composition->getElapsedRealTime(m_time);

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_secondsSpin->blockSignals(true);
        m_secondsSpin->setMinimum(INT_MIN);
        // ??? This will be a problem for ticks since it can't go this high.
        m_secondsSpin->setMaximum(INT_MAX);
        m_secondsSpin->setValue(realTime.sec);
        m_secondsSpin->blockSignals(false);

        // msec

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_msecSpin->blockSignals(true);

        // ??? We need special handling for negative secs.  We'll
        //     need to subtract this number instead of adding.
        m_msecSpin->setMinimum(0);
        m_msecSpin->setMaximum(999);

        // Round value instead of direct read from rt.nsec.
        // Causes cycle of rounding between msec and Time
        // which creates odd typing behavior.
        m_msecSpin->setValue(getRoundedMSec(realTime));
        m_msecSpin->blockSignals(false);

        // Ticks

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_ticksSpin->blockSignals(true);
        m_ticksSpin->setMinimum(INT_MIN);
        m_ticksSpin->setMaximum(INT_MAX);
        m_ticksSpin->setValue(m_time);
        m_ticksSpin->blockSignals(false);

    }

}

int
TimeWidget2::getRoundedMSec(RealTime rt)
{
    double msecDouble = rt.usec() / 1000.0;
    // ??? I think lround() is the same as this.  Check how it
    //     handles negatives.
    //       return lround(msecDouble);
    return rt.sec > 0 ? floor(msecDouble + 0.5) : ceil(msecDouble - 0.5);

}

void
TimeWidget2::setTime(timeT t)
{
    if (m_isDuration  &&  t < m_minimumDuration)
        t = m_minimumDuration;

    m_time = t;

    updateWidgets();
}

void
TimeWidget2::setRealTime(RealTime realTime)
{
    if (m_isDuration) {
        const RealTime startRT = m_composition->getElapsedRealTime(m_startTime);
        // ??? Yikes!  A duration of zero is a serious problem.  Didn't I
        //     fix this years ago?  Did I miss this one?
        if (realTime >= RealTime::zero()) {
            setTime(m_composition->getElapsedTimeForRealTime(startRT + realTime) - m_startTime);
        } else {
            RG_DEBUG << "setRealTime(): WARNING: realTime must be > 0 for duration widget (was " << realTime << ")";
        }
    } else {
        setTime(m_composition->getElapsedTimeForRealTime(realTime));
    }
}

void
TimeWidget2::slotResetToDefault()
{
    setTime(m_defaultTime);
}

void TimeWidget2::updateMeasureBeat64()
{
    // Duration mode.
    if (m_isDuration) {

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
        // ??? Should we cache this when we init m_timeSig and m_tempo?
        const TimeSignature timeSig =
                m_composition->getTimeSignatureAt(m_startTime);

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_measureSpin->blockSignals(true);
        m_measureSpin->setMinimum(0);
        m_measureSpin->setMaximum(9999);
        m_measureSpin->setValue(bars);
        m_measureSpin->blockSignals(false);

        // Beats
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_beatSpin->blockSignals(true);
        m_beatSpin->setMinimum(0);
        m_beatSpin->setMaximum(timeSig.getBeatsPerBar() - 1);
        m_beatSpin->setValue(beats);
        m_beatSpin->blockSignals(false);

        // 64ths
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_fractionSpin->blockSignals(true);
        m_fractionSpin->setMinimum(0);
        m_fractionSpin->setMaximum(
                timeSig.getBeatDuration() / Note(Note::Shortest).getDuration() - 1);
        m_fractionSpin->setValue(hemidemis);
        m_fractionSpin->blockSignals(false);

        // Time Signature
        // ??? m_timeSig is a special field that never changes.  We just need
        //     to do this on init and leave it alone.
        m_timeSig->setText(tr("(%1/%2 time)").
                arg(timeSig.getNumerator()).
                arg(timeSig.getDenominator()));

    } else {  // Absolute time mode
        // Convert m_time to bar/beat/fraction.

        // Update the widgets.
    }
}

void
TimeWidget2::slotNoteChanged(int n)
{
    // 0 is <inexact>.  Nothing we can do with that.
    if (n <= 0)
        return;
    if (n >= (int)m_noteDurations.size())
        return;

    // Update the other fields.
    m_time = m_noteDurations[n];
    updateMeasureBeat64();
    updateSecondsMsec();
    updateTicks();
    updateLimitWarning();
}

void
TimeWidget2::slotTicksChanged(int ticks)
{
    m_time = ticks;
    updateMeasureBeat64();
    updateNote();
    updateSecondsMsec();
    updateLimitWarning();
}

void
TimeWidget2::slotMeasureBeatOrFractionChanged(int)
{
    const int bar = m_measureSpin->value();
    const int beat = m_beatSpin->value();
    const int fraction = m_fractionSpin->value();

    if (m_isDuration) {
        setTime(m_composition->getDurationForMusicalTime(
                m_startTime, bar, beat, fraction, 0));
    } else {
        setTime(m_composition->getAbsoluteTimeForMusicalTime(
                bar, beat, fraction, 0));
    }
}

void
TimeWidget2::slotSecondsOrMSecChanged(int)
{
    // Update the rest of the fields based on the Seconds and msec fields.
    setRealTime(RealTime(m_secondsSpin->value(), m_msecSpin->value() * 1000000));
}

void
TimeWidget2::slotMSecChanged(int)
{
    // The msec field has changed.  Either from the user typing into it
    // or from the up/down arrows on the spin box.

    // Avoid duplicate connections.
    // ??? Isn't there a flag we can pass that will do that.  Yes.
    //     Qt::UniqueConnection
    disconnect(m_msecSpin, &QAbstractSpinBox::editingFinished,
               this, &TimeWidget2::slotMSecUpdate);
    // Since we are in the middle of a change to msec, we want to
    // be informed if the user tabs out of the field.
    connect(m_msecSpin, &QAbstractSpinBox::editingFinished,
            this, &TimeWidget2::slotMSecUpdate);

    // No need to monitor the user tabbing out of the Ticks field.  The
    // msec field is in play now.
    // This prevents an update if the user clicks on the Ticks field then
    // tabs out of it without making any changes.
    disconnect(m_ticksSpin, &QAbstractSpinBox::editingFinished,
               this, &TimeWidget2::slotTicksUpdate);

}

void
TimeWidget2::slotMSecUpdate()
{
    // Either the user has tabbed out of the msec field, or the
    // delayed update timer has gone off.  Update the rest of the fields
    // based on the Seconds and msec fields.

    // Perform an immediate update.  Arg is ignored.
    slotSecondsOrMSecChanged(0);
}


}
