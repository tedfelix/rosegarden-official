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

#include "Segment.h"
#include "Quantizer.h"

#include "notepixmapfactory.h"

SegmentParameterBox::SegmentParameterBox(QWidget *parent,
                                         const char *name,
                                         WFlags f) :
    QFrame(parent, name, f),
    m_standardQuantizations(Rosegarden::StandardQuantization::getStandardQuantizations())
{
    setFixedSize(136, 120);
    initBox();
}


SegmentParameterBox::~SegmentParameterBox()
{
    delete m_repeatValue;
    delete m_quantizeValue;
    delete m_transposeValue;
    delete m_delayValue;
}

void
SegmentParameterBox::initBox()
{
    int comboWidth = 64;
    int comboHeight = 20;

    // font
//    QFont font ("lucidasanstypewriter", 8);
//    font.setPixelSize(10);

    // bit experimental --cc
    QFont plainFont;
    plainFont.setPixelSize(10);

    QFont boldFont;
    boldFont.setPixelSize(11);
    boldFont.setBold(true);

    QGridLayout *gridLayout = new QGridLayout(this, 2, 2, 5, 1);

    QLabel *repeatLabel = new QLabel("Repeat", this);
    QLabel *quantizeLabel = new QLabel("Quantize", this);
    QLabel *transposeLabel = new QLabel("Transpose", this);
    QLabel *delayLabel = new QLabel("Delay", this);

    m_repeatValue = new RosegardenTristateCheckBox(this);
    m_repeatValue->setFont(plainFont);
    m_repeatValue->setFixedHeight(comboHeight);

    // handle state changes
    connect(m_repeatValue, SIGNAL(pressed()), SLOT(repeatPressed()));


    // motif style read-only combo
    m_quantizeValue = new RosegardenComboBox(false, this);
    m_quantizeValue->setFont(plainFont);
    m_quantizeValue->setFixedSize(comboWidth, comboHeight);

    // handle quantize changes from drop down
    connect(m_quantizeValue, SIGNAL(activated(int)),
            SLOT(slotQuantizeSelected(int)));

    // handle quantize changes from mouse wheel
    connect(m_quantizeValue, SIGNAL(propagate(int)),
            SLOT(slotQuantizeSelected(int)));

    // motif style read-only combo
    m_transposeValue = new RosegardenComboBox(true, this);
    m_transposeValue->setFont(plainFont);
    m_transposeValue->setFixedSize(comboWidth, comboHeight);

    // motif style read-only combo
    m_delayValue = new RosegardenComboBox(true, this);
    m_delayValue->setFont(plainFont);
    m_delayValue->setFixedSize(comboWidth, comboHeight);

    repeatLabel->setFont(plainFont);
    quantizeLabel->setFont(plainFont);
    transposeLabel->setFont(plainFont);
    delayLabel->setFont(plainFont);

    QLabel *title = new QLabel("Segment Parameters", this);
    title->setFont(boldFont);
    title->setFixedHeight(comboHeight);

    gridLayout->addMultiCellWidget(title, 0, 0, 0, 1, AlignLeft);

    gridLayout->addWidget(repeatLabel, 1, 0, AlignLeft);
    gridLayout->addWidget(m_repeatValue, 1, 1, AlignRight);

    gridLayout->addWidget(quantizeLabel, 2, 0, AlignLeft);
    gridLayout->addWidget(m_quantizeValue, 2, 1, AlignRight);

    gridLayout->addWidget(transposeLabel, 3, 0, AlignLeft);
    gridLayout->addWidget(m_transposeValue, 3, 1, AlignRight);

    gridLayout->addWidget(delayLabel, 4, 0, AlignLeft);
    gridLayout->addWidget(m_delayValue, 4, 1, AlignRight);

    // populate the quantize combo
    //
    NotePixmapFactory npf;
    QPixmap noMap = npf.makeToolbarPixmap("menu-no-note");

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
	std::string noteName = m_standardQuantizations[i].noteName;
	QString qname = m_standardQuantizations[i].name.c_str();
	QPixmap pmap = noMap;
	if (noteName != "") {
	    noteName = "menu-" + noteName;
	    pmap = npf.makeToolbarPixmap(noteName.c_str());
	}
	m_quantizeValue->insertItem(pmap, qname);
    }
    m_quantizeValue->insertItem(noMap, "Off");

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

void
SegmentParameterBox::useSegment(Rosegarden::Segment *segment)
{
    m_segments.clear();
    m_segments.push_back(segment);
    populateBoxFromSegments();
}

void 
SegmentParameterBox::useSegments(std::vector<Rosegarden::Segment*> segments)
{
    m_segments.clear();
    m_segments = segments;
    populateBoxFromSegments();
}

// Use the currently selected Segments to populate the fields in
// this box
//
void
SegmentParameterBox::populateBoxFromSegments()
{
    std::vector<Rosegarden::Segment*>::iterator it;
    Tristate repeat = None;
    Tristate quantized = None;
    Rosegarden::Quantizer qntzLevel;

    for (it = m_segments.begin(); it != m_segments.end(); it++)
    {
        // Are all, some or none of the Segments repeating?
        if ((*it)->isRepeating())
        {
            if (it == m_segments.begin())
                repeat = All;
            else
            {
                if (repeat == None)
                    repeat = Some;
            }
        }
        else
        {
            if (repeat == All)
                repeat = Some;
        }

        if ((*it)->hasQuantization())
        {
            if (it == m_segments.begin())
            {
                quantized = All;
                qntzLevel = (*it)->getQuantizer();
            }
            else
            {
                // If quantize levels don't match
                if (quantized == None ||
                        (quantized == All &&
                            qntzLevel.getUnit() !=
                            (*it)->getQuantizer().getUnit()))
                    quantized = Some;
            }
        }
        else
        {
            if (quantized == All)
                quantized = Some;
        }

    }

    switch(repeat)
    {
        case All:
            m_repeatValue->setChecked(true);
            break;

        case Some:
            m_repeatValue->setNoChange();
            break;

        case None:
        default:
            m_repeatValue->setChecked(false);
            break;
    }

    switch(quantized)
    {
        case All:
            {
                for (unsigned int i = 0;
                     i < m_standardQuantizations.size(); ++i)
                {
                    if (m_standardQuantizations[i].unit == qntzLevel.getUnit())
                    {
                        m_quantizeValue->setCurrentItem(i);
                        break;
                    }
                }
            }
            break;

        case Some:
            // Set the edit text to an unfeasible blank value meaning "Some"
            //
            m_quantizeValue->setCurrentItem(-1);
            break;

            // Assuming "Off" is always the last field
        case None:
        default:
            m_quantizeValue->setCurrentItem(m_quantizeValue->count()-1);
            break;
    }
}

void
SegmentParameterBox::slotRepeatPressed()
{
    bool state;

    switch(m_repeatValue->state())
    {
        case QButton::Off:
            state = true;
            break;

        case QButton::NoChange:
        case QButton::On:
        default:
            state = false;
            break;
    }

    // update the check box and all current Segments
    m_repeatValue->setChecked(state);

    std::vector<Rosegarden::Segment*>::iterator it;

    for (it = m_segments.begin(); it != m_segments.end(); it++)
        (*it)->setRepeating(state);
}

// Set the quantize level for the selected Segments
//
void
SegmentParameterBox::slotQuantizeSelected(int qLevel)
{
    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++)
    {
        (*it)->setQuantization(true);
        (*it)->setQuantizeLevel(m_standardQuantizations[qLevel]);
    }
}





