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
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QGridLayout>
#include <QTimer>


namespace Rosegarden
{


namespace
{

    // Message displayed when the time value is outside the valid time range.
    const QString outOfRange = TimeWidget2::tr("Out Of Range");

}

TimeWidget2::TimeWidget2(const QString &title,
                         QWidget *parent,
                         timeT initialTime,
                         bool constrainToCompositionDuration) :
    QGroupBox(title, parent),
    m_isDuration(false),
    m_constrainToCompositionDuration(constrainToCompositionDuration),
    m_startTime(0),
    m_defaultTime(initialTime),
    m_minimumDuration(0),  // Unused in absolute time mode.
    m_time(initialTime)
{
    init();
}

TimeWidget2::TimeWidget2(const QString &title,
                         QWidget *parent,
                         timeT startTime,
                         timeT initialDuration,
                         timeT minimumDuration,
                         bool constrainToCompositionDuration) :
    QGroupBox(title, parent),
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
TimeWidget2::init()
{
    m_composition = &RosegardenDocument::currentDocument->getComposition();

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
        m_ticksSpin->setMinimum(0);
        m_ticksSpin->setMaximum(INT_MAX);
        m_ticksSpin->setSingleStep(Note(Note::Shortest).getDuration());
        connect(m_ticksSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget2::slotTicksChanged);
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
    else
        m_measureSpin->setMinimum(INT_MIN);
    m_measureSpin->setMaximum(INT_MAX);
    connect(m_measureSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget2::slotMeasureBeatOrFractionChanged);
    layout->addWidget(m_measureSpin, row, 1);

    // Beat/Beats
    labelWidget = new QLabel(m_isDuration ? tr("beats:") : tr("beat:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 2);

    // In duration mode, the time signature does not change, so we can do
    // some initialization based on it.
    const TimeSignature timeSig =
            m_composition->getTimeSignatureAt(m_startTime);

    m_beatSpin = new QSpinBox(this);
    if (m_isDuration) {
        m_beatSpin->setMinimum(0);
        m_beatSpin->setMaximum(timeSig.getBeatsPerBar() - 1);
    } else {
        m_beatSpin->setMinimum(1);
    }
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
    // Always 0 to n-1.
    m_fractionSpin->setMinimum(0);
    if (m_isDuration) {
        m_fractionSpin->setMaximum(
                timeSig.getBeatDuration() / Note(Note::Shortest).getDuration() - 1);
    }
    connect(m_fractionSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget2::slotMeasureBeatOrFractionChanged);
    layout->addWidget(m_fractionSpin, row, 5);

    // Time Signature (e.g. 4/4 time)
    m_timeSig = new QLabel(this);
    if (m_isDuration) {
        // Time Signature
        m_timeSig->setText(tr("(%1/%2 time)").
                arg(timeSig.getNumerator()).
                arg(timeSig.getDenominator()));
    }
    layout->addWidget(m_timeSig, row, 6);

    ++row;

    // Seconds
    labelWidget = new QLabel(tr("Seconds:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 0);

    m_secondsSpin = new QSpinBox(this);
    if (m_isDuration)
        m_secondsSpin->setMinimum(0);
    else
        m_secondsSpin->setMinimum(INT_MIN);
    m_secondsSpin->setMaximum(INT_MAX);
    connect(m_secondsSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget2::slotSecondsOrMSecChanged);
    layout->addWidget(m_secondsSpin, row, 1);

    // msec
    labelWidget = new QLabel(tr("msec:"), this);
    labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(labelWidget, row, 2);

    m_msecSpin = new QSpinBox(this);
    // Set for duration mode.  Time mode will change this as needed.
    // See updateSecondsMsec().
    m_msecSpin->setMinimum(0);
    m_msecSpin->setMaximum(999);
    m_msecSpin->setSingleStep(10);
    connect(m_msecSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
            this, &TimeWidget2::slotSecondsOrMSecChanged);
    layout->addWidget(m_msecSpin, row, 3);

    if (m_isDuration) {
        m_tempo = new QLabel(this);
        layout->addWidget(m_tempo, row, 6);
    } else {
        m_tempo = nullptr;
    }

    ++row;

    // Absolute Time mode.
    if (!m_isDuration) {

        // Ticks
        labelWidget = new QLabel(tr("Ticks:"), this);
        labelWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(labelWidget, row, 0);

        m_ticksSpin = new QSpinBox(this);
        m_ticksSpin->setMinimum(INT_MIN);
        m_ticksSpin->setMaximum(INT_MAX);
        m_ticksSpin->setSingleStep(Note(Note::Shortest).getDuration());
        connect(m_ticksSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget2::slotTicksChanged);
        layout->addWidget(m_ticksSpin, row, 1);
        layout->addWidget(new QLabel(tr("ticks"), this), row, 2);

        ++row;

    }

    // Make a widget for the Out Of Range area so that it can have its
    // own horizontal layout and spacing independent of the other widgets.
    QWidget *widget = new QWidget(this);
    layout->addWidget(widget, row, 0, 1, 7);
    QHBoxLayout *hBox = new QHBoxLayout(widget);
    hBox->setContentsMargins(0,0,0,0);

    // Center the widgets.
    hBox->insertStretch(-1, 1);

    // Out Of Range and Limit button
    m_limitMessage = new QLabel("", widget);
    // Set a minimum width so that the widgets don't bounce around.
    int width = fontMetrics().boundingRect(outOfRange).width() + 2;
    m_limitMessage->setMinimumWidth(width);
    hBox->addWidget(m_limitMessage);
    m_limitButton = new QPushButton(tr("Limit"), widget);
    m_limitButton->setToolTip(tr("Adjust time to be within valid limits."));
    connect(m_limitButton, &QPushButton::clicked,
            this, &TimeWidget2::slotLimitClicked);
    hBox->addWidget(m_limitButton);

    // Center the widgets.
    hBox->insertStretch(-1, 1);

    updateWidgets();
}

void
TimeWidget2::updateWidgets()
{
    // Update all widgets from m_time and m_startTime.

    updateNote();
    updateMeasureBeat64();
    updateSecondsMsec();
    updateTempo();
    updateTicks();
    updateLimitWarning();
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
TimeWidget2::slotResetToDefault()
{
    setTime(m_defaultTime);
}

void
TimeWidget2::updateTempo()
{
    if (!m_isDuration)
        return;
    if (!m_tempo)
        return;

    // Tempo

    const timeT endTime = m_startTime + m_time;

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

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_measureSpin->blockSignals(true);
        m_measureSpin->setValue(bars);
        m_measureSpin->blockSignals(false);

        // Beats
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_beatSpin->blockSignals(true);
        m_beatSpin->setValue(beats);
        m_beatSpin->blockSignals(false);

        // 64ths
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_fractionSpin->blockSignals(true);
        m_fractionSpin->setValue(hemidemis);
        m_fractionSpin->blockSignals(false);

    } else {  // Absolute time mode

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
        m_measureSpin->setValue(bar + 1);
        m_measureSpin->blockSignals(false);

        // Beat
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_beatSpin->blockSignals(true);
        const TimeSignature timeSig =
                m_composition->getTimeSignatureAt(m_time);
        m_beatSpin->setMaximum(timeSig.getBeatsPerBar());
        m_beatSpin->setValue(beat);
        m_beatSpin->blockSignals(false);

        // 64ths
        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_fractionSpin->blockSignals(true);
        m_fractionSpin->setMaximum(
                timeSig.getBeatDuration() / Note(Note::Shortest).getDuration() - 1);
        m_fractionSpin->setValue(hemidemis);
        m_fractionSpin->blockSignals(false);

        // Time Signature
        m_timeSig->setText(tr("(%1/%2 time)").
                arg(timeSig.getNumerator()).
                arg(timeSig.getDenominator()));
    }
}

void
TimeWidget2::updateSecondsMsec()
{
    // Duration mode.
    if (m_isDuration) {

        // Seconds

        const timeT endTime = m_startTime + m_time;

        // Duration in seconds.
        const RealTime realTime = m_composition->getRealTimeDifference(
                m_startTime, endTime);

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_secondsSpin->blockSignals(true);
        m_secondsSpin->setValue(realTime.sec);
        m_secondsSpin->blockSignals(false);

        // msec

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_msecSpin->blockSignals(true);
        m_msecSpin->setValue(realTime.msec());
        m_msecSpin->blockSignals(false);

    } else {  // Absolute time mode.

        // Seconds

        const RealTime realTime = m_composition->getElapsedRealTime(m_time);

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_secondsSpin->blockSignals(true);
        m_secondsSpin->setValue(realTime.sec);
        m_secondsSpin->blockSignals(false);

        // msec

        // Have to block since QSpinBox::valueChanged() fires on
        // programmatic changes as well as user changes.
        m_msecSpin->blockSignals(true);
        if (realTime.sec == 0) {
            m_msecSpin->setMinimum(-999);
            m_msecSpin->setMaximum(999);
        } else if (realTime.sec < 0) {
            m_msecSpin->setMinimum(-999);
            m_msecSpin->setMaximum(0);
        } else {  // realTime.sec > 0
            m_msecSpin->setMinimum(0);
            m_msecSpin->setMaximum(999);
        }
        m_msecSpin->setValue(realTime.msec());
        m_msecSpin->blockSignals(false);

    }
}

void
TimeWidget2::updateTicks()
{
    // Have to block since QSpinBox::valueChanged() fires on
    // programmatic changes as well as user changes.
    m_ticksSpin->blockSignals(true);
    m_ticksSpin->setValue(m_time);
    m_ticksSpin->blockSignals(false);
}

void
TimeWidget2::updateNote()
{
    if (!m_isDuration)
        return;
    if (!m_noteCombo)
        return;

    // Assume <inexact>.
    m_noteCombo->setCurrentIndex(0);
    // Set the note combo to the appropriate value if found.
    for (size_t i = 0; i < m_noteDurations.size(); ++i) {
        if (m_time == m_noteDurations[i]) {
            m_noteCombo->setCurrentIndex(i);
            break;
        }
    }
}

void
TimeWidget2::updateLimitWarning()
{
    if (m_isDuration  &&  m_time < m_minimumDuration) {
        // Set a message of "Out of Range".
        m_limitMessage->setText(outOfRange);
        // Enable the limit button.
        m_limitButton->setEnabled(true);
        m_limit = m_minimumDuration;
        emit signalIsValid(false);
        return;
    }

    if (m_constrainToCompositionDuration) {
        if (m_isDuration) {

            // Test Case: RescaleDialog: Matrix View > Adjust > Rescale >
            //            Stretch or Squash...

            const timeT compositionEnd = m_composition->getEndMarker();
            // Too big to fit?
            if (m_startTime + m_time > compositionEnd)
            {
                // Set a message of "Out of Range".
                m_limitMessage->setText(outOfRange);
                // Enable the limit button.
                m_limitButton->setEnabled(true);
                m_limit = compositionEnd - m_startTime;
                emit signalIsValid(false);
                return;
            }

        } else {  // Absolute time.

            // Test Case: TempoDialog

            const timeT compositionStart = m_composition->getStartMarker();
            // Before start?
            if (m_time < compositionStart)
            {
                // Set a message of "Out of Range".
                m_limitMessage->setText(outOfRange);
                // Enable the limit button.
                m_limitButton->setEnabled(true);
                m_limit = compositionStart;
                emit signalIsValid(false);
                return;
            }

            const timeT compositionEnd = m_composition->getEndMarker();
            // After end?
            if (m_time >= compositionEnd)
            {
                // Set a message of "Out of Range".
                m_limitMessage->setText(outOfRange);
                // Enable the limit button.
                m_limitButton->setEnabled(true);
                m_limit = compositionEnd - 1;
                emit signalIsValid(false);
                return;
            }
        }
    }

    // Clear the message.
    m_limitMessage->setText("");
    // Disable the limit button.
    m_limitButton->setEnabled(false);

    emit signalIsValid(true);
}

void
TimeWidget2::slotLimitClicked(bool)
{
    m_time = m_limit;
    updateWidgets();
}

void
TimeWidget2::slotNoteChanged(int n)
{
    // 0 is <inexact>.  Nothing we can do with that.
    if (n <= 0)
        return;
    if (n >= (int)m_noteDurations.size())
        return;

    m_time = m_noteDurations[n];

    // Update the other fields.
    updateMeasureBeat64();
    updateSecondsMsec();
    updateTicks();
    updateTempo();
    updateLimitWarning();
}

void
TimeWidget2::slotTicksChanged(int ticks)
{
    m_time = ticks;

    // Update the other fields.
    updateMeasureBeat64();
    updateNote();
    updateSecondsMsec();
    updateTempo();
    updateLimitWarning();
}

void
TimeWidget2::slotMeasureBeatOrFractionChanged(int)
{
    const int bar = m_measureSpin->value();
    const int beat = m_beatSpin->value();
    const int fraction = m_fractionSpin->value();

    if (m_isDuration) {
        m_time = m_composition->getDurationForMusicalTime(
                m_startTime, bar, beat, fraction, 0);
    } else {
        m_time = m_composition->getAbsoluteTimeForMusicalTime(
                bar, beat, fraction, 0);
    }

    // Update the other fields.
    updateNote();
    updateSecondsMsec();
    updateTempo();
    updateTicks();
    updateLimitWarning();
}

void
TimeWidget2::slotSecondsOrMSecChanged(int)
{
    // Update msec range based on seconds.

    const int seconds = m_secondsSpin->value();
    // Save the old value so we can put it back if the sign changes.
    const int oldMsec = m_msecSpin->value();

    m_msecSpin->blockSignals(true);
    if (seconds == 0) {
        m_msecSpin->setMinimum(-999);
        m_msecSpin->setMaximum(999);
    } else if (seconds < 0) {
        m_msecSpin->setMinimum(-999);
        m_msecSpin->setMaximum(0);
        if (oldMsec > 0)
            m_msecSpin->setValue(-oldMsec);
    } else {  // seconds > 0
        m_msecSpin->setMinimum(0);
        m_msecSpin->setMaximum(999);
        if (oldMsec < 0)
            m_msecSpin->setValue(-oldMsec);
    }
    m_msecSpin->blockSignals(false);

    // Update m_time.

    const int msec = m_msecSpin->value();

    RealTime realTime(seconds, msec * 1000000);

    if (m_isDuration) {
        const RealTime startRT = m_composition->getElapsedRealTime(m_startTime);
        if (realTime >= RealTime::zero()) {
            m_time = m_composition->getElapsedTimeForRealTime(startRT + realTime) - m_startTime;
        } else {
            RG_DEBUG << "slotSecondsOrMSecChanged(): WARNING: realTime must be >= 0 for duration widget (was " << realTime << ")";
        }
    } else {
        m_time = m_composition->getElapsedTimeForRealTime(realTime);
    }

    // Update the other fields.
    updateNote();
    updateMeasureBeat64();
    updateTempo();
    updateTicks();
    updateLimitWarning();
}


}
