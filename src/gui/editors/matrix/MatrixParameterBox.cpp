/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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
#include <QComboBox>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QLayout>
#include <QWidget>


namespace Rosegarden
{

MatrixParameterBox::MatrixParameterBox(RosegardenGUIDoc *doc,
                                       QWidget *parent, const char* name):
        QFrame(parent),
        m_quantizations(BasicQuantizer::getStandardQuantizations()),
        m_doc(doc)
{
    setObjectName(name);
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

    setContentsMargins(8, 8, 8, 8);
    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(1);

    m_instrumentParameterBox = new InstrumentParameterBox(m_doc, this);
    gridLayout->addWidget(m_instrumentParameterBox, 0, 0, 7- 0+1, 2- 1);

    setLayout(gridLayout);
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
