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

#include "quantizevalues.h"

SegmentParameterBox::SegmentParameterBox(QWidget *parent,
                                         const char *name,
                                         WFlags f)
  :QFrame(parent, name, f)
{
    setMinimumWidth(130);
    setMaximumWidth(130);

    initBox();
}


SegmentParameterBox::~SegmentParameterBox()
{
}

void
SegmentParameterBox::initBox()
{
    int comboWidth = 48;
    int comboHeight = 20;

    // font
    QFont font ("lucidasanstypewriter", 8);
    font.setPixelSize(10);

    QGridLayout *gridLayout = new QGridLayout(this, 2, 2, 8, 1);

    QLabel *repeatLabel = new QLabel("Repeat", this);
    QLabel *quantizeLabel = new QLabel("Quantize", this);
    QLabel *transposeLabel = new QLabel("Transpose", this);
    QLabel *delayLabel = new QLabel("Delay", this);

    m_repeatValue = new QLabel("-", this);
    m_repeatValue->setFont(font);

    m_quantizeValue = new QComboBox(false, this); // motif style read-only
    m_quantizeValue->setFont(font);
    m_quantizeValue->setMinimumSize(comboWidth, comboHeight);
    m_quantizeValue->setMaximumSize(comboWidth, comboHeight);

    m_transposeValue = new QComboBox(false, this); // motif style read-only
    m_transposeValue->setFont(font);
    m_transposeValue->setMinimumSize(comboWidth, comboHeight);
    m_transposeValue->setMaximumSize(comboWidth, comboHeight);

    m_delayValue = new QComboBox(false, this); // motif style read-only
    m_delayValue->setFont(font);
    m_delayValue->setMinimumSize(comboWidth, comboHeight);
    m_delayValue->setMaximumSize(comboWidth, comboHeight);

    repeatLabel->setFont(font);
    quantizeLabel->setFont(font);
    transposeLabel->setFont(font);
    delayLabel->setFont(font);

    QLabel *title = new QLabel("Segment Parameters", this);
    title->setFont(font);

    gridLayout->addMultiCellWidget(title, 0, 0, 0, 1, AlignLeft);

    gridLayout->addWidget(repeatLabel, 1, 0, AlignLeft);
    gridLayout->addWidget(m_repeatValue, 1, 1, AlignCenter);

    gridLayout->addWidget(quantizeLabel, 2, 0, AlignLeft);
    gridLayout->addWidget(m_quantizeValue, 2, 1, AlignRight);

    gridLayout->addWidget(transposeLabel, 3, 0, AlignLeft);
    gridLayout->addWidget(m_transposeValue, 3, 1, AlignRight);

    gridLayout->addWidget(delayLabel, 4, 0, AlignLeft);
    gridLayout->addWidget(m_delayValue, 4, 1, AlignRight);

    // populate the quantize combo
    //
    QuantizeValues qVal;
    QuantizeListIterator it;

    for (it = qVal.begin(); it != qVal.end(); it++)
    {
        m_quantizeValue->insertItem(QString(((*it).second).c_str()));
    }

    // default to last item
    m_quantizeValue->setCurrentItem(m_quantizeValue->count() - 1);

    // populate the transpose combo
    //
    //
    int range = 24;

    for(int i = -range; i < range + 1; i++)
    {
        m_transposeValue->insertItem(QString("%1").arg(i));
    }
    m_transposeValue->setCurrentItem(range);

    // single value for delay for the moment
    m_delayValue->insertItem(QString("0"));


}



