// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include <klocale.h>
#include <kcommand.h>
#include <kcombobox.h>
#include <kcolordialog.h>
#include <klineeditdlg.h>

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

SegmentParameterBox::SegmentParameterBox(RosegardenGUIDoc* doc,
                                         QWidget *parent)
    : RosegardenParameterBox(i18n("Segment Parameters"), parent),
      m_standardQuantizations(Rosegarden::BasicQuantizer::getStandardQuantizations()),
      m_doc(doc),
      m_transposeRange(24)
{
    initBox();

    m_doc->getComposition().addObserver(this);

    connect(getCommandHistory(), SIGNAL(commandExecuted()),
	    this, SLOT(update()));
}


SegmentParameterBox::~SegmentParameterBox()
{
    m_doc->getComposition().removeObserver(this);
}


void
SegmentParameterBox::initBox()
{
    QFont font(m_font);

    QFontMetrics fontMetrics(font);
    // magic numbers: 13 is the height of the menu pixmaps, 10 is just 10
    int comboHeight = std::max(fontMetrics.height(), 13) + 10;

//    QFrame *frame = new QFrame(this);
    QGridLayout *gridLayout = new QGridLayout(this, 6, 4, 4, 2);

    QLabel *label = new QLabel(i18n("Label"), this);
    QLabel *repeatLabel    = new QLabel(i18n("Repeat"), this);
    QLabel *quantizeLabel  = new QLabel(i18n("Quantize"), this);
    QLabel *transposeLabel = new QLabel(i18n("Transpose"), this);
    QLabel *delayLabel     = new QLabel(i18n("Delay"), this);
    QLabel *colourLabel    = new QLabel(i18n("Color"), this);
    m_autoFadeLabel        = new QLabel(i18n("Audio auto-fade"), this);
    m_fadeInLabel          = new QLabel(i18n("Fade in"), this);
    m_fadeOutLabel         = new QLabel(i18n("Fade out"), this);

    // HBox for label
    //
    QHBox *hbox = new QHBox(this);

    // Label ..
    m_label = new QLabel(hbox);
    m_label->setFont(font);
    m_label->setFixedWidth(120);
    m_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    // .. and edit button
    m_labelButton = new QPushButton("...", hbox);
    m_labelButton->setFont(font);
    m_labelButton->setFixedWidth(25);

    connect(m_labelButton, SIGNAL(released()),
            SLOT(slotEditSegmentLabel()));

    m_repeatValue = new RosegardenTristateCheckBox(this);
    m_repeatValue->setFont(font);
    m_repeatValue->setFixedHeight(comboHeight);

    // handle state changes
    connect(m_repeatValue, SIGNAL(pressed()), SLOT(slotRepeatPressed()));

    // non-reversing motif style read-only combo
    m_quantizeValue = new KComboBox(this);
    m_quantizeValue->setFont(font);
    m_quantizeValue->setFixedHeight(comboHeight);

    // handle quantize changes from drop down
    connect(m_quantizeValue, SIGNAL(activated(int)),
            SLOT(slotQuantizeSelected(int)));

    // reversing motif style read-write combo
    m_transposeValue = new KComboBox(this);
    m_transposeValue->setFont(font);
    m_transposeValue->setFixedHeight(comboHeight);

    // handle transpose combo changes
    connect(m_transposeValue, SIGNAL(activated(int)),
            SLOT(slotTransposeSelected(int)));

    // and text changes
    connect(m_transposeValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotTransposeTextChanged(const QString&)));

    // reversing motif style read-write combo
    m_delayValue = new KComboBox(this);
    m_delayValue->setFont(font);
    m_delayValue->setFixedHeight(comboHeight);

    // handle delay combo changes
    connect(m_delayValue, SIGNAL(activated(int)),
            SLOT(slotDelaySelected(int)));

    // Detect when the document colours are updated
    connect(m_doc, SIGNAL(docColoursChanged()),
            this, SLOT(slotDocColoursChanged()));

    // handle text changes for delay
    connect(m_delayValue, SIGNAL(textChanged(const QString&)),
            SLOT(slotDelayTextChanged(const QString &)));

    // set up combo box for colours
    m_colourValue = new KComboBox(this);
    m_colourValue->setFont(font);
    m_colourValue->setFixedHeight(comboHeight);

    // handle colour combo changes
    connect(m_colourValue, SIGNAL(activated(int)),
            SLOT(slotColourSelected(int)));

    // Audio autofade enabled
    //
    m_autoFadeBox = new QCheckBox(this);
    connect(m_autoFadeBox, SIGNAL(stateChanged(int)),
            this, SLOT(slotAudioFadeChanged(int)));

    // Fade in and out times
    //
    m_fadeInSpin = new QSpinBox(this);
    m_fadeInSpin->setMinValue(0);
    m_fadeInSpin->setMaxValue(5000);
    m_fadeInSpin->setSuffix(i18n(" ms"));
    connect(m_fadeInSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotFadeInChanged(int)));

    m_fadeOutSpin = new QSpinBox(this);
    m_fadeOutSpin->setMinValue(0);
    m_fadeOutSpin->setMaxValue(5000);
    m_fadeOutSpin->setSuffix(i18n(" ms"));
    connect(m_fadeOutSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotFadeOutChanged(int)));

    label->setFont(font);
    repeatLabel->setFont(font);
    quantizeLabel->setFont(font);
    transposeLabel->setFont(font);
    delayLabel->setFont(font);
    colourLabel->setFont(font);
    m_autoFadeLabel->setFont(font);
    m_fadeInLabel->setFont(font);
    m_fadeOutLabel->setFont(font);

    gridLayout->addRowSpacing(0, 12);

    gridLayout->addWidget(label, 1, 0, AlignRight);
    gridLayout->addMultiCellWidget(hbox, 1, 1, 1, 3, AlignLeft);

    gridLayout->addWidget(repeatLabel, 2, 0, AlignRight);
    gridLayout->addWidget(m_repeatValue, 2, 1, AlignLeft);

    gridLayout->addWidget(transposeLabel, 2, 2, AlignRight);
    gridLayout->addWidget(m_transposeValue, 2, 3);

    gridLayout->addWidget(quantizeLabel, 3, 0, AlignRight);
    gridLayout->addWidget(m_quantizeValue, 3, 1);

    gridLayout->addWidget(delayLabel, 3, 2, AlignRight);
    gridLayout->addWidget(m_delayValue, 3, 3);

    gridLayout->addWidget(colourLabel, 4, 0, AlignRight);
    gridLayout->addMultiCellWidget(m_colourValue, 4, 4, 1, 3);

    m_autoFadeLabel->hide();
    m_autoFadeBox->hide();

    gridLayout->addWidget(m_fadeInLabel,   5, 0, AlignRight);
    gridLayout->addWidget(m_fadeInSpin,  5, 1);

    gridLayout->addWidget(m_fadeOutLabel,  5, 2, AlignRight);
    gridLayout->addWidget(m_fadeOutSpin, 5, 3);

    // populate the quantize combo
    //
    QPixmap noMap = NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("menu-no-note"));

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {

	Rosegarden::timeT time = m_standardQuantizations[i];
	Rosegarden::timeT error = 0;
	QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
	QPixmap pmap = NotePixmapFactory::toQPixmap(NotePixmapFactory::makeNoteMenuPixmap(time, error));
	m_quantizeValue->insertItem(error ? noMap : pmap, label);
    }
    m_quantizeValue->insertItem(noMap, i18n("Off"));

    // default to last item
    m_quantizeValue->setCurrentItem(m_quantizeValue->count() - 1);

    // populate the transpose combo
    //
    for(int i = -m_transposeRange; i < m_transposeRange + 1; i++)
    {
        m_transposeValue->insertItem(noMap, QString("%1").arg(i));
	if (i == 0) m_transposeValue->setCurrentItem(m_transposeValue->count() - 1);
    }

    m_delays.clear();

    for (int i = 0; i < 6; i++)
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
	QPixmap pmap = NotePixmapFactory::toQPixmap(NotePixmapFactory::makeNoteMenuPixmap(time, error));
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
SegmentParameterBox::setDocument(RosegardenGUIDoc* doc)
{
    if (m_doc != 0)
        disconnect(m_doc, SIGNAL(docColoursChanged()),
                   this, SLOT(slotDocColoursChanged()));
        
    m_doc = doc;

    // Detect when the document colours are updated
    connect (m_doc, SIGNAL(docColoursChanged()),
             this, SLOT(slotDocColoursChanged()));

    slotDocColoursChanged(); // repopulate combo
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
    RG_DEBUG << "SegmentParameterBox::slotDocColoursChanged()" << endl;
	
    m_colourValue->clear();
    m_colourList.clear();
    // Populate it from composition.m_segmentColourMap
    Rosegarden::ColourMap temp = m_doc->getComposition().getSegmentColourMap();

    unsigned int i=0;

    for (Rosegarden::RCMap::const_iterator it=temp.begin(); it != temp.end(); ++it)
    {
        QPixmap colour(15,15);
        colour.fill(Rosegarden::GUIPalette::convertColour(it->second.first));
        if (it->second.second == std::string(""))
            m_colourValue->insertItem(colour, i18n("Default Color"), i);
        else
            m_colourValue->insertItem(colour, strtoqstr(it->second.second), i);
        m_colourList[it->first] = i; // maps colour number to menu index
        ++i;
    }

    m_addColourPos = i;
    m_colourValue->insertItem(i18n("Add New Color"), m_addColourPos);

    m_colourValue->setCurrentItem(0);
}

void SegmentParameterBox::update()
{
    RG_DEBUG << "SegmentParameterBox::update()" << endl;

    populateBoxFromSegments();
}

void
SegmentParameterBox::segmentRemoved(const Rosegarden::Composition *composition,
				    Rosegarden::Segment *segment)
{
    if (composition == &m_doc->getComposition()) {

	for (std::vector<Rosegarden::Segment*>::iterator it =
		 m_segments.begin(); it != m_segments.end(); ++it) {

	    if (*it == segment) {
		m_segments.erase(it);
		return;
	    }
	}
    }
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
			(*it)->getRealTimeDelay().msec());
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
	// setCurrentItem works with QStrings
	// 2nd arg of "true" means "add if necessary"
        case All:
            m_transposeValue->
		setCurrentItem(QString("%1").arg(transposeLevel),true);
            break;

        case Some:
            m_transposeValue->setCurrentItem(QString(""),true);
	    break;

        case None:
        default:
            m_transposeValue->setCurrentItem("0");
            break;
    }

    m_transposeValue->setEnabled(transposed != NotApplicable);

    m_delayValue->blockSignals(true);
    
    switch(delayed)
    {
        case All:
	    if (delayLevel >= 0) {
		Rosegarden::timeT error = 0;
		QString label = NotationStrings::makeNoteMenuLabel(delayLevel,
								   true,
								   error);
		m_delayValue->setCurrentItem(label,true);

	    } else if (delayLevel < 0) {

		m_delayValue->setCurrentItem(i18n("%1 ms").arg(-delayLevel),
					     true);
	    }

            break;

        case Some:
            m_delayValue->setCurrentItem("",true);
            break;

        case None:
        default:
            m_delayValue->setCurrentItem(0);
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

    // Enable or disable the fade in/out params
    if (m_segments.size() == 1 && 
        (*(m_segments.begin()))->getType() == Rosegarden::Segment::Audio)
    {
        m_autoFadeBox->blockSignals(true);
        m_fadeInSpin->blockSignals(true);
        m_fadeOutSpin->blockSignals(true);

/*!!! No, not setting up autofade widgets.  The implementation's too
      incomplete to finish for this release.

      (Or for the next one after the one the previous comment referred to.)

        m_fadeInLabel->show();
        m_fadeInSpin->show();
        m_fadeOutLabel->show();
        m_fadeOutSpin->show();

	instead:
*/
        m_fadeInLabel->hide();
        m_fadeInSpin->hide();
        m_fadeOutLabel->hide();
        m_fadeOutSpin->hide();

        m_autoFadeLabel->setEnabled(true);
        m_autoFadeBox->setEnabled(true);
        m_fadeInLabel->setEnabled(true);
        m_fadeInSpin->setEnabled(true);
        m_fadeOutLabel->setEnabled(true);
        m_fadeOutSpin->setEnabled(true);

        Rosegarden::Segment *seg = *(m_segments.begin());

        int fadeInTime = seg->getFadeInTime().sec * 1000 +
            seg->getFadeInTime().msec();
        m_fadeInSpin->setValue(fadeInTime);

        int fadeOutTime = seg->getFadeOutTime().sec * 1000 +
            seg->getFadeOutTime().msec();
        m_fadeOutSpin->setValue(fadeOutTime);

        m_autoFadeBox->setChecked(seg->isAutoFading());

        m_autoFadeBox->blockSignals(false);
        m_fadeInSpin->blockSignals(false);
        m_fadeOutSpin->blockSignals(false);
    }
    else
    {
        m_autoFadeLabel->setEnabled(false);
        m_autoFadeBox->setEnabled(false);
        m_fadeInLabel->setEnabled(false);
        m_fadeInSpin->setEnabled(false);
        m_fadeOutLabel->setEnabled(false);
        m_fadeOutSpin->setEnabled(false);

        m_autoFadeLabel->hide();
        m_autoFadeBox->hide();
        m_fadeInLabel->hide();
        m_fadeInSpin->hide();
        m_fadeOutLabel->hide();
        m_fadeOutSpin->hide();

        m_autoFadeBox->setChecked(false);
        m_fadeInSpin->setValue(0);
        m_fadeOutSpin->setValue(0);
    }


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
	    (*it)->setRealTimeDelay(Rosegarden::RealTime::zeroTime);
	}

    } else if (delayValue < 0) {
	
	std::vector<Rosegarden::Segment*>::iterator it;
	for (it = m_segments.begin(); it != m_segments.end(); it++) {
	    (*it)->setDelay(0);
	    int sec = (-delayValue) / 1000;
	    int nsec = ((-delayValue) - 1000*sec) * 1000000;
	    (*it)->setRealTimeDelay(Rosegarden::RealTime(sec, nsec));
	}
    } else {

	std::vector<Rosegarden::Segment*>::iterator it;
	for (it = m_segments.begin(); it != m_segments.end(); it++) {
	    (*it)->setDelay(0);
	    (*it)->setRealTimeDelay(Rosegarden::RealTime::zeroTime);
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
    if (value != m_addColourPos)
    {
        unsigned int temp = 0;

	RosegardenColourTable::ColourList::const_iterator pos;
	for (pos = m_colourList.begin(); pos != m_colourList.end(); ++pos) {
	    if (pos->second == value) {
		temp = pos->first;
		break;
	    }
	}

        Rosegarden::SegmentSelection segments;
        std::vector<Rosegarden::Segment*>::iterator it;

        for (it = m_segments.begin(); it != m_segments.end(); ++it)
        {
           segments.insert(*it);
        }

        SegmentColourCommand *command = new SegmentColourCommand(segments, temp);

        addCommandToHistory(command);
    }
    else
    {
        Rosegarden::ColourMap newMap = m_doc->getComposition().getSegmentColourMap();
        QColor newColour;
        bool ok = false;
        QString newName = KLineEditDlg::getText(i18n("New Color Name"), i18n("Enter new name"),
                                                i18n("New"), &ok);
        if ((ok == true) && (!newName.isEmpty()))
        {
            KColorDialog box(this, "", true);

            int result = box.getColor(newColour);

            if (result == KColorDialog::Accepted)
            {
                Rosegarden::Colour newRColour = Rosegarden::GUIPalette::convertColour(newColour);
                newMap.addItem(newRColour, qstrtostr(newName));
                SegmentColourMapCommand *command = new SegmentColourMapCommand(m_doc, newMap);
                addCommandToHistory(command);
                slotDocColoursChanged();
            }
        }
        // Else we don't do anything as they either didn't give a name·
        //  or didn't give a colour
    }


}

MultiViewCommandHistory*
SegmentParameterBox::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

void
SegmentParameterBox::addCommandToHistory(KCommand *command)
{
    m_doc->getCommandHistory()->addCommand(command);
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

    QString newLabel = KLineEditDlg::getText(editLabel,
                                             i18n("Enter new label"),
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


void
SegmentParameterBox::slotAudioFadeChanged(int value)
{
    RG_DEBUG << "SegmentParameterBox::slotAudioFadeChanged - value = "
             << value << endl;

    if (m_segments.size() == 0)
        return;

    bool state = false;
    if (value == QButton::On) state = true;

    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        (*it)->setAutoFade(state);
    }

}


void
SegmentParameterBox::slotFadeInChanged(int value)
{
    RG_DEBUG << "SegmentParameterBox::slotFadeInChanged - value = "
             << value << endl;

    if (m_segments.size() == 0)
        return;

    if (value == 0 && m_fadeOutSpin->value() == 0) slotAudioFadeChanged(QButton::Off);
    else slotAudioFadeChanged(QButton::On);

    // Convert from ms
    //
    Rosegarden::RealTime fadeInTime(value/1000, (value % 1000) * 1000000);

    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        (*it)->setFadeInTime(fadeInTime);
    }

    emit documentModified();
}

void
SegmentParameterBox::slotFadeOutChanged(int value)
{
    RG_DEBUG << "SegmentParameterBox::slotFadeOutChanged - value = "
             << value << endl;

    if (m_segments.size() == 0)
        return;

    if (value == 0 && m_fadeInSpin->value() == 0) slotAudioFadeChanged(QButton::Off);
    else slotAudioFadeChanged(QButton::On);

    // Convert from ms
    //
    Rosegarden::RealTime fadeOutTime(value/1000000, (value % 1000) * 10000000);

    std::vector<Rosegarden::Segment*>::iterator it;
    for (it = m_segments.begin(); it != m_segments.end(); it++) {
        (*it)->setFadeOutTime(fadeOutTime);
    }

    emit documentModified();
}

#include "segmentparameterbox.moc"
