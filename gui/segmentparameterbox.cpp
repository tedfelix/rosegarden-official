// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <qhbox.h>
#include <qlayout.h>
#include <qinputdialog.h>

#include <klocale.h>
#include <kcommand.h>
#include <kcombobox.h>

#include "Segment.h"
#include "Quantizer.h"
#include "NotationTypes.h"

#include "colours.h"
#include "notepixmapfactory.h"
#include "segmentcommands.h"
#include "rosestrings.h"
#include "segmentparameterbox.h"
#include "notationstrings.h"
#include "rosegardenguiview.h"

using Rosegarden::Note;

SegmentParameterBox::SegmentParameterBox(RosegardenGUIView *view,
                                         QWidget *parent)
    : RosegardenParameterBox(i18n("Segment Parameters"), parent),
      m_standardQuantizations(Rosegarden::BasicQuantizer::getStandardQuantizations()),
      m_view(view),
      m_tranposeRange(24)
{
    initBox();

    connect(getCommandHistory(), SIGNAL(commandExecuted()),
	    this, SLOT(update()));
}


SegmentParameterBox::~SegmentParameterBox()
{
    delete m_repeatValue;
    delete m_quantizeValue;
    delete m_transposeValue;
    delete m_delayValue;
    delete m_colourValue;
}

void
SegmentParameterBox::initBox()
{
    QFont font(getFont());

    QFontMetrics fontMetrics(font);
    // magic numbers: 13 is the height of the menu pixmaps, 10 is just 10
    int comboHeight = std::max(fontMetrics.height(), 13) + 10;

    QGridLayout *gridLayout = new QGridLayout(this, 6, 2, 8, 1);

    QLabel *label = new QLabel(i18n("Label"), this);
    QLabel *repeatLabel    = new QLabel(i18n("Repeat"), this);
    QLabel *quantizeLabel  = new QLabel(i18n("Quantize"), this);
    QLabel *transposeLabel = new QLabel(i18n("Transpose"), this);
    QLabel *delayLabel     = new QLabel(i18n("Delay"), this);
    QLabel *colourLabel    = new QLabel(i18n("Color"), this);

    // HBox for label
    //
    QHBox *hbox = new QHBox(this);

    // Label ..
    m_label = new QLabel(hbox);
    m_label->setFont(font);
    m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_label->setFixedWidth(100);

    // .. and edit button
    m_labelButton = new QPushButton("...", hbox);
    m_labelButton->setFont(font);

    connect(m_labelButton, SIGNAL(released()),
            SLOT(slotEditSegmentLabel()));

    m_repeatValue = new RosegardenTristateCheckBox(this);
    m_repeatValue->setFont(font);
    m_repeatValue->setFixedHeight(comboHeight);

    // handle state changes
    connect(m_repeatValue, SIGNAL(pressed()), SLOT(slotRepeatPressed()));

    // non-reversing motif style read-only combo
    m_quantizeValue = new KComboBox(false, this);
    m_quantizeValue->setFont(font);
    m_quantizeValue->setFixedHeight(comboHeight);

    // handle quantize changes from drop down
    connect(m_quantizeValue, SIGNAL(activated(int)),
            SLOT(slotQuantizeSelected(int)));

    // reversing motif style read-write combo
    m_transposeValue = new RosegardenComboBox(true, true, this);
    m_transposeValue->setFont(font);
    m_transposeValue->setFixedHeight(comboHeight);

    // handle transpose combo changes
    connect(m_transposeValue, SIGNAL(activated(int)),
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

    // Detect when the document colours are updated
    connect (m_view->getDocument(), SIGNAL(docColoursChanged()),
             this, SLOT(slotDocColoursChanged()));

    // handle text changes for delay
    connect(m_delayValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotDelayTextChanged(const QString &)));

    // set up combo box for colours
    m_colourValue = new KComboBox(false, this);
    m_colourValue->setFont(font);
    m_colourValue->setFixedHeight(comboHeight);

    // handle colour combo changes
    connect(m_colourValue, SIGNAL(activated(int)),
            SLOT(slotColourSelected(int)));



    label->setFont(font);
    repeatLabel->setFont(font);
    quantizeLabel->setFont(font);
    transposeLabel->setFont(font);
    delayLabel->setFont(font);
    colourLabel->setFont(font);

    gridLayout->addRowSpacing(0, 8);

    gridLayout->addWidget(label,         1, 0, AlignLeft);
    gridLayout->addWidget(hbox,       1, 1, AlignLeft);

    gridLayout->addWidget(repeatLabel,   2, 0, AlignLeft);
    gridLayout->addWidget(m_repeatValue, 2, 1, AlignLeft);

    gridLayout->addWidget(quantizeLabel,   3, 0, AlignLeft);
    gridLayout->addWidget(m_quantizeValue, 3, 1);

    gridLayout->addWidget(transposeLabel,   4, 0, AlignLeft);
    gridLayout->addWidget(m_transposeValue, 4, 1);

    gridLayout->addWidget(delayLabel,   5, 0, AlignLeft);
    gridLayout->addWidget(m_delayValue, 5, 1);

    gridLayout->addWidget(colourLabel,   6, 0, AlignLeft);
    gridLayout->addWidget(m_colourValue, 6, 1);

    // populate the quantize combo
    //
    NotePixmapFactory npf;
    QPixmap noMap = NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap("menu-no-note"));

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {

	Rosegarden::timeT time = m_standardQuantizations[i];
	Rosegarden::timeT error = 0;
	QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
	QPixmap pmap = NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(time, error));
	m_quantizeValue->insertItem(error ? noMap : pmap, label);
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

    m_delays.clear();

    for (int i = 0; i < 9; i++)
    {
	Rosegarden::timeT time = 0;
	if (i > 0 && i < 6) {
	    time = Note(Note::Hemidemisemiquaver).getDuration() << (i-1);
	} else if (i > 5) {
	    time = Note(Note::Crotchet).getDuration() * (i-4);
	}

	m_delays.push_back(time);
	
	// check if it's a valid note duration (it will be for the
	// time defn above, but if we were basing it on the sequencer
	// resolution it might not be) & include a note pixmap if so
	// 
	Rosegarden::timeT error = 0;
	QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
	QPixmap pmap = NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(time, error));
	m_delayValue->insertItem((error ? noMap : pmap), label);
    }

    for (int i = 0; i < 10; i++)
    {
	int rtd = (i < 5 ? ((i + 1) * 10) : ((i - 3) * 50));
	m_realTimeDelays.push_back(rtd);
	m_delayValue->insertItem(i18n("%1 ms").arg(rtd));
    }

    // set delay blank initially
    m_delayValue->setCurrentItem(-1);

    // populate m_colourValue
    slotDocColoursChanged();

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

    m_segments.resize(segments.size());
    std::copy(segments.begin(), segments.end(), m_segments.begin());

    populateBoxFromSegments();
}

void
SegmentParameterBox::slotDocColoursChanged()
{
    m_colourValue->clear();
    m_colourList.clear();
    // Populate it from composition.m_segmentColourMap
    Rosegarden::ColourMap temp = m_view->getDocument()->getComposition().getSegmentColourMap();

    unsigned int i=0;

    for (Rosegarden::RCMap::const_iterator it=temp.begin(); it != temp.end(); ++it)
    {
        QPixmap colour(15,15);
        colour.fill(RosegardenGUIColours::convertColour(it->second.first));
        if (it->second.second == std::string(""))
            m_colourValue->insertItem(colour, i18n("Default Color"), i);
        else
            m_colourValue->insertItem(colour, strtoqstr(it->second.second), i);
        m_colourList[it->first] = i;
        ++i;
    }

    m_colourValue->setCurrentItem(0);
}

void SegmentParameterBox::update()
{
    RG_DEBUG << "SegmentParameterBox::update()\n";

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
    Tristate repeated = NotApplicable;
    Tristate quantized = NotApplicable;
    Tristate transposed = NotApplicable;
    Tristate delayed = NotApplicable;
    Tristate diffcolours = NotApplicable;
    unsigned int myCol = 0;

    Rosegarden::timeT qntzLevel = 0;
    // At the moment we have no negative delay, so we use negative
    // values to represent real-time delay in ms
    Rosegarden::timeT delayLevel = 0;
    int transposeLevel = 0;

    if (m_segments.size() == 0)
        m_label->setText("");
    else
        m_label->setText(strtoqstr(m_segments[0]->getLabel()));

    for (it = m_segments.begin(); it != m_segments.end(); it++)
    {
	// ok, first thing is we know we have at least one segment
	if (repeated == NotApplicable) repeated = None;
	if (quantized == NotApplicable) quantized = None;
	if (transposed == NotApplicable) transposed = None;
	if (delayed == NotApplicable) delayed = None;
	if (diffcolours == NotApplicable) diffcolours = None;

        // Set label to "*" when multiple labels don't match
        //
        if (strtoqstr((*it)->getLabel()) != m_label->text())
            m_label->setText("*");

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
                qntzLevel = (*it)->getQuantizer()->getUnit();
            }
            else
            {
                // If quantize levels don't match
                if (quantized == None ||
                        (quantized == All &&
                            qntzLevel !=
                            (*it)->getQuantizer()->getUnit()))
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
	Rosegarden::timeT myDelay = (*it)->getDelay();
	if (myDelay == 0) {
	    myDelay = -((*it)->getRealTimeDelay().sec * 1000 +
			(*it)->getRealTimeDelay().usec / 1000);
	}

        if (myDelay != 0) 
        {
            if (it == m_segments.begin())
            {
                delayed = All;
                delayLevel = myDelay;
            }
            else
            {
                if (delayed == None ||
                        (delayed == All &&
                             delayLevel != myDelay))
                    delayed = Some;
            }
        }
        else
        {
            if (delayed == All)
                delayed = Some;
        }

        // Colour

        if (it == m_segments.begin())
        {
            myCol = (*it)->getColourIndex();
        }
        else
        {
            if (myCol != (*it)->getColourIndex());
                diffcolours = All;
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
        case NotApplicable:
        default:
            m_repeatValue->setChecked(false);
            break;
    }

    m_repeatValue->setEnabled(repeated != NotApplicable);

    switch(quantized)
    {
        case All:
            {
                for (unsigned int i = 0;
                     i < m_standardQuantizations.size(); ++i)
                {
                    if (m_standardQuantizations[i] == qntzLevel)
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

    m_quantizeValue->setEnabled(quantized != NotApplicable);

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

    m_transposeValue->setEnabled(transposed != NotApplicable);

    m_delayValue->blockSignals(true);
    
    switch(delayed)
    {
        case All:

	    if (delayLevel > 0) {

		NotePixmapFactory npf;
		Rosegarden::timeT error = 0;
		QString label = NotationStrings::makeNoteMenuLabel(delayLevel, true, error);
		m_delayValue->setEditText(label);

	    } else if (delayLevel < 0) {

		m_delayValue->setEditText(i18n("%1 ms").arg(-delayLevel));
	    }

            break;

        case Some:
        case None:
        default:
            m_delayValue->setEditText("");
            break;
    }

    m_delayValue->setEnabled(delayed != NotApplicable);

    m_delayValue->blockSignals(false);

    switch(diffcolours)
    {
        case None:
            if (m_colourList.find(myCol) != m_colourList.end())
                m_colourValue->setCurrentItem(m_colourList[myCol]);
            else
                m_colourValue->setCurrentItem(0);
            break;


        case All:
        case NotApplicable:
        default:
            m_colourValue->setCurrentItem(0);
            break;

    }

    m_colourValue->setEnabled(diffcolours != NotApplicable);

}

void SegmentParameterBox::slotRepeatPressed()
{
    if (m_segments.size() == 0) return;

    bool state = false;

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

    addCommandToHistory(new SegmentCommandRepeat(m_segments, state));

//     std::vector<Rosegarden::Segment*>::iterator it;

//     for (it = m_segments.begin(); it != m_segments.end(); it++)
//         (*it)->setRepeating(state);
}

// Set the quantize level for the selected Segments
//
void
SegmentParameterBox::slotQuantizeSelected(int qLevel)
{
    bool off = (qLevel == m_quantizeValue->count() - 1);

    SegmentChangeQuantizationCommand *command =
	new SegmentChangeQuantizationCommand
	(off ? 0 : m_standardQuantizations[qLevel]);

    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++)
    {
	command->addSegment(*it);
    }

    addCommandToHistory(command);
}



void
SegmentParameterBox::slotTransposeTextChanged(const QString &text)
{
    if (text.isEmpty() || m_segments.size() == 0)
        return;

    int transposeValue = text.toInt();

//     addCommandToHistory(new SegmentCommandChangeTransposeValue(m_segments,
//                                                                transposeValue));

    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        (*it)->setTranspose(transposeValue);
    }

    emit documentModified();
}

void 
SegmentParameterBox::slotTransposeSelected(int value)
{
    slotTransposeTextChanged(m_transposeValue->text(value));
}


void
SegmentParameterBox::slotDelayTimeChanged(Rosegarden::timeT delayValue)
{
    // by convention and as a nasty hack, we use negative timeT here
    // to represent positive RealTime in ms
    
    if (delayValue > 0) {

	std::vector<Rosegarden::Segment*>::iterator it;
	for (it = m_segments.begin(); it != m_segments.end(); it++) {
	    (*it)->setDelay(delayValue);
	    (*it)->setRealTimeDelay(Rosegarden::RealTime(0, 0));
	}

    } else if (delayValue < 0) {
	
	std::vector<Rosegarden::Segment*>::iterator it;
	for (it = m_segments.begin(); it != m_segments.end(); it++) {
	    (*it)->setDelay(0);
	    int sec = (-delayValue) / 1000;
	    int usec = ((-delayValue) - 1000*sec) * 1000;
	    (*it)->setRealTimeDelay(Rosegarden::RealTime(sec, usec));
	}
    } else {

	std::vector<Rosegarden::Segment*>::iterator it;
	for (it = m_segments.begin(); it != m_segments.end(); it++) {
	    (*it)->setDelay(0);
	    (*it)->setRealTimeDelay(Rosegarden::RealTime(0, 0));
	}
    }

    emit documentModified();
}

void
SegmentParameterBox::slotDelayTextChanged(const QString &text)
{
    if (text.isEmpty() || m_segments.size() == 0)
        return;

    slotDelayTimeChanged(-(text.toInt()));
} 

void
SegmentParameterBox::slotDelaySelected(int value)
{
    if (value < int(m_delays.size())) {
	slotDelayTimeChanged(m_delays[value]);
    } else {
	slotDelayTimeChanged(-(m_realTimeDelays[value - m_delays.size()]));
    }
} 

void
SegmentParameterBox::slotColourSelected(int value)
{
    unsigned int temp;

    RosegardenColourTable::ColourList::const_iterator pos = m_colourList.find(value);

    if (pos != m_colourList.end())
        temp = pos->first;
    else // Somehow we are trying to set a colour which doesn't exist
        temp = 0;

    Rosegarden::SegmentSelection segments;
    std::vector<Rosegarden::Segment*>::iterator it;

    for (it = m_segments.begin(); it != m_segments.end(); ++it)
    {
       segments.insert(*it);
    }

    SegmentColourCommand *command = new SegmentColourCommand(segments, temp);

    addCommandToHistory(command);

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

void
SegmentParameterBox::slotEditSegmentLabel()
{
    QString editLabel;

    if (m_segments.size() == 0) return;
    else if (m_segments.size() == 1) editLabel = i18n("Modify Segment label");
    else editLabel = i18n("Modify Segments label");

    bool ok = false;

    // Remove the asterisk if we're using it
    //
    QString label = m_label->text();
    if (label == "*") label = "";

    QString newLabel = QInputDialog::getText(
            editLabel,
            i18n("Enter new label"),
            QLineEdit::Normal,
            m_label->text(),
            &ok,
            this);

    if (ok)
    {
        Rosegarden::SegmentSelection segments;
        std::vector<Rosegarden::Segment*>::iterator it;
        for (it = m_segments.begin(); it != m_segments.end(); ++it)
            segments.insert(*it);

        SegmentLabelCommand *command = new
            SegmentLabelCommand(segments, newLabel);

        addCommandToHistory(command);
    }
}

