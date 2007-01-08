/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "MatrixParameterBox.h"

#include "base/Instrument.h"
#include "base/BasicQuantizer.h"
#include "base/Selection.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include <kcombobox.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qframe.h>
#include <qlayout.h>
#include <qwidget.h>


namespace Rosegarden
{

MatrixParameterBox::MatrixParameterBox(RosegardenGUIDoc *doc,
                                       QWidget *parent, const char* name):
        QFrame(parent, name),
        m_quantizations(BasicQuantizer::getStandardQuantizations()),
        m_doc(doc)
{
    setFrameStyle(NoFrame);
    initBox();
}

MatrixParameterBox::~MatrixParameterBox()
{}

void
MatrixParameterBox::initBox()
{
    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    boldFont.setBold(true);

    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 9 / 10);
    QFont font = plainFont;

    QFontMetrics fontMetrics(font);
    // magic numbers: 13 is the height of the menu pixmaps, 10 is just 10
    //int comboHeight = std::max(fontMetrics.height(), 13) + 10;

    QGridLayout *gridLayout = new QGridLayout(this, 20, 3, 8, 1);

    m_instrumentParameterBox = new InstrumentParameterBox(m_doc, this);
    gridLayout->addMultiCellWidget(m_instrumentParameterBox, 0, 7, 0, 2);

}

void
MatrixParameterBox::setSelection(EventSelection *selection)
{
    if (!selection)
        return ;

    EventSelection::eventcontainer::iterator
    it = selection->getSegmentEvents().begin();

for (; it != selection->getSegmentEvents().end(); it++) {}

}

void
MatrixParameterBox::useInstrument(Instrument *instrument)
{
    m_instrumentParameterBox->useInstrument(instrument);
}

}
#include "MatrixParameterBox.moc"
