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


#include "InstrumentParameterPanel.h"

#include "base/Instrument.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/SqueezedLabel.h"
#include <QFontMetrics>
#include <QFrame>
#include <QLabel>
#include <QWidget>


namespace Rosegarden
{

InstrumentParameterPanel::InstrumentParameterPanel(RosegardenDocument *doc,
                                                   QWidget* parent) :
    QFrame(parent),
    m_instrumentLabel(new SqueezedLabel(this)),
    m_selectedInstrument(0),
    m_doc(doc)
{
}

void
InstrumentParameterPanel::setDocument(RosegardenDocument* doc)
{
    m_doc = doc;
    m_selectedInstrument = 0;
}

}

#include "InstrumentParameterPanel.moc"
