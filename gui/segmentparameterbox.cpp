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
#include "NotationTypes.h"

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
    connect(m_repeatValue, SIGNAL(pressed()), SLOT(slotRepeatPressed()));

    // non-reversing motif style read-only combo
    m_quantizeValue = new RosegardenComboBox(false, false, this);
    m_quantizeValue->setFont(plainFont);
    m_quantizeValue->setFixedSize(comboWidth, comboHeight);

    // handle quantize changes from drop down
    connect(m_quantizeValue, SIGNAL(activated(int)),
            SLOT(slotQuantizeSelected(int)));

    // handle quantize changes from mouse wheel
    connect(m_quantizeValue, SIGNAL(propagate(int)),
            SLOT(slotQuantizeSelected(int)));

    // reversing motif style read-write combo
    m_transposeValue = new RosegardenComboBox(true, true, this);
    m_transposeValue->setFont(plainFont);
    m_transposeValue->setFixedSize(comboWidth, comboHeight);

    // handle transpose combo changes
    connect(m_transposeValue, SIGNAL(activated(int)),
            SLOT(slotTransposeSelected(int)));

    // and text changes
    connect(m_transposeValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotTransposeTextChanged(const QString&)));

    // reversing motif style read-write combo
    m_delayValue = new RosegardenComboBox(true, true, this);
    m_delayValue->setFont(plainFont);
    m_delayValue->setFixedSize(comboWidth, comboHeight);

    // handle delay combo changes
    connect(m_delayValue, SIGNAL(activated(int)),
            SLOT(slotDelaySelected(int)));
    //
    // handle text changes for delay
    connect(m_delayValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotDelayTextChanged(const QString &)));

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
    m_transposeValue->setCurrentItem(-1);

    // initial delay values as function of sequencer resolution
    for(int i = 0; i < 4; i++)
    {
        m_delayValue->insertItem(QString("%1").
                arg(Rosegarden::Note(Rosegarden::Note::Crotchet, false).
                    getDuration() * i));
    }

    // set delay blank initially
    m_delayValue->setCurrentItem(-1);

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
// this box.  All fields (whether they're Checkbox or Combobox)
// are tristate because we can have "All", "Some" or "None" of
// the selected Segments at a given state.
//
void
SegmentParameterBox::populateBoxFromSegments()
{
    std::vector<Rosegarden::Segment*>::iterator it;
    Tristate repeated = None;
    Tristate quantized = None;
    Tristate transposed = None;
    Tristate delayed = None;

    Rosegarden::Quantizer qntzLevel;
    Rosegarden::timeT delayLevel = 0;
    int transposeLevel = 0;

    for (it = m_segments.begin(); it != m_segments.end(); it++)
    {
        // Are all, some or none of the Segments repeating?
        if ((*it)->isRepeating())
        {
            if (it == m_segments.begin())
                repeated = All;
            else
            {
                if (repeated == None)
                    repeated = Some;
            }
        }
        else
        {
            if (repeated == All)
                repeated = Some;
        }

        // Quantization
        //
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

        // Transpose 
        //
        if ((*it)->getTranspose() != 0)
        {
            if (it == m_segments.begin())
            {
                transposed = All;
                transposeLevel = (*it)->getTranspose();
            }
            else
            {
                if (transposed == None ||
                       (transposed == All &&
                            transposeLevel != (*it)->getTranspose()))
                    transposed = Some;
            }

        }
        else
        {
            if (transposed == All)
                transposed = Some;
        }

        // Delay
        //
        if ((*it)->getDelay() != 0)
        {
            if (it == m_segments.begin())
            {
                delayed = All;
                delayLevel = (*it)->getDelay();
            }
            else
            {
                if (delayed == None ||
                        (delayed == All &&
                             delayLevel != (*it)->getDelay()))
                    delayed = Some;
            }
        }
        else
        {
            if (delayed == All)
                delayed = Some;
        }

    }

    switch(repeated)
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

    switch(transposed)
    {
        case All:
            {
                bool setValue = false;

                for (int i = 0; i < m_transposeValue->count(); i++)
                {
                    if (m_transposeValue->text(i).toInt() == transposeLevel)
                    {
                        m_transposeValue->setCurrentItem(i);
                        setValue = true;
                    }
                }
                
                if (!setValue)
                    m_transposeValue->
                        setEditText(QString("%1").arg(transposeLevel));
            }
            break;

        case Some:
        case None:
        default:
            m_transposeValue->setEditText("");
            break;
    }

    switch(delayed)
    {
        case All:
            m_delayValue->setEditText(QString("%1").arg(delayLevel));
            break;

        case Some:
        case None:
        default:
            m_delayValue->setEditText("");
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



void
SegmentParameterBox::slotTransposeTextChanged(const QString &text)
{
    if (text.isEmpty())
        return;

    int transposeValue = text.toInt();

    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++)
        (*it)->setTranspose(transposeValue);

}

void 
SegmentParameterBox::slotTransposeSelected(int value)
{
    slotTransposeTextChanged(m_transposeValue->text(value));
}


void
SegmentParameterBox::slotDelayTextChanged(const QString &text)
{
    if (text.isEmpty())
        return;

    int delayValue = text.toInt();

    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++)
        (*it)->setDelay(delayValue);

}

void
SegmentParameterBox::slotDelaySelected(int value)
{
    slotDelayTextChanged(m_delayValue->text(value));
}



