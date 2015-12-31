
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

#ifndef RG_TIMEWIDGET_H
#define RG_TIMEWIDGET_H

#include "base/RealTime.h"
#include "base/Event.h"
#include "gui/widgets/LineEdit.h"

#include <QGroupBox>
#include <QString>

#include <vector>


class QWidget;
class QSpinBox;
class QLabel;
class QComboBox;


namespace Rosegarden
{

class Composition;


class TimeWidget : public QGroupBox
{
    Q_OBJECT
public:
    /**
     * Constructor for absolute time widget
     */
    TimeWidget(QString title,
               QWidget *parent,
               Composition *composition, // for bar/beat/msec
               timeT initialTime,
               bool editable = true,
               bool constrainToCompositionDuration = true);

    /**
     * Constructor for duration widget.  startTime is the absolute time
     * at which this duration begins, necessary so that we can show the
     * correct real-time (based on tempo at startTime) etc.
     */
    TimeWidget(QString title,
               QWidget *parent,
               Composition *composition, // for bar/beat/msec
               timeT startTime,
               timeT initialDuration,
               timeT minimumDuration,
               bool editable = true,
               bool constrainToCompositionDuration = true);

    timeT getTime();
    RealTime getRealTime();

private:

    /**
     * Update delay in msec.
     */
    static const int UPDATE_DELAY_TIME = 1500;

    /**
     * Disconnect m_timeT, m_msec, and m_delayUpdateTimer
     */
    void disconnectSignals();

    /**
     * Return a rounded msec reading from the given realTime argument.
     */
    int getRoundedMSec(RealTime rt);

signals:
    void timeChanged(timeT);
    void realTimeChanged(RealTime);

public slots:
    void slotSetTime(timeT);
    void slotSetRealTime(RealTime);
    void slotResetToDefault();

    void slotNoteChanged(int);

    /**
     * Restart the update delay timer and connect it for m_timeT.
     */
    void slotTimeTChanged(int);

    /**
     * Stop the delay timer and call slotSetTime(int)
     */
    void slotTimeTUpdate();

    void slotBarBeatOrFractionChanged(int);

    /**
     * Determine realtime based on Sec or msec update and repopulate boxes.
     */
    void slotSecOrMSecChanged(int);

    /**
     * Restart the update delay timer and connect it to m_msec.
     */
    void slotMSecChanged();

    /**
     * Stop the delay timer and call slotSecOrMSecChanged(int)
     */
    void slotMSecUpdate();

private:
    Composition *m_composition;
    bool m_isDuration;
    bool m_constrain;
    timeT m_time;
    timeT m_startTime;
    timeT m_defaultTime;
    timeT m_minimumDuration;

    QComboBox *m_note;
    QSpinBox  *m_timeT;
    QSpinBox  *m_bar;
    QSpinBox  *m_beat;
    QSpinBox  *m_fraction;
    LineEdit  *m_barLabel;
    LineEdit  *m_beatLabel;
    LineEdit  *m_fractionLabel;
    QLabel    *m_timeSig;
    QSpinBox  *m_sec;
    QSpinBox  *m_msec;
    LineEdit  *m_secLabel;
    LineEdit  *m_msecLabel;
    QLabel    *m_tempo;
    QTimer    *m_delayUpdateTimer;

    void init(bool editable);
    void populate();

    std::vector<timeT> m_noteDurations;
};


}

#endif
