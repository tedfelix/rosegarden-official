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
    m_quantizations(
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

    m_quantizeCombo = new RosegardenComboBox(false, false, this);

    m_quantizeCombo->setFont(font);
    m_quantizeCombo->setFixedHeight(comboHeight);

    // populate the quantize combo
    //
    NotePixmapFactory npf;
    QPixmap noMap = npf.makeToolbarPixmap("menu-no-note");

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {
        std::string noteName = m_quantizations[i].noteName;
        QString qname = strtoqstr(m_quantizations[i].name);
        QPixmap pmap = noMap;
        if (noteName != "") {
            noteName = "menu-" + noteName;
            pmap = npf.makeToolbarPixmap(strtoqstr(noteName));
        }
        m_quantizeCombo->insertItem(pmap, qname);
    }
    m_quantizeCombo->insertItem(noMap, i18n("Off"));

    connect(m_quantizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotQuantizeSelected(int)));

    // default to last item
    m_quantizeCombo->setCurrentItem(m_quantizeCombo->count() - 1);

    QLabel *snapGridLabel = new QLabel(i18n("Snap grid"), this);
    m_snapGridCombo = new RosegardenComboBox(false, false, this);

    for (Rosegarden::timeT i = 0; i < 3840; i += 960)
    {
        m_snapValues.push_back(i);
        m_snapGridCombo->insertItem(QString("Snap %1").arg(i));
    }

    connect(m_snapGridCombo, SIGNAL(activated(int)),
            this, SLOT(slotSetSnap(int)));

    // Insert everything
    gridLayout->addRowSpacing(0, 8);
    gridLayout->addWidget(quantizeLabel, 1, 0, AlignLeft|AlignTop);
    gridLayout->addWidget(m_quantizeCombo, 1, 1, AlignTop);
    gridLayout->addWidget(snapGridLabel, 2, 0, AlignLeft|AlignTop);
    gridLayout->addWidget(m_snapGridCombo, 2, 1, AlignTop);


}


void
MatrixParameterBox::setSelection(Rosegarden::EventSelection *selection)
{
}

void
MatrixParameterBox::slotQuantizeSelected(int q)
{
    emit quantizeSelection(m_quantizations[q]);
}

void
MatrixParameterBox::slotSetSnap(int s)
{
    emit modifySnapTime(m_snapValues[s]);
}




