// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _TIME_WIDGET_H_
#define _TIME_WIDGET_H_

#include "Event.h" // for timeT

class QComboBox;
class QSpinBox;
class QLineEdit;
class QLabel;

#include <qgroupbox.h>
#include <qstring.h>

namespace Rosegarden { class Composition; }

class RosegardenTimeWidget : public QGroupBox
{
    Q_OBJECT
public:
    /**
     * Constructor for absolute time widget
     */
    RosegardenTimeWidget(QString title,
			 QWidget *parent,
			 Rosegarden::Composition *composition, // for bar/beat/msec
			 Rosegarden::timeT initialTime,
			 bool editable = true);

    /**
     * Constructor for duration widget.  startTime is the absolute time
     * at which this duration begins, necessary so that we can show the
     * correct real-time (based on tempo at startTime) etc.
     */
    RosegardenTimeWidget(QString title,
			 QWidget *parent,
			 Rosegarden::Composition *composition, // for bar/beat/msec
			 Rosegarden::timeT startTime,
			 Rosegarden::timeT initialDuration,
			 bool editable = true);

    Rosegarden::timeT getTime();
    Rosegarden::RealTime getRealTime();

signals:
    void timeChanged(Rosegarden::timeT);
    void realTimeChanged(Rosegarden::RealTime);

public slots:
    void slotSetTime(Rosegarden::timeT);
    void slotSetRealTime(Rosegarden::RealTime);
    void slotResetToDefault();

    void slotNoteChanged(int);
    void slotTimeTChanged(int);
    void slotBarBeatOrFractionChanged(int);
    void slotSecOrMSecChanged(int);

private:
    Rosegarden::Composition *m_composition;
    bool m_isDuration;
    Rosegarden::timeT m_time;
    Rosegarden::timeT m_startTime;
    Rosegarden::timeT m_defaultTime;

    QComboBox *m_note;
    QSpinBox  *m_timeT;
    QSpinBox  *m_bar;
    QSpinBox  *m_beat;
    QSpinBox  *m_fraction;
    QLineEdit *m_barLabel;
    QLineEdit *m_beatLabel;
    QLineEdit *m_fractionLabel;
    QLabel    *m_timeSig;
    QSpinBox  *m_sec;
    QSpinBox  *m_msec;
    QLineEdit *m_secLabel;
    QLineEdit *m_msecLabel;
    QLabel    *m_tempo;

    void init(bool editable);
    void populate();

    std::vector<Rosegarden::timeT> m_noteDurations;
};

#endif
