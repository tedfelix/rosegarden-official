// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include "segmentparameterbox.h"
#include <qhbox.h>
#include <qlayout.h>

SegmentParameterBox::SegmentParameterBox(QWidget *parent,
                                         const char *name,
                                         WFlags f)
  :QFrame(parent, name, f)
{
    setMinimumSize(130, 100);
    setMaximumSize(130, 100);

    initBox();
}


SegmentParameterBox::~SegmentParameterBox()
{
}

void
SegmentParameterBox::initBox()
{
    // font
    QFont font ("lucidasanstypewriter", 8);
    font.setPixelSize(10);

    QGridLayout *gridLayout = new QGridLayout(this, 2, 2, 8, 1);

    QLabel *repeatLabel = new QLabel("Repeat", this);
    QLabel *quantizeLabel = new QLabel("Quantize", this);
    QLabel *transposeLabel = new QLabel("Transpose", this);

    //repeatLabel->setMaximumWidth(30);
    //quantizeLabel->setMaximumWidth(30);
    //transposeLabel->setMaximumWidth(30);

    m_repeatValue = new QLabel("-", this);
    m_repeatValue->setFont(font);

    m_quantizeValue = new QComboBox(false, this); // motif style read-only
    m_quantizeValue->setFont(font);
    m_quantizeValue->setMaximumWidth(60);
    m_quantizeValue->setMaximumHeight(20);

    m_transposeValue = new QComboBox(false, this); // motif style read-only
    m_transposeValue->setFont(font);
    m_transposeValue->setMaximumWidth(60);
    m_transposeValue->setMaximumHeight(20);

    repeatLabel->setFont(font);
    quantizeLabel->setFont(font);
    transposeLabel->setFont(font);

    QLabel *title = new QLabel("Segment Parameters", this);
    title->setFont(font);

    //gridLayout->setRowStretch(0, 1);

    gridLayout->addMultiCellWidget(title, 0, 0, 0, 1, AlignLeft);
    gridLayout->addWidget(repeatLabel, 1, 0, AlignLeft);
    gridLayout->addWidget(m_repeatValue, 1, 1, AlignCenter);
    gridLayout->addWidget(quantizeLabel, 2, 0, AlignLeft);
    gridLayout->addWidget(m_quantizeValue, 2, 1, AlignCenter);
    gridLayout->addWidget(transposeLabel, 3, 0, AlignLeft);
    gridLayout->addWidget(m_transposeValue, 3, 1, AlignCenter);
}



