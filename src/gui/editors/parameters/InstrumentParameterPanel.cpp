/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InstrumentParameterPanel.h"

#include "base/Instrument.h"
#include "gui/widgets/SqueezedLabel.h"
#include "gui/application/RosegardenMainWindow.h"

#include <QFrame>
#include <QWidget>


namespace Rosegarden
{

InstrumentParameterPanel::InstrumentParameterPanel(QWidget *parent) :
    QFrame(parent),
    m_instrumentLabel(new SqueezedLabel(this)),
    m_selectedInstrument(nullptr)
{
}

void
InstrumentParameterPanel::setSelectedInstrument(Instrument *instrument)
{
    m_selectedInstrument = instrument;
    if (instrument) {
        // Make instrument tell us if it gets destroyed.
        connect(instrument, &QObject::destroyed,
                this, &InstrumentParameterPanel::slotInstrumentGone);
    }
}

Instrument *
InstrumentParameterPanel::getSelectedInstrument()
{
    return m_selectedInstrument;
}

void
InstrumentParameterPanel::
slotInstrumentGone()
{
    m_selectedInstrument = nullptr;
    m_instrumentLabel->setText(tr("none"));
}


}

