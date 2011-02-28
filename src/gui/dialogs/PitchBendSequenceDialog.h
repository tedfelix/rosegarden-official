
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/// This dialog inserts series of modulated pitch bend events
/// to a given segment. -Jani

#ifndef _RG_PITCHBENDSEQUENCEDIALOG_H_
#define _RG_PITCHBENDSEQUENCEDIALOG_H_

#include <QDialog>
#include <QDoubleSpinBox>
#include <QComboBox>

#include "base/Event.h"

namespace Rosegarden
{

class Segment;

class PitchBendSequenceDialog : public QDialog
{
    Q_OBJECT

public:
    PitchBendSequenceDialog(QWidget *parent, Segment *segment,
                            timeT startTime, timeT endTime);

public slots:
    virtual void accept();
    void slotSequencePresetChanged(int);
    void slotHelpRequested();

protected:
    Segment *m_segment;

    QDoubleSpinBox *m_prebendValue;
    QDoubleSpinBox *m_prebendDuration;
    QDoubleSpinBox *m_sequenceRampDuration;
    QDoubleSpinBox *m_sequenceEndValue;
    QDoubleSpinBox *m_resolution;
    QDoubleSpinBox *m_vibratoStartAmplitude;
    QDoubleSpinBox *m_vibratoEndAmplitude;
    QDoubleSpinBox *m_vibratoWaveLength;
    QComboBox *m_sequencePreset;

    timeT m_startTime;
    timeT m_endTime;

    enum {
        USER,
        LINEAR_RAMP,
        FAST_VIBRATO_ARM_RELEASE,
        VIBRATO,
    };
};


}

#endif
