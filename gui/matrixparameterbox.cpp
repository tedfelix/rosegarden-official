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

#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>

#include "matrixparameterbox.h"
#include "notepixmapfactory.h"
#include "rosestrings.h"


using std::cout;
using std::cerr;
using std::endl;

MatrixParameterBox::MatrixParameterBox(QWidget *parent):
    RosegardenParameterBox(i18n("Parameters "), parent),
    m_standardQuantizations(
        Rosegarden::StandardQuantization::getStandardQuantizations())
{
    initBox();
}


MatrixParameterBox::~MatrixParameterBox()
{
}

void
MatrixParameterBox::initBox()
{
    QFont font(getFont());

    QFontMetrics fontMetrics(font);
    // magic numbers: 13 is the height of the menu pixmaps, 10 is just 10
    int comboHeight = std::max(fontMetrics.height(), 13) + 10;

    QGridLayout *gridLayout = new QGridLayout(this, 5, 2, 8, 1);

    QLabel *quantizeLabel  = new QLabel(i18n("Quantize"), this);
    quantizeLabel->setFont(font);

    m_quantizeValue = new RosegardenComboBox(false, false, this);

    m_quantizeValue->setFont(font);
    m_quantizeValue->setFixedHeight(comboHeight);

    // populate the quantize combo
    //
    NotePixmapFactory npf;
    QPixmap noMap = npf.makeToolbarPixmap("menu-no-note");

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
        std::string noteName = m_standardQuantizations[i].noteName;
        QString qname = strtoqstr(m_standardQuantizations[i].name);
        QPixmap pmap = noMap;
        if (noteName != "") {
            noteName = "menu-" + noteName;
            pmap = npf.makeToolbarPixmap(strtoqstr(noteName));
        }
        m_quantizeValue->insertItem(pmap, qname);
    }
    m_quantizeValue->insertItem(noMap, i18n("Off"));

    // default to last item
    m_quantizeValue->setCurrentItem(m_quantizeValue->count() - 1);

    // Insert everything
    gridLayout->addRowSpacing(0, 8);
    gridLayout->addWidget(quantizeLabel, 1, 0, AlignLeft|AlignTop);
    gridLayout->addWidget(m_quantizeValue, 1, 1, AlignTop);


}




