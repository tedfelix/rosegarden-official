/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
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

/// Signals for all Instrument instances.
/**
 * Allows observers to subscribe for signals from all Instrument
 * objects.
 *
 * Previously, static signals for Instrument were kept in
 * RosegardenMainWindow.  This class moves them closer to Instrument.
 *
 * See Instrument::getStaticSignals().
 *
 * It would be nice if Qt offered proper static signals so that
 * this class wasn't necessary.  An alternative non-static design
 * would be to have observers connect directly to the Instrument
 * objects.  Studio would need to let the observers know when
 * new Instruments were created.
 */
class InstrumentStaticSignals : public QObject
{
    Q_OBJECT

signals:
    /// An Instrument object has changed.
    /**
     * When connecting, use Instrument::getStaticSignals() to get
     * the object instance.  Search the codebase on
     * "Instrument::getStaticSignals()" for connect() examples.
     *
     * Controllers can cause a very high rate of update notification when
     * the user makes rapid changes using a knob or slider.
     * Handlers of this signal should be prepared to deal with this.
     * They should check for changes relevant to them, and bail if there
     * are none.  As of this writing, only the MIPP has been rewritten
     * to deal with this properly.  All other handlers should be reviewed
     * and modified as needed.
     *
     * Formerly RosegardenMainWindow::instrumentParametersChanged().
     */
    void changed(Instrument *);

private:
    // Since Qt4 makes signals "protected" we do this to give Instrument
    // the ability to directly emit the signals.  When we upgrade to
    // Qt5, we can remove this.
    friend class Instrument;
};


}

#endif // RG_INSTRUMENTSTATICSIGNALS_H
