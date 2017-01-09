/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INSTRUMENTALIASBUTTON_H
#define RG_INSTRUMENTALIASBUTTON_H

#include <QPushButton>


namespace Rosegarden
{


class Instrument;

/** A QPushButton that's associated with an Instrument so it can manipulate the
 * instrument's alias.  Pressing it creates a LineEdit that prompts the user for
 * a new alias, the instrument is updated accordingly, and it fires the
 * changed() signal, which can be used to trigger updates in the parent.
 */
class InstrumentAliasButton : public QPushButton
{
    Q_OBJECT

public:
    InstrumentAliasButton(QWidget *parent,
                   Instrument *instrument = 0);

    void setInstrument(Instrument *instrument) { m_instrument = instrument; }

protected slots:
    void slotPressed();

private slots:
    /// Instrument is being destroyed
    void slotInstrumentGone(void);

signals:
    // The button writes changes directly to the instrument, but it
    // also emits this to let you know something has changed
    void changed();

private:
    Instrument *m_instrument;
};


}

#endif
