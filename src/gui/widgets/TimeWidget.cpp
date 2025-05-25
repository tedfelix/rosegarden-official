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
#include "gui/widgets/LineEdit.h"

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
                       bool editable,
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
    init(editable);
}

TimeWidget::TimeWidget(const QString& title,
                       QWidget *parent,
                       Composition *composition,
                       timeT startTime,
                       timeT initialDuration,
                       timeT minimumDuration,
                       bool editable,
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
    init(editable);
}

void
TimeWidget::init(bool editable)
{
    bool savedEditable = editable;
    editable = true;

    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(5);
    QLabel *label = nullptr;

    // Duration Mode
    if (m_isDuration) {

        label = new QLabel(tr("Note:"));
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label, 0, 0);

        if (editable) {
            m_note = new QComboBox;
            m_noteDurations.push_back(0);
            m_note->addItem(tr("<inexact>"));
            int denoms[] = {
                1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128
            };

            for (size_t i = 0; i < sizeof(denoms) / sizeof(denoms[0]); ++i) {

                timeT duration =
                    Note(Note::Breve).getDuration() / denoms[i];

                if (denoms[i] > 1 && denoms[i] < 128 && (denoms[i] % 3) != 0) {
                    // not breve or hemidemi, not a triplet
                    timeT dottedDuration = duration * 3 / 2;
                    m_noteDurations.push_back(dottedDuration);
                    timeT error = 0;
                    QString label = NotationStrings::makeNoteMenuLabel
                                    (dottedDuration, false, error);
                    QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(dottedDuration, error);
                    m_note->addItem(pmap, label); // ignore error
                }

                m_noteDurations.push_back(duration);
                timeT error = 0;
                QString label = NotationStrings::makeNoteMenuLabel
                                (duration, false, error);
                QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(duration, error);
                m_note->addItem(pmap, label); // ignore error
            }
            connect(m_note,
                        static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                    this, &TimeWidget::slotNoteChanged);
            layout->addWidget(m_note, 0, 1, 0-0+ 1, 3);

        } else {

            m_note = nullptr;
            timeT error = 0;
            QString label = NotationStrings::makeNoteMenuLabel(
                    m_time, false, error);
            if (error != 0)
                label = tr("<inexact>");
            LineEdit *le = new LineEdit(label);
            le->setReadOnly(true);
            layout->addWidget(le, 0, 1, 0- 0+1, 3);
        }

        label = new QLabel(tr("Units:"));
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label, 0, 4);

        if (editable) {
            m_timeSpin = new QSpinBox;
            m_timeSpin->setSingleStep(Note(Note::Shortest).getDuration());
            connect(m_timeSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                    this, &TimeWidget::slotTimeTChanged);
            layout->addWidget(m_timeSpin, 0, 5);
        } else {
            m_timeSpin = nullptr;
            LineEdit *le = new LineEdit(QString("%1").arg(m_time));
            le->setReadOnly(true);
            layout->addWidget(le, 0, 5);
        }

    } else {  // Absolute Time Mode

        m_note = nullptr;

        label = new QLabel(tr("Time:"));
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label, 0, 0);

        if (editable) {
            m_timeSpin = new QSpinBox;
            m_timeSpin->setSingleStep(Note(Note::Shortest).getDuration());
            connect(m_timeSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                    this, &TimeWidget::slotTimeTChanged);
            layout->addWidget(m_timeSpin, 0, 1);
            layout->addWidget(new QLabel(tr("units")), 0, 2);
        } else {
            m_timeSpin = nullptr;
            LineEdit *le = new LineEdit(QString("%1").arg(m_time));
            le->setReadOnly(true);
            layout->addWidget(le, 0, 2);
        }
    }

    // Measure/Measures
    label = new QLabel(m_isDuration ? tr("Measures:") : tr("Measure:"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 0);

    if (editable) {
        m_measureReadOnly = nullptr;
        m_measureSpin = new QSpinBox;
        if (m_isDuration)
            m_measureSpin->setMinimum(0);
        connect(m_measureSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget::slotBarBeatOrFractionChanged);
        layout->addWidget(m_measureSpin, 1, 1);
    } else {
        m_measureSpin = nullptr;
        m_measureReadOnly = new LineEdit;
        m_measureReadOnly->setReadOnly(true);
        layout->addWidget(m_measureReadOnly, 1, 1);
    }

    // Beat/Beats
    label = new QLabel(m_isDuration ? tr("beats:") : tr("beat:"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 2);

    if (editable) {
        m_beatReadOnly = nullptr;
        m_beatSpin = new QSpinBox;
        m_beatSpin->setMinimum(1);
        connect(m_beatSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget::slotBarBeatOrFractionChanged);
        layout->addWidget(m_beatSpin, 1, 3);
    } else {
        m_beatSpin = nullptr;
        m_beatReadOnly = new LineEdit;
        m_beatReadOnly->setReadOnly(true);
        layout->addWidget(m_beatReadOnly, 1, 3);
    }

    // 64ths
    label = new QLabel(tr("%1:").arg(NotationStrings::getShortNoteName(
            Note(Note::Shortest),  // note
            true)));  // plural
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 4);

    if (editable) {
        m_fractionReadOnly = nullptr;
        m_fractionSpin = new QSpinBox;
        m_fractionSpin->setMinimum(1);
        connect(m_fractionSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget::slotBarBeatOrFractionChanged);
        layout->addWidget(m_fractionSpin, 1, 5);
    } else {
        m_fractionSpin = nullptr;
        m_fractionReadOnly = new LineEdit;
        m_fractionReadOnly->setReadOnly(true);
        layout->addWidget(m_fractionReadOnly, 1, 5);
    }

    // Time Signature (e.g. 4/4 time)
    m_timeSig = new QLabel;
    layout->addWidget(m_timeSig, 1, 6);

    // Seconds
    label = new QLabel(tr("Seconds:"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 2, 0);

    if (editable) {
        m_secondsReadOnly = nullptr;
        m_secondsSpin = new QSpinBox;
        if (m_isDuration)
            m_secondsSpin->setMinimum(0);
        connect(m_secondsSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget::slotSecOrMSecChanged);
        layout->addWidget(m_secondsSpin, 2, 1);
    } else {
        m_secondsSpin = nullptr;
        m_secondsReadOnly = new LineEdit;
        m_secondsReadOnly->setReadOnly(true);
        layout->addWidget(m_secondsReadOnly, 2, 1);
    }

    // msec
    label = new QLabel(tr("msec:"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 2, 2);

    if (editable) {
        m_msecReadOnly = nullptr;
        m_msecSpin = new QSpinBox;
        m_msecSpin->setMinimum(0);
        m_msecSpin->setSingleStep(10);
        connect(m_msecSpin, (void(QSpinBox::*)(int))(&QSpinBox::valueChanged),
                this, &TimeWidget::slotMSecChanged);
        layout->addWidget(m_msecSpin, 2, 3);
    } else {
        m_msecSpin = nullptr;
        m_msecReadOnly = new LineEdit;
        m_msecReadOnly->setReadOnly(true);
        layout->addWidget(m_msecReadOnly, 2, 3);
    }

    if (m_isDuration) {
        m_tempo = new QLabel;
        layout->addWidget(m_tempo, 2, 6);
    } else {
        m_tempo = nullptr;
    }

    if (!savedEditable) {
        if (m_note)
            m_note ->setEnabled(false);
        if (m_timeSpin)
            m_timeSpin ->setEnabled(false);
        if (m_measureSpin)
            m_measureSpin ->setEnabled(false);
        if (m_beatSpin)
            m_beatSpin ->setEnabled(false);
        if (m_fractionSpin)
            m_fractionSpin ->setEnabled(false);
        if (m_secondsSpin)
            m_secondsSpin ->setEnabled(false);
        if (m_msecSpin)
            m_msecSpin ->setEnabled(false);
    }

    populate();

    // Create a One-shot timer for use in msec and unit box to delay updates
    m_delayUpdateTimer = new QTimer(this);
    m_delayUpdateTimer->setSingleShot(true);
}

void
TimeWidget::populate()
{
    // populate everything from m_time and m_startTime

    if (m_note)
        m_note->blockSignals(true);
    if (m_timeSpin)
        m_timeSpin->blockSignals(true);
    if (m_measureSpin)
        m_measureSpin->blockSignals(true);
    if (m_beatSpin)
        m_beatSpin->blockSignals(true);
    if (m_fractionSpin)
        m_fractionSpin->blockSignals(true);
    if (m_secondsSpin)
        m_secondsSpin->blockSignals(true);
    if (m_msecSpin)
        m_msecSpin->blockSignals(true);

    if (m_isDuration) {

        if (m_time + m_startTime > m_composition->getEndMarker()) {
            m_time = m_composition->getEndMarker() - m_startTime;
        }

        if (m_timeSpin) {
            m_timeSpin->setMinimum(0);
            if (m_constrainToCompositionDuration) {
                m_timeSpin->setMaximum(m_composition->getEndMarker() - m_startTime);
            } else {
                m_timeSpin->setMaximum(INT_MAX);
            }
            m_timeSpin->setValue(m_time);
        }

        if (m_note) {
            m_note->setCurrentIndex(0);
            for (size_t i = 0; i < m_noteDurations.size(); ++i) {
                if (m_time == m_noteDurations[i]) {
                    m_note->setCurrentIndex(i);
                    break;
                }
            }
        }

        // the bar/beat etc timings are considered to be times of a note
        // starting at the start of a bar, in the time signature in effect
        // at m_startTime

        int bars = 0, beats = 0, hemidemis = 0, remainder = 0;
        m_composition->getMusicalTimeForDuration(m_startTime, m_time,
                bars, beats, hemidemis, remainder);
        TimeSignature timeSig =
            m_composition->getTimeSignatureAt(m_startTime);

        if (m_measureSpin) {
            m_measureSpin->setMinimum(0);
            if (m_constrainToCompositionDuration) {
                m_measureSpin->setMaximum
                    (m_composition->getBarNumber(m_composition->getEndMarker()) -
                     m_composition->getBarNumber(m_startTime));
            } else {
                m_measureSpin->setMaximum(9999);
            }
            m_measureSpin->setValue(bars);
        } else {
            m_measureReadOnly->setText(QString("%1").arg(bars));
        }

        if (m_beatSpin) {
            m_beatSpin->setMinimum(0);
            m_beatSpin->setMaximum(timeSig.getBeatsPerBar() - 1);
            m_beatSpin->setValue(beats);
        } else {
            m_beatReadOnly->setText(QString("%1").arg(beats));
        }

        if (m_fractionSpin) {
            m_fractionSpin->setMinimum(0);
            m_fractionSpin->setMaximum(timeSig.getBeatDuration() /
                                    Note(Note::Shortest).
                                    getDuration() - 1);
            m_fractionSpin->setValue(hemidemis);
        } else {
            m_fractionReadOnly->setText(QString("%1").arg(hemidemis));
        }

        m_timeSig->setText(tr("(%1/%2 time)")
			   .arg(timeSig.getNumerator())
                           .arg(timeSig.getDenominator()));

        timeT endTime = m_startTime + m_time;

        RealTime rt = m_composition->getRealTimeDifference
                      (m_startTime, endTime);

        if (m_secondsSpin) {
            m_secondsSpin->setMinimum(0);
            if (m_constrainToCompositionDuration) {
                m_secondsSpin->setMaximum(m_composition->getRealTimeDifference
                                   (m_startTime, m_composition->getEndMarker()).sec);
            } else {
                m_secondsSpin->setMaximum(9999);
            }
            m_secondsSpin->setValue(rt.sec);
        } else {
            m_secondsReadOnly->setText(QString("%1").arg(rt.sec));
        }

        if (m_msecSpin) {
            m_msecSpin->setMinimum(0);
            m_msecSpin->setMaximum(999);

            // Round value instead of direct read from rt.msec
            // Causes cycle of rounding between msec and units
            // which creates odd typing behavior
            m_msecSpin->setValue(getRoundedMSec(rt));
        } else {
            m_msecReadOnly->setText(QString("%1").arg(rt.msec()));
        }

        bool change = (m_composition->getTempoChangeNumberAt(endTime) !=
                       m_composition->getTempoChangeNumberAt(m_startTime));

        // imprecise -- better to work from tempoT directly
        double tempo = m_composition->getTempoQpm(m_composition->getTempoAtTime(m_startTime));

        int qpmc = int(tempo * 100.0);
        int bpmc = qpmc;
        if (timeSig.getBeatDuration()
                != Note(Note::Crotchet).getDuration()) {
            bpmc = int(tempo * 100.0 *
                       Note(Note::Crotchet).getDuration() /
                       timeSig.getBeatDuration());
        }
        if (change) {
            if (bpmc != qpmc) {
                m_tempo->setText(tr("(starting %1.%2 qpm, %3.%4 bpm)")
                                 .arg(qpmc / 100)
                                 .arg(qpmc % 100)
                                 .arg(bpmc / 100)
                                 .arg(bpmc % 100));
            } else {
                m_tempo->setText(tr("(starting %1.%2 bpm)")
                                 .arg(bpmc / 100)
                                 .arg(bpmc % 100));
            }
        } else {
            if (bpmc != qpmc) {
                m_tempo->setText(tr("(%1.%2 qpm, %3.%4 bpm)")
                                 .arg(qpmc / 100)
                                 .arg(qpmc % 100)
                                 .arg(bpmc / 100)
                                 .arg(bpmc % 100));
            } else {
                m_tempo->setText(tr("(%1.%2 bpm)")
                                 .arg(bpmc / 100)
                                 .arg(bpmc % 100));
            }
        }

    } else {

        if (m_constrainToCompositionDuration && m_time > m_composition->getEndMarker()) {
            m_time = m_composition->getEndMarker();
        }

        if (m_constrainToCompositionDuration && m_time < m_composition->getStartMarker()) {
            m_time = m_composition->getStartMarker();
        }

        if (m_timeSpin) {
            if (m_constrainToCompositionDuration) {
                m_timeSpin->setMinimum(m_composition->getStartMarker());
                m_timeSpin->setMaximum(m_composition->getEndMarker());
            } else {
                m_timeSpin->setMinimum(INT_MIN);
                m_timeSpin->setMaximum(INT_MAX);
            }
            m_timeSpin->setValue(m_time);

        }

        int bar = 1, beat = 1, hemidemis = 0, remainder = 0;
        m_composition->getMusicalTimeForAbsoluteTime
        (m_time, bar, beat, hemidemis, remainder);

        TimeSignature timeSig =
            m_composition->getTimeSignatureAt(m_time);

        if (m_measureSpin) {
            m_measureSpin->setMinimum(INT_MIN);
            if (m_constrainToCompositionDuration) {
                m_measureSpin->setMaximum(m_composition->getBarNumber
                                   (m_composition->getEndMarker()));
            } else {
                m_measureSpin->setMaximum(9999);
            }
            m_measureSpin->setValue(bar + 1);
        } else {
            m_measureReadOnly->setText(QString("%1").arg(bar + 1));
        }

        if (m_beatSpin) {
            m_beatSpin->setMinimum(1);
            m_beatSpin->setMaximum(timeSig.getBeatsPerBar());
            m_beatSpin->setValue(beat);
        } else {
            m_beatReadOnly->setText(QString("%1").arg(beat));
        }

        if (m_fractionSpin) {
            m_fractionSpin->setMinimum(0);
            m_fractionSpin->setMaximum(timeSig.getBeatDuration() /
                                    Note(Note::Shortest).
                                    getDuration() - 1);
            m_fractionSpin->setValue(hemidemis);
        } else {
            m_fractionReadOnly->setText(QString("%1").arg(hemidemis));
        }

        m_timeSig->setText(tr("(%1/%2 time)")
	                   .arg(timeSig.getNumerator())
                           .arg(timeSig.getDenominator()));

        RealTime rt = m_composition->getElapsedRealTime(m_time);

        if (m_secondsSpin) {
            m_secondsSpin->setMinimum(INT_MIN);
            if (m_constrainToCompositionDuration) {
                m_secondsSpin->setMaximum(m_composition->getElapsedRealTime
                                   (m_composition->getEndMarker()).sec);
            } else {
                m_secondsSpin->setMaximum(9999);
            }
            m_secondsSpin->setValue(rt.sec);
        } else {
            m_secondsReadOnly->setText(QString("%1").arg(rt.sec));
        }

        if (m_msecSpin) {

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
        } else {
            m_msecReadOnly->setText(QString("%1").arg(rt.msec()));
        }
    }

    if (m_note)
        m_note ->blockSignals(false);
    if (m_timeSpin)
        m_timeSpin ->blockSignals(false);
    if (m_measureSpin)
        m_measureSpin ->blockSignals(false);
    if (m_beatSpin)
        m_beatSpin ->blockSignals(false);
    if (m_fractionSpin)
        m_fractionSpin ->blockSignals(false);
    if (m_secondsSpin)
        m_secondsSpin ->blockSignals(false);
    if (m_msecSpin)
        m_msecSpin ->blockSignals(false);
}

timeT
TimeWidget::getTime()
{
    return m_time;
}

RealTime
TimeWidget::getRealTime()
{
    if (m_isDuration) {
        return m_composition->getRealTimeDifference(m_startTime,
                m_startTime + m_time);
    } else {
        return m_composition->getElapsedRealTime(m_time);
    }
}

int
TimeWidget::getRoundedMSec(RealTime rt)
{
    double msecDouble = rt.usec() / 1000.0;
    return rt.sec > 0 ? floor(msecDouble + 0.5) : ceil(msecDouble - 0.5);

}

void
TimeWidget::slotSetTime(timeT t)
{
    if (m_isDuration  &&  t < m_minimumDuration)
        t = m_minimumDuration;

    m_time = t;
    populate();

    // ??? No one appears to ever connect to this signal.
    //emit timeChanged(getTime());
    // ??? No one appears to ever connect to this signal.
    //emit realTimeChanged(getRealTime());
}

void
TimeWidget::slotSetRealTime(RealTime rt)
{
    if (m_isDuration) {
        RealTime startRT = m_composition->getElapsedRealTime(m_startTime);
        if (rt >= RealTime::zero()) {
            slotSetTime(m_composition->getElapsedTimeForRealTime(startRT + rt) -
                        m_startTime);
        } else {
            RG_DEBUG << "WARNING: TimeWidget::slotSetRealTime: rt must be >0 for duration widget (was " << rt << ")";
        }
    } else {
        slotSetTime(m_composition->getElapsedTimeForRealTime(rt));
    }
}

void
TimeWidget::slotResetToDefault()
{
    slotSetTime(m_defaultTime);
}

void
TimeWidget::slotNoteChanged(int n)
{
    if (n > 0) {
        slotSetTime(m_noteDurations[n]);
    }
}

void
TimeWidget::slotTimeTChanged(int t)
{
    RG_DEBUG << "slotTimeTChanged: t is " << t << ", value is " << m_timeSpin->value();

    // The Time field has changed.  Either from the user typing into it or
    // from the up/down arrows on the spin box.

    // Not editable?  Then how did we ever get here?
    if (!m_timeSpin)
        return;
    // Not editable?  Then how did we ever get here?
    if (!m_msecSpin)
        return;
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
    if (m_timeSpin)
    {
        slotSetTime(m_timeSpin->value());
    }
    else
    {
        RG_DEBUG << "slotTimeTUpdate: no m_timeT found, but slotCalled in error "
                << " noop.";
    }
}

void
TimeWidget::slotBarBeatOrFractionChanged(int)
{
    int bar = m_measureSpin->value();
    int beat = m_beatSpin->value();
    int fraction = m_fractionSpin->value();

    if (m_isDuration) {
        slotSetTime(m_composition->getDurationForMusicalTime
                    (m_startTime, bar, beat, fraction, 0));

    } else {
        slotSetTime(m_composition->getAbsoluteTimeForMusicalTime
                    (bar, beat, fraction, 0));
    }
}

void
TimeWidget::slotSecOrMSecChanged(int)
{
    int sec = m_secondsSpin->value();
    int msec = m_msecSpin->value();

    slotSetRealTime(RealTime(sec, msec * 1000000));
}

void
TimeWidget::slotMSecChanged(int)
{
    m_delayUpdateTimer->stop();

    // Disconnect any signals from timer
    m_delayUpdateTimer->disconnect();

    // Disconnect signals from m_timeT
    if (m_timeSpin) {
        disconnect(m_timeSpin, &QAbstractSpinBox::editingFinished,
                   this, &TimeWidget::slotTimeTUpdate);

    }

    // Disconnect signals from m_msec
    if (m_msecSpin) {
        disconnect(m_msecSpin, &QAbstractSpinBox::editingFinished,
                   this, &TimeWidget::slotMSecUpdate);
    }

    if (m_msecSpin)  // Checking in case called by accident
    {
        connect(m_msecSpin, &QAbstractSpinBox::editingFinished,
                this, &TimeWidget::slotMSecUpdate);
    }

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
