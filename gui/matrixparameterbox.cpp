// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qcheckbox.h>

#include "SnapGrid.h"

#include "instrumentparameterbox.h"
#include "matrixparameterbox.h"
#include "notepixmapfactory.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"

#include "Quantizer.h"
#include "Selection.h"
#include "MappedEvent.h"


using std::cout;
using std::cerr;
using std::endl;

using Rosegarden::Note;


MatrixParameterBox::MatrixParameterBox(QWidget *parent,
                                       RosegardenGUIDoc *doc):
    QVBox(parent),
    m_quantizations(
            Rosegarden::StandardQuantization::getStandardQuantizations()),
    m_doc(doc)
{
    setFrameStyle(NoFrame);
    initBox();
}


MatrixParameterBox::~MatrixParameterBox()
{
}

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
MatrixParameterBox::setSelection(Rosegarden::EventSelection *selection)
{
    if (!selection) return;

    Rosegarden::EventSelection::eventcontainer::iterator
        it = selection->getSegmentEvents().begin();

    for (; it != selection->getSegmentEvents().end(); it++)
    {
    }

}

void
MatrixParameterBox::useInstrument(Rosegarden::Instrument *instrument)
{
    m_instrumentParameterBox->useInstrument(instrument);
}


