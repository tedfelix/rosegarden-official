// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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
#include "Quantizer.h"
#include "Selection.h"
#include "MappedEvent.h"


using std::cout;
using std::cerr;
using std::endl;

using Rosegarden::Note;


MatrixParameterBox::MatrixParameterBox(QWidget *parent):
    QFrame(parent),
    m_quantizations(
            Rosegarden::StandardQuantization::getStandardQuantizations())
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
    int comboHeight = std::max(fontMetrics.height(), 13) + 10;

    QGridLayout *gridLayout = new QGridLayout(this, 20, 3, 8, 1);

    /*
    QLabel *quantizeLabel  = new QLabel(i18n("Quantize positions"), this);
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

    // mouse wheel
    connect(m_quantizeCombo, SIGNAL(propagate(int)),
            this, SLOT(slotQuantizeSelected(int)));

    // default to last item
    m_quantizeCombo->setCurrentItem(m_quantizeCombo->count() - 1);

    QLabel *snapGridLabel = new QLabel(i18n("Snap division"), this);
    snapGridLabel->setFont(font);

    m_snapGridCombo = new RosegardenComboBox(false, false, this);
    m_snapGridCombo->setFont(font);

    Rosegarden::timeT crotchetDuration = Note(Note::Crotchet).getDuration();
    m_snapValues.push_back(Rosegarden::SnapGrid::NoSnap);
    m_snapValues.push_back(Rosegarden::SnapGrid::SnapToUnit);
    m_snapValues.push_back(crotchetDuration / 96);
    m_snapValues.push_back(crotchetDuration / 48);
    m_snapValues.push_back(crotchetDuration / 24);
    m_snapValues.push_back(crotchetDuration / 16);
    m_snapValues.push_back(crotchetDuration / 8);
    m_snapValues.push_back(crotchetDuration / 4);
    m_snapValues.push_back(crotchetDuration / 2);
    m_snapValues.push_back(Rosegarden::SnapGrid::SnapToBeat);
    m_snapValues.push_back(Rosegarden::SnapGrid::SnapToBar);

    for (unsigned int i = 0; i < m_snapValues.size(); i++)
    {
        Note nearestNote = Note::getNearestNote(m_snapValues[i]);
        if (nearestNote.getDuration() == m_snapValues[i])
        {
            QString pmapName = "menu-" +
                strtoqstr(nearestNote.getReferenceName());
            QPixmap pmap = npf.makeToolbarPixmap(pmapName);

            m_snapGridCombo->insertItem(pmap,
                    strtoqstr(nearestNote.getShortName()));
        }
        else
        {
            QString noteName = "";

            if (m_snapValues[i] == Rosegarden::SnapGrid::NoSnap)
                noteName = i18n("None");
            else if (m_snapValues[i] == Rosegarden::SnapGrid::SnapToUnit)
                noteName = i18n("Unit");
            else if (m_snapValues[i] == Rosegarden::SnapGrid::SnapToBeat)
                noteName = i18n("Beat");
            else if (m_snapValues[i] == Rosegarden::SnapGrid::SnapToBar)
                noteName = i18n("Bar");

            if (noteName == "")
            {
                // attempt to construct a meaningful note length but just
                // without the pixmap

//                float fValue = float(crotchetDuration) / float(m_snapValues[i]);
                int iValue = crotchetDuration / m_snapValues[i];

		if (iValue * m_snapValues[i] == crotchetDuration) {
                    noteName = i18n("1/%1").arg(iValue);
		} else {
                    noteName = i18n("%1 ticks").arg(m_snapValues[i]);
		}
            }


            m_snapGridCombo->insertItem(noteName);
        }
    }

    connect(m_snapGridCombo, SIGNAL(activated(int)),
            this, SLOT(slotSetSnap(int)));

    connect(m_snapGridCombo, SIGNAL(propagate(int)),
            this, SLOT(slotSetSnap(int)));
    */

    m_instrumentParameterBox = new InstrumentParameterBox(0, this);

    connect(m_instrumentParameterBox,
            SIGNAL(sendMappedEvent(Rosegarden::MappedEvent*)),
            SIGNAL(sendMappedEvent(Rosegarden::MappedEvent*)));

    connect(m_instrumentParameterBox,
            SIGNAL(sendMappedInstrument(const Rosegarden::MappedInstrument&)),
            SIGNAL(sendMappedInstrument(const Rosegarden::MappedInstrument&)));

    // Insert everything
    gridLayout->addMultiCellWidget(m_instrumentParameterBox, 0, 7, 0, 2);

    /*
    gridLayout->addWidget(quantizeLabel, 10, 0, AlignLeft|AlignTop);
    gridLayout->addWidget(m_quantizeCombo, 10, 2, AlignTop);
    gridLayout->addWidget(snapGridLabel, 11, 0, AlignLeft|AlignTop);
    gridLayout->addWidget(m_snapGridCombo, 11, 2, AlignTop);
    */
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

/*
void
MatrixParameterBox::slotQuantizeSelected(int q)
{
    using Rosegarden::Quantizer;

    Rosegarden::timeT unit = m_quantizations[q].unit;

    // if we're the last value then we're Off
    //
    if ((unsigned int)(q) == m_quantizations.size())
    {
        unit = 0;
    }

    emit quantizeSelection(Quantizer(Quantizer::GlobalSource,
                                     Quantizer::RawEventData,
                                     Quantizer::PositionQuantize,
                                     unit,
                                     m_quantizations[q].maxDots));
}

void
MatrixParameterBox::slotSetSnap(int s)
{
    emit modifySnapTime(m_snapValues[s]);
}
*/


void
MatrixParameterBox::useInstrument(Rosegarden::Instrument *instrument)
{
    m_instrumentParameterBox->useInstrument(instrument);
}




