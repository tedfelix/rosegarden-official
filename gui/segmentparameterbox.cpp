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

#include <klocale.h>

#include "Segment.h"
#include "Quantizer.h"
#include "NotationTypes.h"

#include "notepixmapfactory.h"
#include "segmentcommands.h"
#include "rosegardenguiview.h"

using Rosegarden::Note;

SegmentParameterBox::SegmentParameterBox(RosegardenGUIView *view,
                                         QWidget *parent,
                                         const char *name,
                                         WFlags)
    : QGroupBox(i18n("Segment Parameters"), parent, name),
      m_standardQuantizations(Rosegarden::StandardQuantization::getStandardQuantizations()),
      m_view(view),
      m_tranposeRange(24)

{
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
    QFont ourFont = font();
    ourFont.setPointSize(10);
    ourFont.setBold(true);
    setFont(ourFont);

    QFont font;
    font.setPointSize(10);

    QFontMetrics fontMetrics(font);
    // magic numbers: 13 is the height of the menu pixmaps, 10 is just 10
    int comboHeight = std::max(fontMetrics.height(), 13) + 10;

    QGridLayout *gridLayout = new QGridLayout(this, 5, 2, 8, 1);

    QLabel *repeatLabel    = new QLabel(i18n("Repeat"), this);
    QLabel *quantizeLabel  = new QLabel(i18n("Quantize"), this);
    QLabel *transposeLabel = new QLabel(i18n("Transpose"), this);
    QLabel *delayLabel     = new QLabel(i18n("Delay"), this);

    m_repeatValue = new RosegardenTristateCheckBox(this);
    m_repeatValue->setFont(font);
    m_repeatValue->setFixedHeight(comboHeight);

    // handle state changes
    connect(m_repeatValue, SIGNAL(pressed()), SLOT(slotRepeatPressed()));

    // non-reversing motif style read-only combo
    m_quantizeValue = new RosegardenComboBox(false, false, this);
    m_quantizeValue->setFont(font);
    m_quantizeValue->setFixedHeight(comboHeight);

    // handle quantize changes from drop down
    connect(m_quantizeValue, SIGNAL(activated(int)),
            SLOT(slotQuantizeSelected(int)));

    // handle quantize changes from mouse wheel
    connect(m_quantizeValue, SIGNAL(propagate(int)),
            SLOT(slotQuantizeSelected(int)));

    // reversing motif style read-write combo
    m_transposeValue = new RosegardenComboBox(true, true, this);
    m_transposeValue->setFont(font);
    m_transposeValue->setFixedHeight(comboHeight);

    // handle transpose combo changes
    connect(m_transposeValue, SIGNAL(activated(int)),
            SLOT(slotTransposeSelected(int)));

    // handle transpose combo changes
    connect(m_transposeValue, SIGNAL(propagate(int)),
            SLOT(slotTransposeSelected(int)));

    // and text changes
    connect(m_transposeValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotTransposeTextChanged(const QString&)));

    // reversing motif style read-write combo
    m_delayValue = new RosegardenComboBox(true, true, this);
    m_delayValue->setFont(font);
    m_delayValue->setFixedHeight(comboHeight);

    // handle delay combo changes
    connect(m_delayValue, SIGNAL(activated(int)),
            SLOT(slotDelaySelected(int)));
    //
    // handle text changes for delay
    connect(m_delayValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotDelayTextChanged(const QString &)));

    repeatLabel->setFont(font);
    quantizeLabel->setFont(font);
    transposeLabel->setFont(font);
    delayLabel->setFont(font);

    gridLayout->addRowSpacing(0, 8);

    gridLayout->addWidget(repeatLabel,   1, 0, AlignLeft);
    gridLayout->addWidget(m_repeatValue, 1, 1, AlignLeft);

    gridLayout->addWidget(quantizeLabel,   2, 0, AlignLeft);
    gridLayout->addWidget(m_quantizeValue, 2, 1);

    gridLayout->addWidget(transposeLabel,   3, 0, AlignLeft);
    gridLayout->addWidget(m_transposeValue, 3, 1);

    gridLayout->addWidget(delayLabel,   4, 0, AlignLeft);
    gridLayout->addWidget(m_delayValue, 4, 1);

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
    m_quantizeValue->insertItem(noMap, i18n("Off"));

    // default to last item
    m_quantizeValue->setCurrentItem(m_quantizeValue->count() - 1);

    // populate the transpose combo
    //
    for(int i = -m_tranposeRange; i < m_tranposeRange + 1; i++)
    {
        m_transposeValue->insertItem(QString("%1").arg(i));
    }
    m_transposeValue->setCurrentItem(-1);

    // initial delay values as function of sequencer resolution
    for(int i = 0; i < 4; i++)
    {
	// this could be anything
	Rosegarden::timeT time = Note(Note::Crotchet).getDuration() * i;
	
	// check if it's a valid note duration (it will be for the
	// time defn above, but if we were basing it on the sequencer
	// resolution it might not be) & include a note pixmap if so
	// 
	Note nearestNote = Note::getNearestNote(time);
	if (nearestNote.getDuration() == time) {
	    std::string noteName = nearestNote.getReferenceName(); 
	    noteName = "menu-" + noteName;
	    QPixmap pmap = npf.makeToolbarPixmap(noteName.c_str());
	    m_delayValue->insertItem(pmap, QString("%1").arg(time));
	} else {
	    m_delayValue->insertItem(QString("%1").arg(time));
	}	    
    }

    // set delay blank initially
    m_delayValue->setCurrentItem(-1);

//     // set widths
//     int comboWidth = 0;
//     if (m_quantizeValue->width() > comboWidth) comboWidth = m_quantizeValue->width();
//     if (m_transposeValue->width() > comboWidth) comboWidth = m_transposeValue->width();
//     if (m_delayValue->width() > comboWidth) comboWidth = m_delayValue->width();

//     m_quantizeValue->setMinimumWidth(comboWidth);
//     m_transposeValue->setMinimumWidth(comboWidth);
//     m_delayValue->setMinimumWidth(comboWidth);
}

void
SegmentParameterBox::useSegment(Rosegarden::Segment *segment)
{
    m_segments.clear();
    m_segments.push_back(segment);
    populateBoxFromSegments();
}

void 
SegmentParameterBox::useSegments(const Rosegarden::SegmentSelection &segments)
{
    m_segments.clear();
    for (Rosegarden::SegmentSelection::const_iterator i = segments.begin();
	 i != segments.end(); ++i) {
	m_segments.push_back(*i);
    }
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
    bool off = (qLevel == m_quantizeValue->count() - 1);

    SegmentChangeQuantizationCommand *command =
	new SegmentChangeQuantizationCommand
	(off ? 0 : &m_standardQuantizations[qLevel]);

    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++)
    {
	command->addSegment(*it);
    }

    //!!! okay, what to do with the command -- got to get the 
    // command history from the document -- do it here or emit
    // signal & do it elsewhere?
    //
    addCommandToHistory(command);
    
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

    NotePixmapFactory npf;
    QPixmap pmap = npf.makeToolbarPixmap("menu-no-note");
    Note nearestNote = Note::getNearestNote(delayValue);
    if (nearestNote.getDuration() == delayValue) {
	std::string noteName = nearestNote.getReferenceName(); 
	noteName = "menu-" + noteName;
	pmap = npf.makeToolbarPixmap(noteName.c_str());
    }
    //!!! Now, how to get pmap into the pixmap part of the text field?



    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++)
        (*it)->setDelay(delayValue);

}

void
SegmentParameterBox::slotDelaySelected(int value)
{
    slotDelayTextChanged(m_delayValue->text(value));
} 

MultiViewCommandHistory*
SegmentParameterBox::getCommandHistory()
{
        return m_view->getDocument()->getCommandHistory();
}

void
SegmentParameterBox::addCommandToHistory(KCommand *command)
{
        m_view->getCommandHistory()->addCommand(command);
}
