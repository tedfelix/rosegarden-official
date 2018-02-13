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
 * It would be nice if Qt offered proper static signals so that
 * this class wasn't necessary.  An alternative non-static design
 * would be to have observers connect directly to the Instrument
 * objects.  Studio would need to let the observers know when
 * new Instruments were created.
 *
 * OTOH, pulling signals out of data objects like Instrument eliminates
 * the need for data objects to derive from QObject.  This is makes
 * it possible to copy data objects freely.
 *
 */
class InstrumentStaticSignals : public QObject
{
    Q_OBJECT

public:

    /*
     * When connecting, use Instrument::getStaticSignals() to get
     * the object instance.  Search the codebase on
     * "Instrument::getStaticSignals()" for connect() examples.
     *
     * ??? Future Direction: Instrument::getStaticSignals() should be moved
     *     into here and renamed "self()" or "instance()".
     */

    /// Public wrapper since Qt4 signals are protected.
    /**
     * Emits controlChange().  See comments on controlChange() for more.
     */
    void emitControlChange(Instrument *instrument, int cc)
        { emit controlChange(instrument, cc); }

signals:
    /// Fine-grain, high-frequency notification mechanism.
    /**
     * Call this if you change the value for a control change for
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
     * notifications, we can avoid updating too much of the UI when
     * these come in.  This should improve performance.
     *
     * Note that this only applies to the initial control change
     * on a Track.  This has nothing to do with any control changes
     * that may appear on the rulers as the Composition progresses.
     */
    void controlChange(Instrument *instrument, int cc);

};


}

#endif // RG_INSTRUMENTSTATICSIGNALS_H
