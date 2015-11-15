/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InstrumentAliasButton.h"

#include "base/Instrument.h"
#include "base/Studio.h"
#include "gui/widgets/InputDialog.h"
#include "misc/Strings.h"

#include <QString>
#include <QPushButton>


namespace Rosegarden
{

InstrumentAliasButton::InstrumentAliasButton(QWidget *parent,
                               Instrument *instrument) :
        QPushButton(parent),
        m_instrument(instrument)
{
    connect(this, SIGNAL(clicked()), this, SLOT(slotPressed()));
    if (instrument) {
        // Make instrument tell us if it gets destroyed.
        connect(instrument, SIGNAL(destroyed()),
                this, SLOT(slotInstrumentGone()));
    }
}

void
InstrumentAliasButton::slotPressed()
{
    if (!m_instrument) return;

    bool ok = false;

    QString newAlias = InputDialog::getText(this,
                                            tr("Rosegarden"),
                                            tr("Enter instrument alias:"),
                                            LineEdit::Normal,
                                            strtoqstr(m_instrument->getAlias()),
                                            &ok);
    if (ok) {
        m_instrument->setAlias(newAlias.toStdString());
        emit changed();
    }
}

void
InstrumentAliasButton::
slotInstrumentGone(void)
{
    m_instrument = 0;
}

}
