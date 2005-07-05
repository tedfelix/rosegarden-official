// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file Copyright 2003
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "eventfilter.h"

#include <kapp.h>               // let the dialog remember its settings
#include <kconfig.h>            // for the next use
#include <klocale.h>
#include <qvariant.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlayout.h>
#include <qtooltip.h>

#include "midipitchlabel.h"	// number to text pitch display
#include "rosedebug.h"          // debug stream
#include "notepixmapfactory.h"  // pixmaps for duration combos
#include "Quantizer.h"          // duration combo
#include "NotationTypes.h"      // "
#include "notationstrings.h"    // "
#include "MidiTypes.h"          // event filtration
#include "BaseProperties.h"     // "
#include "Event.h"              // "
#include "rosedebug.h"          // debug stream



using Rosegarden::Note;

const char* const EventFilterDialog::ConfigGroup = "EventFilter Dialog";

// EventFilterDialog
//
// Constructs a dialog to select various criteria used to filter events out of
// selections
EventFilterDialog::EventFilterDialog(QWidget* parent)
    : KDialogBase(parent, "eventfilerdialog", true, i18n("Event Filter"), Ok|Cancel, Ok),
      m_standardQuantizations(Rosegarden::BasicQuantizer::getStandardQuantizations())
{
    cfg = kapp->config();
    initDialog();
}

// Destructor
EventFilterDialog::~EventFilterDialog()
{
    // nothing here
}

void
EventFilterDialog::initDialog()
{
    QVBox* mainWidget = makeVBoxMainWidget();


    //----------[ Note Filter Widgets ]-------------------------
    
    // Frame
    QGroupBox* noteFrame = new QGroupBox(i18n("Note Events"), mainWidget);
    QGridLayout* noteFrameLayout = new QGridLayout(noteFrame, 1, 1, 20, 6); 

    // Master Checkbox
//! getting rid of this for now because it's superfluous now that I've gotten
//rid of the Wheel and Controller sections of the dialog...  Leaving it here
//in case I come up with something later on to replace the Wheel and
//Controller sections of the dialog.
//
//    m_noteCheckBox = new QCheckBox(i18n("Enable"),noteFrame);
//    cfg->setGroup(EventFilterDialog::ConfigGroup);
//    m_noteCheckBox->setChecked(cfg->readBoolEntry("notecheckbox", true));
//    noteFrameLayout->addWidget(m_noteCheckBox, 0, 0);
//    connect(m_noteCheckBox, SIGNAL(stateChanged(int)), this,
//	    SLOT(slotNoteCheckBoxToggle(int))); 

    // Labels
    QLabel* pitchFromLabel = new QLabel(i18n("from:"),noteFrame);
    noteFrameLayout->addWidget(pitchFromLabel, 0, 2);

    QLabel* pitchToLabel = new QLabel(i18n("to:"),noteFrame);
    noteFrameLayout->addWidget(pitchToLabel, 0, 4);

    QLabel* pitchLabel = new QLabel(i18n("Pitch:"), noteFrame);
    noteFrameLayout->addWidget(pitchLabel, 1, 1);
    
    QLabel* velocityLabel = new QLabel(i18n("Velocity:"), noteFrame);
    noteFrameLayout->addWidget(velocityLabel, 2, 1);
    
    QLabel* durationLabel = new QLabel(i18n("Duration:"), noteFrame);
    noteFrameLayout->addWidget(durationLabel, 3, 1);

    // Include Boxes
    m_notePitchIncludeComboBox = new QComboBox(0, noteFrame);
    m_notePitchIncludeComboBox->insertItem(i18n("include"));
    m_notePitchIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_notePitchIncludeComboBox->setCurrentItem(cfg->readBoolEntry("pitchinclude", 0));
    noteFrameLayout->addWidget(m_notePitchIncludeComboBox, 1, 0);
    
    m_noteVelocityIncludeComboBox = new QComboBox(0, noteFrame);
    m_noteVelocityIncludeComboBox->insertItem(i18n("include"));
    m_noteVelocityIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_noteVelocityIncludeComboBox->setCurrentItem(cfg->readBoolEntry("velocityinclude", 0));
    noteFrameLayout->addWidget(m_noteVelocityIncludeComboBox, 2, 0);
    
    m_noteDurationIncludeComboBox = new QComboBox(0, noteFrame);
    m_noteDurationIncludeComboBox->insertItem(i18n("include"));
    m_noteDurationIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_noteDurationIncludeComboBox->setCurrentItem(cfg->readBoolEntry("durationinclude", 0));
    noteFrameLayout->addWidget(m_noteDurationIncludeComboBox, 3, 0);

    // Pitch From
    m_pitchFromSpinBox = new QSpinBox(noteFrame);
    m_pitchFromSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_pitchFromSpinBox->setValue(cfg->readUnsignedNumEntry("pitchfrom", 0));
    noteFrameLayout->addWidget(m_pitchFromSpinBox, 1, 2);
    connect(m_pitchFromSpinBox, SIGNAL(valueChanged(int)),
	    SLOT(slotPitchFromChanged(int)));

    m_pitchFromChooserButton = new QPushButton(i18n("..."), noteFrame);
    m_pitchFromChooserButton->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
	(QSizePolicy::SizeType)0, 0, 0, m_pitchFromChooserButton->
	sizePolicy().hasHeightForWidth()));
    QToolTip::add(m_pitchFromChooserButton, i18n("choose a pitch using a staff"));  
    noteFrameLayout->addWidget(m_pitchFromChooserButton, 1, 3);
    connect(m_pitchFromChooserButton, SIGNAL(clicked()),
	    SLOT(slotPitchFromChooser()));

    // Pitch To
    m_pitchToSpinBox = new QSpinBox(noteFrame);
    m_pitchToSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_pitchToSpinBox->setValue(cfg->readUnsignedNumEntry("pitchto", 127));
    noteFrameLayout->addWidget(m_pitchToSpinBox, 1, 4);
    connect(m_pitchToSpinBox, SIGNAL(valueChanged(int)),
	    SLOT(slotPitchToChanged(int)));

    m_pitchToChooserButton = new QPushButton(i18n("..."), noteFrame);
    QToolTip::add(m_pitchToChooserButton, i18n("choose a pitch using a staff"));  
    noteFrameLayout->addWidget(m_pitchToChooserButton, 1, 5);
    connect(m_pitchToChooserButton, SIGNAL(clicked()),
	    SLOT(slotPitchToChooser()));

    // Velocity From/To
    m_velocityFromSpinBox = new QSpinBox(noteFrame);
    m_velocityFromSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_velocityFromSpinBox->setValue(cfg->readUnsignedNumEntry("velocityfrom", 0));
    noteFrameLayout->addWidget(m_velocityFromSpinBox, 2, 2);
    connect(m_velocityFromSpinBox, SIGNAL(valueChanged(int)),
	    SLOT(slotVelocityFromChanged(int)));

    m_velocityToSpinBox = new QSpinBox(noteFrame);
    m_velocityToSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_velocityToSpinBox->setValue(cfg->readUnsignedNumEntry("velocityto", 127));
    noteFrameLayout->addWidget( m_velocityToSpinBox, 2, 4 );
    connect(m_velocityToSpinBox, SIGNAL(valueChanged(int)),
	    SLOT(slotVelocityToChanged(int)));

    
    // Duration From/To
    m_noteDurationFromComboBox = new QComboBox(0, noteFrame);
    m_noteDurationFromComboBox->insertItem(i18n("unlimited"));
    noteFrameLayout->addWidget(m_noteDurationFromComboBox, 3, 2);
    connect(m_noteDurationFromComboBox, SIGNAL(activated(int)),
		SLOT(slotDurationFromChanged(int)));

    m_noteDurationToComboBox = new QComboBox(0, noteFrame);
    m_noteDurationToComboBox->insertItem(i18n("unlimited"));
    noteFrameLayout->addWidget(m_noteDurationToComboBox, 3, 4);
    connect(m_noteDurationToComboBox, SIGNAL(activated(int)),
	    	SLOT(slotDurationToChanged(int)));

    populateDurationCombos();


    //---------[ Buttons ]--------------------------------------
    QFrame* privateLayoutWidget = new QFrame(mainWidget);
    QGridLayout* buttonLayout = new QGridLayout(privateLayoutWidget, 1, 1, 20, 6); 

    m_buttonAll = new QPushButton(i18n("Include all"), privateLayoutWidget);
    m_buttonAll->setAutoDefault(true);
    QToolTip::add(m_buttonAll, i18n("Include entire range of values"));  
    buttonLayout->addWidget( m_buttonAll, 0, 0 );
    
    m_buttonNone = new QPushButton(i18n("Exclude all"), privateLayoutWidget);
    m_buttonNone->setAutoDefault(true);
    QToolTip::add(m_buttonNone, i18n("Exclude entire range of values"));  
    buttonLayout->addWidget( m_buttonNone, 0, 1 );
    
    connect(m_buttonAll, SIGNAL(clicked()), this, SLOT(slotToggleAll()));
    connect(m_buttonNone, SIGNAL(clicked()), this, SLOT(slotToggleNone()));

    
}

// Populate the duration combos with a few reasonable values, including
// pixmaps; as per the segment parameters widget in the main window
//
// lifted from segmentparameterbox.cpp on 5/31/03
//
// NOTE: these combos are calculated, rather than fixed, but the code that
// picks a duration by index is dependant upon each index having a previously
// expected value.  This could break easily at some point.
//
// NOTE: per the outstanding bug on this issue, it is impossible to select
// dotted durations with this code.  In order for that to be possible, it will
// probably be necessary to rework this code significantly.  (Post 1.0)
void
EventFilterDialog::populateDurationCombos()
{
    QPixmap noMap = NotePixmapFactory::toQPixmap
	(NotePixmapFactory::makeToolbarPixmap("menu-no-note"));

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i)
    {
        Rosegarden::timeT time = m_standardQuantizations[i];
        Rosegarden::timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::toQPixmap
	    (NotePixmapFactory::makeNoteMenuPixmap(time, error));
	m_noteDurationFromComboBox->insertItem(error ? noMap : pmap, label);
        m_noteDurationToComboBox  ->insertItem(error ? noMap : pmap, label);
    }
    m_noteDurationFromComboBox->insertItem(noMap, i18n("0"));
    m_noteDurationToComboBox->insertItem(noMap, i18n("0"));

    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_noteDurationFromComboBox->setCurrentItem(
	    cfg->readUnsignedNumEntry("durationfrom", 0));
    m_noteDurationToComboBox->setCurrentItem(
	    cfg->readUnsignedNumEntry("durationto", (m_noteDurationToComboBox->count() - 1)));
}

void
EventFilterDialog::slotToggleAll()
{
    RG_DEBUG << "EventFilterDialog::slotToggleAll()" << endl;
    m_pitchFromSpinBox           ->setValue(0);
    m_pitchToSpinBox             ->setValue(127);
    m_velocityFromSpinBox        ->setValue(0);
    m_velocityToSpinBox          ->setValue(127);
    m_noteDurationFromComboBox   ->setCurrentItem(11); // hard coded; should be variable
    m_noteDurationToComboBox     ->setCurrentItem(0);  // 0 = unlimited; 11 = 0
}

void
EventFilterDialog::slotToggleNone()
{
    RG_DEBUG << "EventFilterDialog::slotToggleNone()" << endl;
    m_pitchFromSpinBox           ->setValue(0);
    m_pitchToSpinBox             ->setValue(0);
    m_velocityFromSpinBox        ->setValue(0);
    m_velocityToSpinBox          ->setValue(0);
    m_noteDurationFromComboBox   ->setCurrentItem(11);
    m_noteDurationToComboBox     ->setCurrentItem(11);
}

void
EventFilterDialog::slotOk()
{
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    
    cfg->writeEntry("notecheckbox", m_noteCheckBox->isChecked());
    
    cfg->writeEntry("pitchinclude", m_notePitchIncludeComboBox->currentItem());
    cfg->writeEntry("pitchfrom",    m_pitchFromSpinBox->value());
    cfg->writeEntry("pitchto",      m_pitchToSpinBox->value());
    
    cfg->writeEntry("velocityinclude", m_noteVelocityIncludeComboBox->currentItem());
    cfg->writeEntry("velocityfrom",    m_velocityFromSpinBox->value());
    cfg->writeEntry("velocityto",      m_velocityToSpinBox->value());
    
    cfg->writeEntry("durationinclude", m_noteDurationIncludeComboBox->currentItem());
    cfg->writeEntry("durationfrom",    m_noteDurationFromComboBox->currentItem());
    cfg->writeEntry("durationto",      m_noteDurationToComboBox->currentItem());
    
    accept();
}

void
EventFilterDialog::slotNoteCheckBoxToggle(int)
{
    bool state = m_noteCheckBox->isChecked();
    m_notePitchIncludeComboBox   ->setEnabled(state);
    m_noteVelocityIncludeComboBox->setEnabled(state);
    m_noteDurationIncludeComboBox->setEnabled(state);
    m_pitchToSpinBox             ->setEnabled(state);
    m_pitchFromSpinBox           ->setEnabled(state);
    m_velocityToSpinBox          ->setEnabled(state);
    m_velocityFromSpinBox        ->setEnabled(state);
    m_noteDurationToComboBox     ->setEnabled(state);
    m_noteDurationFromComboBox   ->setEnabled(state);
}


void
EventFilterDialog::slotPitchFromChanged(int pitch)
{
    if (pitch > m_pitchToSpinBox->value())
	m_pitchToSpinBox->setValue(pitch);
}

void
EventFilterDialog::slotPitchToChanged(int pitch)
{
    if (pitch < m_pitchFromSpinBox->value())
	m_pitchFromSpinBox->setValue(pitch);
}

void
EventFilterDialog::slotVelocityFromChanged(int velocity)
{
    if (velocity > m_velocityToSpinBox->value())
	m_velocityToSpinBox->setValue(velocity);
}

void
EventFilterDialog::slotVelocityToChanged(int velocity)
{
    if (velocity < m_velocityFromSpinBox->value())
	m_velocityFromSpinBox->setValue(velocity);
}

void
EventFilterDialog::slotDurationFromChanged(int index)
{
    if (index < m_noteDurationToComboBox->currentItem())
	m_noteDurationToComboBox->setCurrentItem(index);
}

void
EventFilterDialog::slotDurationToChanged(int index)
{
    if (index > m_noteDurationFromComboBox->currentItem())
	m_noteDurationFromComboBox->setCurrentItem(index);
}


void
EventFilterDialog::slotPitchFromChooser()
{
    PitchPickerDialog dialog(this, m_pitchFromSpinBox->value(), true);

    if (dialog.exec() == QDialog::Accepted)
    {
        m_pitchFromSpinBox->setValue(dialog.getPitch());
    }
}

void
EventFilterDialog::slotPitchToChooser()
{
    PitchPickerDialog dialog(this, m_pitchToSpinBox->value(), false);

    if (dialog.exec() == QDialog::Accepted)
    {
        m_pitchToSpinBox->setValue(dialog.getPitch());
    }
}

// Returns a duration based on a combo box index
//
// NOTE: the combos are populated with calculated values, but this code
// assumes that they are always calculated in the same way, and that it can
// expect a fixed relationship between combo index and the duration it's
// supposed to represent.  This could break in the future.
long
EventFilterDialog::getDurationFromIndex(int index)
{
    switch (index)
    {
	// 0
	case 11: return 0;
	// 1/96
	case 10: return long(Note(Note::SixtyFourthNote).getDuration() / 3);
	// 1/64
	case 9 : return long(Note(Note::SixtyFourthNote).getDuration());
	// 1/48
	case 8 : return long(Note(Note::ThirtySecondNote).getDuration() / 3);
	// 1/32
	case 7 : return long(Note(Note::ThirtySecondNote).getDuration());
	// 1/24
	case 6 : return long(Note(Note::SixteenthNote).getDuration() / 3);
	// 1/16
	case 5 : return long(Note(Note::SixteenthNote).getDuration());
	// 1/8
	case 4 : return long(Note(Note::EighthNote).getDuration());
	// 1/4
	case 3 : return long(Note(Note::QuarterNote).getDuration());
	// 1/2
	case 2 : return long(Note(Note::HalfNote).getDuration());
	// 1/1
	case 1 : return long(Note(Note::WholeNote).getDuration());
	// unlimited
	case 0 : return LONG_MAX;	 
    }
    // failsafe
    return LONG_MAX;
}

void
EventFilterDialog::invert(EventFilterDialog::filterRange &foo)
{
    long c = foo.first;
    foo.first = foo.second;
    foo.second = c;
} 

EventFilterDialog::filterRange
EventFilterDialog::getPitch()
{
    EventFilterDialog::filterRange foo;
    foo.first  = m_pitchFromSpinBox->value();
    foo.second = m_pitchToSpinBox  ->value();
    if (!pitchIsInclusive()) invert(foo);
    return foo;
}

EventFilterDialog::filterRange
EventFilterDialog::getVelocity()
{
    EventFilterDialog::filterRange foo;
    foo.first  = m_velocityFromSpinBox->value();
    foo.second = m_velocityToSpinBox  ->value();
    if (!velocityIsInclusive()) invert(foo);
    return foo;
}

EventFilterDialog::filterRange
EventFilterDialog::getDuration()
{
    EventFilterDialog::filterRange foo;
    foo.first  = getDurationFromIndex(m_noteDurationFromComboBox->currentItem());
    foo.second = getDurationFromIndex(m_noteDurationToComboBox  ->currentItem());
    if (!durationIsInclusive()) invert(foo);
    return foo;
}


bool
EventFilterDialog::keepEvent(Rosegarden::Event* const &e)
{
    if ((*e).isa(Rosegarden::Note::EventType))
    {
	long property = 0;

	if ((*e).isa(Rosegarden::Note::EventType) && filterNote())
	{
	    // pitch
	    (*e).get<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH, property);
	    if (!eventInRange(getPitch(), property)) return false;
	    property = 0;
	    
	    // velocity
	    (*e).get<Rosegarden::Int>(Rosegarden::BaseProperties::VELOCITY, property);
	    if (!EventFilterDialog::eventInRange(getVelocity(), property)) return false;
	    property = 0;
		
	    // duration
	    property = (*e).getNotationDuration();
	    if (!EventFilterDialog::eventInRange(getDuration(), property)) return false; 
	    property = 0; 
	} 
	return true;
    }
    return false;
}

// PitchPickerDialog
//
// Constructs a dialog to contain a RosegardenPitchChooser widget to feed back
// to the main event filter dialog and provide a spiffy way for users to
// choose the pitch they want on a staff.
PitchPickerDialog::PitchPickerDialog(QWidget *parent, int initialPitch, bool isFrom) :
    KDialogBase(parent, 0, true, i18n("Pitch Selector"), Ok | Cancel)
{
    QVBox *vBox = makeVBoxMainWidget();

    QFrame *frame = new QFrame(vBox);

    QGridLayout *layout = new QGridLayout(frame, 4, 3, 10, 5);

    QString toFrom = (isFrom ? "Starting" : "Ending");

    m_pitch = new RosegardenPitchChooser(i18n("%1 Pitch").arg(toFrom), frame, initialPitch);
    layout->addMultiCellWidget(m_pitch, 0, 0, 0, 2, Qt::AlignHCenter);
}

PitchPickerDialog::~PitchPickerDialog()
{
    // Nothing here...
}
#include "eventfilter.moc"
