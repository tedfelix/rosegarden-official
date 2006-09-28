/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
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
#include "document/RosegardenGUIDoc.h"
#include <ksqueezedtextlabel.h>
#include <qfontmetrics.h>
#include <qframe.h>
#include <qlabel.h>
#include <qwidget.h>


namespace Rosegarden
{

InstrumentParameterPanel::InstrumentParameterPanel(RosegardenGUIDoc *doc,
        QWidget* parent)
        : QFrame(parent),
        m_instrumentLabel(new KSqueezedTextLabel(this)),
        m_selectedInstrument(0),
        m_doc(doc)
{
    QFontMetrics metrics(m_instrumentLabel->fontMetrics());
    int width25 = metrics.width("1234567890123456789012345");

    m_instrumentLabel->setFixedWidth(width25);
    m_instrumentLabel->setAlignment(Qt::AlignCenter);
}

void
InstrumentParameterPanel::setDocument(RosegardenGUIDoc* doc)
{
    m_doc = doc;
}

}
