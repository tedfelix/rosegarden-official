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

#define RG_MODULE_STRING "[TimeWidget]"

#include "TimeWidget.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/LineEdit.h"

#include <cmath>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPixmap>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QLayout>
#include <QTimer>


namespace Rosegarden
{

TimeWidget::TimeWidget(QString title,
                       QWidget *parent,
                       Composition *composition,
                       timeT absTime,
                       bool editable,
                       bool constrainToCompositionDuration) :
        QGroupBox(title, parent),
        m_composition(composition),
        m_isDuration(false),
        m_constrain(constrainToCompositionDuration),
        m_time(absTime),
        m_startTime(0),
        m_defaultTime(absTime),
        m_delayUpdateTimer(0)
{
    init(editable);
}

TimeWidget::TimeWidget(QString title,
                       QWidget *parent,
                       Composition *composition,
                       timeT startTime,
                       timeT duration,
                       timeT minimumDuration,
                       bool editable,
                       bool constrainToCompositionDuration) :
        QGroupBox(title, parent),
        m_composition(composition),
        m_isDuration(true),
        m_constrain(constrainToCompositionDuration),
        m_time(duration),
        m_startTime(startTime),
        m_defaultTime(duration),
        m_minimumDuration(minimumDuration),
        m_delayUpdateTimer(0)
{
    init(editable);
}

void
TimeWidget::init(bool editable)
{
    int denoms[] = {
                       1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128
                   };

    bool savedEditable = editable;
    editable = true;

    setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(5);
    QLabel *label = 0;

    if (m_isDuration) {

        label = new QLabel(tr("Note:"));
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label, 0, 0);

        if (editable) {
            m_note = new QComboBox;
            m_noteDurations.push_back(0);
            m_note->addItem(tr("<inexact>"));
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
            connect(m_note, SIGNAL(activated(int)),
                    this, SLOT(slotNoteChanged(int)));
            layout->addWidget(m_note, 0, 1, 0-0+ 1, 3);

        } else {

            m_note = 0;
            timeT error = 0;
            QString label = NotationStrings::makeNoteMenuLabel
                            (m_time, false, error);
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
            m_timeT = new QSpinBox;
            m_timeT->setSingleStep
            (Note(Note::Shortest).getDuration());
            connect(m_timeT, SIGNAL(valueChanged(int)),
                    this, SLOT(slotTimeTChanged(int)));
            layout->addWidget(m_timeT, 0, 5);
        } else {
            m_timeT = 0;
            LineEdit *le = new LineEdit(QString("%1").arg(m_time));
            le->setReadOnly(true);
            layout->addWidget(le, 0, 5);
        }

    } else {

        m_note = 0;

        label = new QLabel(tr("Time:"));
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label, 0, 0);

        if (editable) {
            m_timeT = new QSpinBox;
            m_timeT->setSingleStep
            (Note(Note::Shortest).getDuration());
            connect(m_timeT, SIGNAL(valueChanged(int)),
                    this, SLOT(slotTimeTChanged(int)));
            layout->addWidget(m_timeT, 0, 1);
            layout->addWidget(new QLabel(tr("units")), 0, 2);
        } else {
            m_timeT = 0;
            LineEdit *le = new LineEdit(QString("%1").arg(m_time));
            le->setReadOnly(true);
            layout->addWidget(le, 0, 2);
        }
    }

    label = new QLabel(m_isDuration ? tr("Measures:") : tr("Measure:"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 0);

    if (editable) {
        m_barLabel = 0;
        m_bar = new QSpinBox;
        if (m_isDuration)
            m_bar->setMinimum(0);
        connect(m_bar, SIGNAL(valueChanged(int)),
                this, SLOT(slotBarBeatOrFractionChanged(int)));
        layout->addWidget(m_bar, 1, 1);
    } else {
        m_bar = 0;
        m_barLabel = new LineEdit;
        m_barLabel->setReadOnly(true);
        layout->addWidget(m_barLabel, 1, 1);
    }

    label = new QLabel(m_isDuration ? tr("beats:") : tr("beat:"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 2);

    if (editable) {
        m_beatLabel = 0;
        m_beat = new QSpinBox;
        m_beat->setMinimum(1);
        connect(m_beat, SIGNAL(valueChanged(int)),
                this, SLOT(slotBarBeatOrFractionChanged(int)));
        layout->addWidget(m_beat, 1, 3);
    } else {
        m_beat = 0;
        m_beatLabel = new LineEdit;
        m_beatLabel->setReadOnly(true);
        layout->addWidget(m_beatLabel, 1, 3);
    }

    label = new QLabel(tr("%1:").arg(NotationStrings::getShortNoteName
                                            (Note(Note::Shortest), true)));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 4);

    if (editable) {
        m_fractionLabel = 0;
        m_fraction = new QSpinBox;
        m_fraction->setMinimum(1);
        connect(m_fraction, SIGNAL(valueChanged(int)),
                this, SLOT(slotBarBeatOrFractionChanged(int)));
        layout->addWidget(m_fraction, 1, 5);
    } else {
        m_fraction = 0;
        m_fractionLabel = new LineEdit;
        m_fractionLabel->setReadOnly(true);
        layout->addWidget(m_fractionLabel, 1, 5);
    }

    m_timeSig = new QLabel;
    layout->addWidget(m_timeSig, 1, 6);

    label = new QLabel(tr("Seconds:"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 2, 0);

    if (editable) {
        m_secLabel = 0;
        m_sec = new QSpinBox;
        if (m_isDuration)
            m_sec->setMinimum(0);
        connect(m_sec, SIGNAL(valueChanged(int)),
                this, SLOT(slotSecOrMSecChanged(int)));
        layout->addWidget(m_sec, 2, 1);
    } else {
        m_sec = 0;
        m_secLabel = new LineEdit;
        m_secLabel->setReadOnly(true);
        layout->addWidget(m_secLabel, 2, 1);
    }

    label = new QLabel(tr("msec:"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 2, 2);

    if (editable) {
        m_msecLabel = 0;
        m_msec = new QSpinBox;
        m_msec->setMinimum(0);
        m_msec->setSingleStep(10);
        connect(m_msec, SIGNAL(valueChanged(int)),
                this, SLOT(slotMSecChanged()));
        layout->addWidget(m_msec, 2, 3);
    } else {
        m_msec = 0;
        m_msecLabel = new LineEdit;
        m_msecLabel->setReadOnly(true);
        layout->addWidget(m_msecLabel, 2, 3);
    }

    if (m_isDuration) {
        m_tempo = new QLabel;
        layout->addWidget(m_tempo, 2, 6);
    } else {
        m_tempo = 0;
    }

    setLayout(layout);

    if (!savedEditable) {
        if (m_note)
            m_note ->setEnabled(false);
        if (m_timeT)
            m_timeT ->setEnabled(false);
        if (m_bar)
            m_bar ->setEnabled(false);
        if (m_beat)
            m_beat ->setEnabled(false);
        if (m_fraction)
            m_fraction ->setEnabled(false);
        if (m_sec)
            m_sec ->setEnabled(false);
        if (m_msec)
            m_msec ->setEnabled(false);
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
        m_note ->blockSignals(true);
    if (m_timeT)
        m_timeT ->blockSignals(true);
    if (m_bar)
        m_bar ->blockSignals(true);
    if (m_beat)
        m_beat ->blockSignals(true);
    if (m_fraction)
        m_fraction ->blockSignals(true);
    if (m_sec)
        m_sec ->blockSignals(true);
    if (m_msec)
        m_msec ->blockSignals(true);

    if (m_isDuration) {

        if (m_time + m_startTime > m_composition->getEndMarker()) {
            m_time = m_composition->getEndMarker() - m_startTime;
        }

        if (m_timeT) {
            m_timeT->setMinimum(0);
            if (m_constrain) {
                m_timeT->setMaximum(m_composition->getEndMarker() - m_startTime);
            } else {
                m_timeT->setMaximum(INT_MAX);
            }
            m_timeT->setValue(m_time);
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

        if (m_bar) {
            m_bar->setMinimum(0);
            if (m_constrain) {
                m_bar->setMaximum
                    (m_composition->getBarNumber(m_composition->getEndMarker()) -
                     m_composition->getBarNumber(m_startTime));
            } else {
                m_bar->setMaximum(9999);
            }
            m_bar->setValue(bars);
        } else {
            m_barLabel->setText(QString("%1").arg(bars));
        }

        if (m_beat) {
            m_beat->setMinimum(0);
            m_beat->setMaximum(timeSig.getBeatsPerBar() - 1);
            m_beat->setValue(beats);
        } else {
            m_beatLabel->setText(QString("%1").arg(beats));
        }

        if (m_fraction) {
            m_fraction->setMinimum(0);
            m_fraction->setMaximum(timeSig.getBeatDuration() /
                                    Note(Note::Shortest).
                                    getDuration() - 1);
            m_fraction->setValue(hemidemis);
        } else {
            m_fractionLabel->setText(QString("%1").arg(hemidemis));
        }

        m_timeSig->setText(tr("(%1/%2 time)")
			   .arg(timeSig.getNumerator())
                           .arg(timeSig.getDenominator()));

        timeT endTime = m_startTime + m_time;

        RealTime rt = m_composition->getRealTimeDifference
                      (m_startTime, endTime);

        if (m_sec) {
            m_sec->setMinimum(0);
            if (m_constrain) {
                m_sec->setMaximum(m_composition->getRealTimeDifference
                                   (m_startTime, m_composition->getEndMarker()).sec);
            } else {
                m_sec->setMaximum(9999);
            }
            m_sec->setValue(rt.sec);
        } else {
            m_secLabel->setText(QString("%1").arg(rt.sec));
        }

        if (m_msec) {
            m_msec->setMinimum(0);
            m_msec->setMaximum(999);

            // Round value instead of direct read from rt.msec
            // Causes cycle of rounding between msec and units
            // which creates odd typing behavior
            m_msec->setValue(getRoundedMSec(rt));
        } else {
            m_msecLabel->setText(QString("%1").arg(rt.msec()));
        }

        bool change = (m_composition->getTempoChangeNumberAt(endTime) !=
                       m_composition->getTempoChangeNumberAt(m_startTime));

        //!!! imprecise -- better to work from tempoT directly
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

        if (m_constrain && m_time > m_composition->getEndMarker()) {
            m_time = m_composition->getEndMarker();
        }

        if (m_constrain && m_time < m_composition->getStartMarker()) {
            m_time = m_composition->getStartMarker();
        }

        if (m_timeT) {
            if (m_constrain) {
                m_timeT->setMinimum(m_composition->getStartMarker());
                m_timeT->setMaximum(m_composition->getEndMarker());
            } else {
                m_timeT->setMinimum(INT_MIN);
                m_timeT->setMaximum(INT_MAX);
            }
            m_timeT->setValue(m_time);

        }

        int bar = 1, beat = 1, hemidemis = 0, remainder = 0;
        m_composition->getMusicalTimeForAbsoluteTime
        (m_time, bar, beat, hemidemis, remainder);

        TimeSignature timeSig =
            m_composition->getTimeSignatureAt(m_time);

        if (m_bar) {
            m_bar->setMinimum(INT_MIN);
            if (m_constrain) {
                m_bar->setMaximum(m_composition->getBarNumber
                                   (m_composition->getEndMarker()));
            } else {
                m_bar->setMaximum(9999);
            }
            m_bar->setValue(bar + 1);
        } else {
            m_barLabel->setText(QString("%1").arg(bar + 1));
        }

        if (m_beat) {
            m_beat->setMinimum(1);
            m_beat->setMaximum(timeSig.getBeatsPerBar());
            m_beat->setValue(beat);
        } else {
            m_beatLabel->setText(QString("%1").arg(beat));
        }

        if (m_fraction) {
            m_fraction->setMinimum(0);
            m_fraction->setMaximum(timeSig.getBeatDuration() /
                                    Note(Note::Shortest).
                                    getDuration() - 1);
            m_fraction->setValue(hemidemis);
        } else {
            m_fractionLabel->setText(QString("%1").arg(hemidemis));
        }

        m_timeSig->setText(tr("(%1/%2 time)")
	                   .arg(timeSig.getNumerator())
                           .arg(timeSig.getDenominator()));

        RealTime rt = m_composition->getElapsedRealTime(m_time);

        if (m_sec) {
            m_sec->setMinimum(INT_MIN);
            if (m_constrain) {
                m_sec->setMaximum(m_composition->getElapsedRealTime
                                   (m_composition->getEndMarker()).sec);
            } else {
                m_sec->setMaximum(9999);
            }
            m_sec->setValue(rt.sec);
        } else {
            m_secLabel->setText(QString("%1").arg(rt.sec));
        }

        if (m_msec) {

            if (m_time >= 0)
            {
                m_msec->setMinimum(0);
                m_msec->setMaximum(999);
            }
            else
            {
                m_msec->setMinimum(-999);
                m_msec->setMaximum(0);
            }

            // Round value instead of direct read from rt.msec
            // Causes cycle of rounding between msec and units
            // which creates odd typing behavior
            m_msec->setValue(getRoundedMSec(rt));
        } else {
            m_msecLabel->setText(QString("%1").arg(rt.msec()));
        }
    }

    if (m_note)
        m_note ->blockSignals(false);
    if (m_timeT)
        m_timeT ->blockSignals(false);
    if (m_bar)
        m_bar ->blockSignals(false);
    if (m_beat)
        m_beat ->blockSignals(false);
    if (m_fraction)
        m_fraction ->blockSignals(false);
    if (m_sec)
        m_sec ->blockSignals(false);
    if (m_msec)
        m_msec ->blockSignals(false);
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

void
TimeWidget::disconnectSignals()
{
    // Disconnect any signals from timer
    m_delayUpdateTimer->disconnect();

    // Disconnect singals from m_timeT
    if (m_timeT)
    {
        m_timeT->disconnect(SIGNAL(editingFinished()));
    }

    // Disconnect singals from m_msec
    if (m_msec)
    {
        m_msec->disconnect(SIGNAL(editingFinished()));
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
    emit timeChanged(getTime());
    emit realTimeChanged(getRealTime());
}

void
TimeWidget::slotSetRealTime(RealTime rt)
{
    if (m_isDuration) {
        RealTime startRT = m_composition->getElapsedRealTime(m_startTime);
        if (rt >= RealTime::zeroTime) {
            slotSetTime(m_composition->getElapsedTimeForRealTime(startRT + rt) -
                        m_startTime);
        } else {
            RG_DEBUG << "WARNING: TimeWidget::slotSetRealTime: rt must be >0 for duration widget (was " << rt << ")" << endl;
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
    RG_DEBUG << "slotTimeTChanged: t is " << t << ", value is " << m_timeT->value() << endl;

    m_delayUpdateTimer->stop();

    disconnectSignals();

    if (m_timeT)  // Checking in case called by accident
    {
        connect(m_timeT, SIGNAL(editingFinished()),
                this, SLOT(slotTimeTUpdate()));
    }

    connect(m_delayUpdateTimer, SIGNAL(timeout()),
            this, SLOT(slotTimeTUpdate()));

    m_delayUpdateTimer->start(UPDATE_DELAY_TIME);
}

void
TimeWidget::slotTimeTUpdate()
{
    // May have fired already, but stop it in case called when widget lost
    // focus.
    m_delayUpdateTimer->stop();

    // Perform an immediate update.
    if (m_timeT)
    {
        slotSetTime(m_timeT->value());
    }
    else
    {
        RG_DEBUG << "slotTimeTUpdate: no m_timeT found, but slotCalled in error "
                << " noop." << endl;
    }
}

void
TimeWidget::slotBarBeatOrFractionChanged(int)
{
    int bar = m_bar->value();
    int beat = m_beat->value();
    int fraction = m_fraction->value();

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
    int sec = m_sec->value();
    int msec = m_msec->value();

    slotSetRealTime(RealTime(sec, msec * 1000000));
}

void
TimeWidget::slotMSecChanged()
{
    m_delayUpdateTimer->stop();

    disconnectSignals();

    if (m_msec)  // Checking in case called by accident
    {
        connect(m_msec, SIGNAL(editingFinished()),
                this, SLOT(slotMSecUpdate()));
    }

    connect(m_delayUpdateTimer, SIGNAL(timeout()),
            this, SLOT(slotMSecUpdate()));

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
