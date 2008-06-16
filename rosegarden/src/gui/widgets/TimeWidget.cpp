/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TimeWidget.h"
#include <qlayout.h>

#include <klocale.h>
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <qcombobox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpixmap.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

TimeWidget::TimeWidget(QString title,
                       QWidget *parent,
                       Composition *composition,
                       timeT absTime,
                       bool editable,
                       bool constrainToCompositionDuration) :
        QGroupBox(1, Horizontal, title, parent),
        m_composition(composition),
        m_isDuration(false),
        m_constrain(constrainToCompositionDuration),
        m_time(absTime),
        m_startTime(0),
        m_defaultTime(absTime)
{
    init(editable);
}

TimeWidget::TimeWidget(QString title,
                       QWidget *parent,
                       Composition *composition,
                       timeT startTime,
                       timeT duration,
                       bool editable,
                       bool constrainToCompositionDuration) :
        QGroupBox(1, Horizontal, title, parent),
        m_composition(composition),
        m_isDuration(true),
        m_constrain(constrainToCompositionDuration),
        m_time(duration),
        m_startTime(startTime),
        m_defaultTime(duration)
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

    QFrame *frame = new QFrame(this);
    QGridLayout *layout = new QGridLayout(frame, 7, 3, 5, 5);
    QLabel *label = 0;

    if (m_isDuration) {

        label = new QLabel(i18n("Note:"), frame);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label, 0, 0);

        if (editable) {
            m_note = new QComboBox(frame);
            m_noteDurations.push_back(0);
            m_note->insertItem(i18n("<inexact>"));
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
                    QPixmap pmap = NotePixmapFactory::toQPixmap
                                   (NotePixmapFactory::makeNoteMenuPixmap(dottedDuration, error));
                    m_note->insertItem(pmap, label); // ignore error
                }

                m_noteDurations.push_back(duration);
                timeT error = 0;
                QString label = NotationStrings::makeNoteMenuLabel
                                (duration, false, error);
                QPixmap pmap = NotePixmapFactory::toQPixmap
                               (NotePixmapFactory::makeNoteMenuPixmap(duration, error));
                m_note->insertItem(pmap, label); // ignore error
            }
            connect(m_note, SIGNAL(activated(int)),
                    this, SLOT(slotNoteChanged(int)));
            layout->addMultiCellWidget(m_note, 0, 0, 1, 3);

        } else {

            m_note = 0;
            timeT error = 0;
            QString label = NotationStrings::makeNoteMenuLabel
                            (m_time, false, error);
            if (error != 0)
                label = i18n("<inexact>");
            QLineEdit *le = new QLineEdit(label, frame);
            le->setReadOnly(true);
            layout->addMultiCellWidget(le, 0, 0, 1, 3);
        }

        label = new QLabel(i18n("Units:"), frame);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label, 0, 4);

        if (editable) {
            m_timeT = new QSpinBox(frame);
            m_timeT->setLineStep
            (Note(Note::Shortest).getDuration());
            connect(m_timeT, SIGNAL(valueChanged(int)),
                    this, SLOT(slotTimeTChanged(int)));
            layout->addWidget(m_timeT, 0, 5);
        } else {
            m_timeT = 0;
            QLineEdit *le = new QLineEdit(QString("%1").arg(m_time), frame);
            le->setReadOnly(true);
            layout->addWidget(le, 0, 5);
        }

    } else {

        m_note = 0;

        label = new QLabel(i18n("Time:"), frame);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(label, 0, 0);

        if (editable) {
            m_timeT = new QSpinBox(frame);
            m_timeT->setLineStep
            (Note(Note::Shortest).getDuration());
            connect(m_timeT, SIGNAL(valueChanged(int)),
                    this, SLOT(slotTimeTChanged(int)));
            layout->addWidget(m_timeT, 0, 1);
            layout->addWidget(new QLabel(i18n("units"), frame), 0, 2);
        } else {
            m_timeT = 0;
            QLineEdit *le = new QLineEdit(QString("%1").arg(m_time), frame);
            le->setReadOnly(true);
            layout->addWidget(le, 0, 2);
        }
    }

    label = new QLabel(m_isDuration ? i18n("Measures:") : i18n("Measure:"), frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 0);

    if (editable) {
        m_barLabel = 0;
        m_bar = new QSpinBox(frame);
        if (m_isDuration)
            m_bar->setMinValue(0);
        connect(m_bar, SIGNAL(valueChanged(int)),
                this, SLOT(slotBarBeatOrFractionChanged(int)));
        layout->addWidget(m_bar, 1, 1);
    } else {
        m_bar = 0;
        m_barLabel = new QLineEdit(frame);
        m_barLabel->setReadOnly(true);
        layout->addWidget(m_barLabel, 1, 1);
    }

    label = new QLabel(m_isDuration ? i18n("beats:") : i18n("beat:"), frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 2);

    if (editable) {
        m_beatLabel = 0;
        m_beat = new QSpinBox(frame);
        m_beat->setMinValue(1);
        connect(m_beat, SIGNAL(valueChanged(int)),
                this, SLOT(slotBarBeatOrFractionChanged(int)));
        layout->addWidget(m_beat, 1, 3);
    } else {
        m_beat = 0;
        m_beatLabel = new QLineEdit(frame);
        m_beatLabel->setReadOnly(true);
        layout->addWidget(m_beatLabel, 1, 3);
    }

    label = new QLabel(i18n("%1:").arg(NotationStrings::getShortNoteName
                                       (Note
                                        (Note::Shortest), true)),
                       frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 1, 4);

    if (editable) {
        m_fractionLabel = 0;
        m_fraction = new QSpinBox(frame);
        m_fraction->setMinValue(1);
        connect(m_fraction, SIGNAL(valueChanged(int)),
                this, SLOT(slotBarBeatOrFractionChanged(int)));
        layout->addWidget(m_fraction, 1, 5);
    } else {
        m_fraction = 0;
        m_fractionLabel = new QLineEdit(frame);
        m_fractionLabel->setReadOnly(true);
        layout->addWidget(m_fractionLabel, 1, 5);
    }

    m_timeSig = new QLabel(frame);
    layout->addWidget(m_timeSig, 1, 6);

    label = new QLabel(i18n("Seconds:"), frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 2, 0);

    if (editable) {
        m_secLabel = 0;
        m_sec = new QSpinBox(frame);
        if (m_isDuration)
            m_sec->setMinValue(0);
        connect(m_sec, SIGNAL(valueChanged(int)),
                this, SLOT(slotSecOrMSecChanged(int)));
        layout->addWidget(m_sec, 2, 1);
    } else {
        m_sec = 0;
        m_secLabel = new QLineEdit(frame);
        m_secLabel->setReadOnly(true);
        layout->addWidget(m_secLabel, 2, 1);
    }

    label = new QLabel(i18n("msec:"), frame);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(label, 2, 2);

    if (editable) {
        m_msecLabel = 0;
        m_msec = new QSpinBox(frame);
        m_msec->setMinValue(0);
        m_msec->setLineStep(10);
        connect(m_msec, SIGNAL(valueChanged(int)),
                this, SLOT(slotSecOrMSecChanged(int)));
        layout->addWidget(m_msec, 2, 3);
    } else {
        m_msec = 0;
        m_msecLabel = new QLineEdit(frame);
        m_msecLabel->setReadOnly(true);
        layout->addWidget(m_msecLabel, 2, 3);
    }

    if (m_isDuration) {
        m_tempo = new QLabel(frame);
        layout->addWidget(m_tempo, 2, 6);
    } else {
        m_tempo = 0;
    }

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
            m_timeT->setMinValue(0);
            if (m_constrain) {
                m_timeT->setMaxValue(m_composition->getEndMarker() - m_startTime);
            } else {
                m_timeT->setMaxValue(INT_MAX);
            }
            m_timeT->setValue(m_time);
        }

        if (m_note) {
            m_note->setCurrentItem(0);
            for (size_t i = 0; i < m_noteDurations.size(); ++i) {
                if (m_time == m_noteDurations[i]) {
                    m_note->setCurrentItem(i);
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
            m_bar->setMinValue(0);
            if (m_constrain) {
                m_bar->setMaxValue
                    (m_composition->getBarNumber(m_composition->getEndMarker()) -
                     m_composition->getBarNumber(m_startTime));
            } else {
                m_bar->setMaxValue(9999);
            }
            m_bar->setValue(bars);
        } else {
            m_barLabel->setText(QString("%1").arg(bars));
        }

        if (m_beat) {
            m_beat->setMinValue(0);
            m_beat->setMaxValue(timeSig.getBeatsPerBar() - 1);
            m_beat->setValue(beats);
        } else {
            m_beatLabel->setText(QString("%1").arg(beats));
        }

        if (m_fraction) {
            m_fraction->setMinValue(0);
            m_fraction->setMaxValue(timeSig.getBeatDuration() /
                                    Note(Note::Shortest).
                                    getDuration() - 1);
            m_fraction->setValue(hemidemis);
        } else {
            m_fractionLabel->setText(QString("%1").arg(hemidemis));
        }

        m_timeSig->setText(i18n("(%1/%2 time)").arg(timeSig.getNumerator()).
                           arg(timeSig.getDenominator()));

        timeT endTime = m_startTime + m_time;

        RealTime rt = m_composition->getRealTimeDifference
                      (m_startTime, endTime);

        if (m_sec) {
            m_sec->setMinValue(0);
            if (m_constrain) {
                m_sec->setMaxValue(m_composition->getRealTimeDifference
                                   (m_startTime, m_composition->getEndMarker()).sec);
            } else {
                m_sec->setMaxValue(9999);
            }
            m_sec->setValue(rt.sec);
        } else {
            m_secLabel->setText(QString("%1").arg(rt.sec));
        }

        if (m_msec) {
            m_msec->setMinValue(0);
            m_msec->setMaxValue(999);
            m_msec->setValue(rt.msec());
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
                m_tempo->setText(i18n("(starting %1.%2 qpm, %2.%3 bpm)").
                                 arg(qpmc / 100).
                                 arg(qpmc % 100).
                                 arg(bpmc / 100).
                                 arg(bpmc % 100));
            } else {
                m_tempo->setText(i18n("(starting %1.%2 bpm)").
                                 arg(bpmc / 100).
                                 arg(bpmc % 100));
            }
        } else {
            if (bpmc != qpmc) {
                m_tempo->setText(i18n("(%1.%2 qpm, %2.%3 bpm)").
                                 arg(qpmc / 100).
                                 arg(qpmc % 100).
                                 arg(bpmc / 100).
                                 arg(bpmc % 100));
            } else {
                m_tempo->setText(i18n("(%1.%2 bpm)").
                                 arg(bpmc / 100).
                                 arg(bpmc % 100));
            }
        }

    } else {

        if (m_time > m_composition->getEndMarker()) {
            m_time = m_composition->getEndMarker();
        }

        if (m_timeT) {
            if (m_constrain) {
                m_timeT->setMinValue(m_composition->getStartMarker());
                m_timeT->setMaxValue(m_composition->getEndMarker());
            } else {
                m_timeT->setMinValue(INT_MIN);
                m_timeT->setMaxValue(INT_MAX);
            }
            m_timeT->setValue(m_time);
        }

        int bar = 1, beat = 1, hemidemis = 0, remainder = 0;
        m_composition->getMusicalTimeForAbsoluteTime
        (m_time, bar, beat, hemidemis, remainder);

        TimeSignature timeSig =
            m_composition->getTimeSignatureAt(m_time);

        if (m_bar) {
            m_bar->setMinValue(INT_MIN);
            if (m_constrain) {
                m_bar->setMaxValue(m_composition->getBarNumber
                                   (m_composition->getEndMarker()));
            } else {
                m_bar->setMaxValue(9999);
            }
            m_bar->setValue(bar + 1);
        } else {
            m_barLabel->setText(QString("%1").arg(bar + 1));
        }

        if (m_beat) {
            m_beat->setMinValue(1);
            m_beat->setMaxValue(timeSig.getBeatsPerBar());
            m_beat->setValue(beat);
        } else {
            m_beatLabel->setText(QString("%1").arg(beat));
        }

        if (m_fraction) {
            m_fraction->setMinValue(0);
            m_fraction->setMaxValue(timeSig.getBeatDuration() /
                                    Note(Note::Shortest).
                                    getDuration() - 1);
            m_fraction->setValue(hemidemis);
        } else {
            m_fractionLabel->setText(QString("%1").arg(hemidemis));
        }

        m_timeSig->setText(i18n("(%1/%2 time)").arg(timeSig.getNumerator()).
                           arg(timeSig.getDenominator()));

        RealTime rt = m_composition->getElapsedRealTime(m_time);

        if (m_sec) {
            m_sec->setMinValue(INT_MIN);
            if (m_constrain) {
                m_sec->setMaxValue(m_composition->getElapsedRealTime
                                   (m_composition->getEndMarker()).sec);
            } else {
                m_sec->setMaxValue(9999);
            }
            m_sec->setValue(rt.sec);
        } else {
            m_secLabel->setText(QString("%1").arg(rt.sec));
        }

        if (m_msec) {
            m_msec->setMinValue(0);
            m_msec->setMaxValue(999);
            m_msec->setValue(rt.msec());
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
TimeWidget::slotSetTime(timeT t)
{
    bool change = (m_time != t);
    if (!change)
        return ;
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

    slotSetTime(t);
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

}
#include "TimeWidget.moc"
