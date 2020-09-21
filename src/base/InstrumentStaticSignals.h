/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INSTRUMENTSTATICSIGNALS_H
#define RG_INSTRUMENTSTATICSIGNALS_H

#include <QObject>

namespace Rosegarden
{


class Instrument;

/// Signals related to Instrument.
/**
 * This class was created to allow for a single global signal
 * for control change notifications across all Instruments.
 *
 * As a side-effect, this also removes a signal from Instrument
 * which means Instrument has one less reason to inherit from QObject.
 * Deriving from QObject seems wrong for data objects like Instrument.
 * It prevents them from being copied.
 *
 */
class InstrumentStaticSignals : public QObject
{
    Q_OBJECT

signals:
    /// Fine-grain, high-frequency notification mechanism.
    /**
     * To emit:
     *
     *   Instrument::emitControlChange(instrument, cc);
     *
     * To connect:
     *
     *   // Qt5 style (preferred, compile-time checking)
     *   connect(Instrument::getStaticSignals().data(),
     *               &InstrumentStaticSignals::controlChange,
     *           this,  // or whomever
     *           &CLASSNAME::slotControlChange);
     *
     *   // Qt4 style (discouraged, runtime checking only)
     *   connect(Instrument::getStaticSignals().data(),
     *               SIGNAL(controlChange(Instrument *, int)),
     *           SLOT(slotControlChange(Instrument *, int)));
     *
     * Emit this if you change the value for a control change for
     * an Instrument.  This will trigger an update of relevant portions
     * of the UI (sliders and knobs) to reflect the new values.  This
     * will also trigger RosegardenSequencer to send out appropriate
     * control change messages via MIDI or update the level and pan
     * settings for audio instruments.
     *
     * Handlers should update only that part of the
     * UI that displays this specific control change value.
     *
     * This is used for control change notifications which can happen
     * very quickly as the user moves volume sliders, pan knobs, and
     * other control change knobs.
     *
     * By separating these out from the other more general update
     * notifications (e.g. RD::documentModified()), we can avoid updating
     * too much of the UI when these come in.  This should improve
     * performance.
     *
     * Note that this only applies to the initial control change
     * on a Track.  This has nothing to do with any control changes
     * that may appear on the rulers as the Composition progresses.
     */
    void controlChange(Instrument *instrument, int cc);

private:
    // Singleton.  Use Instrument::getStaticSignals() to get the instance.
    InstrumentStaticSignals()  { }
    friend Instrument;

    /// Private wrapper called by Instrument::emitControlChange().
    void emitControlChange(Instrument *instrument, int cc)
        { emit controlChange(instrument, cc); }
};


}

#endif // RG_INSTRUMENTSTATICSIGNALS_H
