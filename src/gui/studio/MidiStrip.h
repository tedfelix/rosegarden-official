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

#ifndef RG_MIDISTRIP_H
#define RG_MIDISTRIP_H

#include "base/Instrument.h"  // InstrumentId

#include <QTimer>
#include <QWidget>

class QVBoxLayout;

#include <vector>


namespace Rosegarden
{


class Fader;
class MidiMixerVUMeter;
class Rotary;


/// A strip of controls on the MIDI Mixer window.
class MidiStrip : public QWidget
{
    Q_OBJECT

public:

    MidiStrip(QWidget *parent, InstrumentId instrumentID, int stripNumber);

private slots:

    void slotFaderLevelChanged(float value);
    void slotControllerChanged(float value);

    /// Update the strip to match changes to the Instrument.
    /**
     * Connected to InstrumentStaticSignals::controlChange().
     */
    void slotControlChange(Instrument *instrument, const int controllerNumber);

    void slotUpdateMeter();

private:

    InstrumentId m_id{0};

    // Widgets
    MidiMixerVUMeter *m_vuMeter{nullptr};
    Fader *m_volumeFader{nullptr};
    std::vector<Rotary *> m_controllerRotaries;

    void createWidgets(int stripNumber);

    /// Meter update timer.
    QTimer m_timer;

};


}

#endif
