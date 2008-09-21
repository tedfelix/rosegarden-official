/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    This file is Copyright 2003-2006
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "EventFilterDialog.h"

#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/BasicQuantizer.h"
#include "gui/dialogs/PitchPickerDialog.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "document/ConfigGroups.h"
#include <QApplication>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <klocale.h>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>

#include <limits.h>


namespace Rosegarden
{

EventFilterDialog::EventFilterDialog(QWidget* parent)
        : QDialog(parent),
        m_standardQuantizations(BasicQuantizer::getStandardQuantizations())
{
    //###JAS next line not needed.  Commented out.
    //###settings = confq4;
    initDialog();
}

EventFilterDialog::~EventFilterDialog()
{
    // nothing here
}

void
EventFilterDialog::initDialog()
{
    setModal(true);
    setWindowTitle(i18n("Event Filter"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainWidgetLayout = new QVBoxLayout;
    metagrid->addWidget(mainWidget, 0, 0);

    //----------[ Note Filter Widgets ]-------------------------

    // Frame
    QGroupBox* noteFrame = new QGroupBox(i18n("Note Events"));
    noteFrame->setContentsMargins(20, 20, 20, 20);
    QGridLayout* noteFrameLayout = new QGridLayout;
    noteFrameLayout->setSpacing(6);
    mainWidgetLayout->addWidget(noteFrame);

    // Labels
    QLabel* pitchFromLabel = new QLabel(i18n("lowest:"), noteFrame);
    noteFrameLayout->addWidget(pitchFromLabel, 0, 2);

    QLabel* pitchToLabel = new QLabel(i18n("highest:"), noteFrame);
    noteFrameLayout->addWidget(pitchToLabel, 0, 4);

    QLabel* pitchLabel = new QLabel(i18n("Pitch:"), noteFrame);
    noteFrameLayout->addWidget(pitchLabel, 1, 1);

    QLabel* velocityLabel = new QLabel(i18n("Velocity:"), noteFrame);
    noteFrameLayout->addWidget(velocityLabel, 2, 1);

    QLabel* durationLabel = new QLabel(i18n("Duration:"), noteFrame);
    noteFrameLayout->addWidget(durationLabel, 3, 1);

    // Include Boxes
    m_notePitchIncludeComboBox = new QComboBox(noteFrame);
    m_notePitchIncludeComboBox->setEditable(false);
    m_notePitchIncludeComboBox->addItem(i18n("include"));
    m_notePitchIncludeComboBox->addItem(i18n("exclude"));
    QSettings settings;
    settings.beginGroup( EventFilterDialogConfigGroup );

    m_notePitchIncludeComboBox->setCurrentIndex( qStrToBool( settings.value("pitchinclude", "0" ) ) );
    noteFrameLayout->addWidget(m_notePitchIncludeComboBox, 1, 0);

    m_noteVelocityIncludeComboBox = new QComboBox(noteFrame);
    m_noteVelocityIncludeComboBox->setEditable(false);
    m_noteVelocityIncludeComboBox->addItem(i18n("include"));
    m_noteVelocityIncludeComboBox->addItem(i18n("exclude"));

    //### settings.beginGroup( EventFilterDialogConfigGroup );
    m_noteVelocityIncludeComboBox->setCurrentIndex( qStrToBool( settings.value("velocityinclude", "0" ) ) );
    noteFrameLayout->addWidget(m_noteVelocityIncludeComboBox, 2, 0);

    m_noteDurationIncludeComboBox = new QComboBox(noteFrame);
    m_noteDurationIncludeComboBox->setEditable(false);
    m_noteDurationIncludeComboBox->addItem(i18n("include"));
    m_noteDurationIncludeComboBox->addItem(i18n("exclude"));

    //### settings.beginGroup( EventFilterDialogConfigGroup );
    m_noteDurationIncludeComboBox->setCurrentIndex( qStrToBool( settings.value("durationinclude", "0" ) ) );
    noteFrameLayout->addWidget(m_noteDurationIncludeComboBox, 3, 0);

    // Pitch From
    m_pitchFromSpinBox = new QSpinBox(noteFrame);
    m_pitchFromSpinBox->setMaximum(127);

    //### settings.beginGroup( EventFilterDialogConfigGroup );
    m_pitchFromSpinBox->setValue( settings.value("pitchfrom", 0).toUInt() );
    noteFrameLayout->addWidget(m_pitchFromSpinBox, 1, 2);
    connect(m_pitchFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPitchFromChanged(int)));

    m_pitchFromChooserButton = new QPushButton(i18n("edit"), noteFrame);
    m_pitchFromChooserButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
                                                        QSizePolicy::Fixed,
                                                        QSizePolicy::PushButton));
    m_pitchFromChooserButton->setToolTip(i18n("choose a pitch using a staff"));
    noteFrameLayout->addWidget(m_pitchFromChooserButton, 1, 3);
    connect(m_pitchFromChooserButton, SIGNAL(clicked()),
            SLOT(slotPitchFromChooser()));

    // Pitch To
    m_pitchToSpinBox = new QSpinBox(noteFrame);
    m_pitchToSpinBox->setMaximum(127);

    //### settings.beginGroup( EventFilterDialogConfigGroup );

    m_pitchToSpinBox->setValue( settings.value("pitchto", 127).toUInt() );
    noteFrameLayout->addWidget(m_pitchToSpinBox, 1, 4);
    connect(m_pitchToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPitchToChanged(int)));

    m_pitchToChooserButton = new QPushButton(i18n("edit"), noteFrame);
    m_pitchToChooserButton->setToolTip(i18n("choose a pitch using a staff"));
    noteFrameLayout->addWidget(m_pitchToChooserButton, 1, 5);
    connect(m_pitchToChooserButton, SIGNAL(clicked()),
            SLOT(slotPitchToChooser()));

    // Velocity From/To
    m_velocityFromSpinBox = new QSpinBox(noteFrame);
    m_velocityFromSpinBox->setMaximum(127);

    //### settings.beginGroup( EventFilterDialogConfigGroup );
    m_velocityFromSpinBox->setValue( settings.value("velocityfrom", 0).toUInt() );
    noteFrameLayout->addWidget(m_velocityFromSpinBox, 2, 2);
    connect(m_velocityFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotVelocityFromChanged(int)));

    m_velocityToSpinBox = new QSpinBox(noteFrame);
    m_velocityToSpinBox->setMaximum(127);

    //###settings.beginGroup( EventFilterDialogConfigGroup );
    m_velocityToSpinBox->setValue( settings.value("velocityto", 127).toUInt() );
    noteFrameLayout->addWidget( m_velocityToSpinBox, 2, 4 );
    connect(m_velocityToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotVelocityToChanged(int)));


    // Duration From/To
    m_noteDurationFromComboBox = new QComboBox(noteFrame);
    m_noteDurationFromComboBox->setEditable(false);
    m_noteDurationFromComboBox->addItem(i18n("longest"));
    noteFrameLayout->addWidget(m_noteDurationFromComboBox, 3, 2);
    connect(m_noteDurationFromComboBox, SIGNAL(activated(int)),
            SLOT(slotDurationFromChanged(int)));

    m_noteDurationToComboBox = new QComboBox(noteFrame);
    m_noteDurationToComboBox->setEditable(false);
    m_noteDurationToComboBox->addItem(i18n("longest"));
    noteFrameLayout->addWidget(m_noteDurationToComboBox, 3, 4);
    connect(m_noteDurationToComboBox, SIGNAL(activated(int)),
            SLOT(slotDurationToChanged(int)));

    populateDurationCombos();


    //---------[ Buttons ]--------------------------------------
    QFrame* privateLayoutWidget = new QFrame(mainWidget);
    privateLayoutWidget->setContentsMargins(20, 20, 20, 20);
    QGridLayout* buttonLayout = new QGridLayout;
    buttonLayout->setSpacing(6);
    mainWidgetLayout->addWidget(privateLayoutWidget);

    m_buttonAll = new QPushButton(i18n("Include all"), privateLayoutWidget);
    m_buttonAll->setAutoDefault(true);
    m_buttonAll->setToolTip(i18n("Include entire range of values"));
    buttonLayout->addWidget( m_buttonAll, 0, 0 );

    m_buttonNone = new QPushButton(i18n("Exclude all"), privateLayoutWidget);
    m_buttonNone->setAutoDefault(true);
    m_buttonNone->setToolTip(i18n("Exclude entire range of values"));
    buttonLayout->addWidget( m_buttonNone, 0, 1 );

    connect(m_buttonAll, SIGNAL(clicked()), this, SLOT(slotToggleAll()));
    connect(m_buttonNone, SIGNAL(clicked()), this, SLOT(slotToggleNone()));

    settings.endGroup();

    privateLayoutWidget->setLayout(buttonLayout);
    noteFrame->setLayout(noteFrameLayout);
    mainWidget->setLayout(mainWidgetLayout);

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
EventFilterDialog::populateDurationCombos()
{
    QPixmap noMap = NotePixmapFactory::toQPixmap
                    (NotePixmapFactory::makeToolbarPixmap("menu-no-note"));

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
        timeT time = m_standardQuantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::toQPixmap
                       (NotePixmapFactory::makeNoteMenuPixmap(time, error));
        m_noteDurationFromComboBox->addItem(error ? noMap : pmap, label);
        m_noteDurationToComboBox ->addItem(error ? noMap : pmap, label);
    }
    m_noteDurationFromComboBox->addItem(noMap, i18n("shortest"));
    m_noteDurationToComboBox->addItem(noMap, i18n("shortest"));

    QSettings settings;
    settings.beginGroup( EventFilterDialogConfigGroup );

    m_noteDurationFromComboBox->setCurrentIndex( settings.value("durationfrom", 0).toUInt() );
    m_noteDurationToComboBox->setCurrentIndex( settings.value("durationto", (m_noteDurationToComboBox->count() - 1)).toUInt());

    settings.endGroup();
}

void
EventFilterDialog::slotToggleAll()
{
    RG_DEBUG << "EventFilterDialog::slotToggleAll()" << endl;
    m_pitchFromSpinBox ->setValue(0);
    m_pitchToSpinBox ->setValue(127);
    m_velocityFromSpinBox ->setValue(0);
    m_velocityToSpinBox ->setValue(127);
    m_noteDurationFromComboBox ->setCurrentIndex(11); // hard coded; should be variable
    m_noteDurationToComboBox ->setCurrentIndex(0);  // 0 = unlimited; 11 = 0
}

void
EventFilterDialog::slotToggleNone()
{
    RG_DEBUG << "EventFilterDialog::slotToggleNone()" << endl;
    m_pitchFromSpinBox ->setValue(0);
    m_pitchToSpinBox ->setValue(0);
    m_velocityFromSpinBox ->setValue(0);
    m_velocityToSpinBox ->setValue(0);
    m_noteDurationFromComboBox ->setCurrentIndex(11);
    m_noteDurationToComboBox ->setCurrentIndex(11);
}

void
EventFilterDialog::slotOk()
{
    QSettings settings;
    settings.beginGroup( EventFilterDialogConfigGroup );

    settings.setValue("pitchinclude", m_notePitchIncludeComboBox->currentIndex());
    settings.setValue("pitchfrom", m_pitchFromSpinBox->value());
    settings.setValue("pitchto", m_pitchToSpinBox->value());

    settings.setValue("velocityinclude", m_noteVelocityIncludeComboBox->currentIndex());
    settings.setValue("velocityfrom", m_velocityFromSpinBox->value());
    settings.setValue("velocityto", m_velocityToSpinBox->value());

    settings.setValue("durationinclude", m_noteDurationIncludeComboBox->currentIndex());
    settings.setValue("durationfrom", m_noteDurationFromComboBox->currentIndex());
    settings.setValue("durationto", m_noteDurationToComboBox->currentIndex());

    accept();

    settings.endGroup();
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
    if (index < m_noteDurationToComboBox->currentIndex())
        m_noteDurationToComboBox->setCurrentIndex(index);
}

void
EventFilterDialog::slotDurationToChanged(int index)
{
    if (index > m_noteDurationFromComboBox->currentIndex())
        m_noteDurationFromComboBox->setCurrentIndex(index);
}


void
EventFilterDialog::slotPitchFromChooser()
{
    PitchPickerDialog dialog(this, m_pitchFromSpinBox->value(), i18n("Lowest pitch"));

    if (dialog.exec() == QDialog::Accepted) {
        m_pitchFromSpinBox->setValue(dialog.getPitch());
    }
}

void
EventFilterDialog::slotPitchToChooser()
{
    PitchPickerDialog dialog(this, m_pitchToSpinBox->value(), i18n("Highest pitch"));

    if (dialog.exec() == QDialog::Accepted) {
        m_pitchToSpinBox->setValue(dialog.getPitch());
    }
}

long
EventFilterDialog::getDurationFromIndex(int index)
{
    switch (index) {
        // 0
    case 11:
        return 0;
        // 1/96
    case 10:
        return long(Note(Note::SixtyFourthNote).getDuration() / 3);
        // 1/64
    case 9 :
        return long(Note(Note::SixtyFourthNote).getDuration());
        // 1/48
    case 8 :
        return long(Note(Note::ThirtySecondNote).getDuration() / 3);
        // 1/32
    case 7 :
        return long(Note(Note::ThirtySecondNote).getDuration());
        // 1/24
    case 6 :
        return long(Note(Note::SixteenthNote).getDuration() / 3);
        // 1/16
    case 5 :
        return long(Note(Note::SixteenthNote).getDuration());
        // 1/8
    case 4 :
        return long(Note(Note::EighthNote).getDuration());
        // 1/4
    case 3 :
        return long(Note(Note::QuarterNote).getDuration());
        // 1/2
    case 2 :
        return long(Note(Note::HalfNote).getDuration());
        // 1/1
    case 1 :
        return long(Note(Note::WholeNote).getDuration());
        // unlimited
    case 0 :
        return LONG_MAX;
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
    foo.first = m_pitchFromSpinBox->value();
    foo.second = m_pitchToSpinBox ->value();
    if (!pitchIsInclusive())
        invert(foo);
    return foo;
}

EventFilterDialog::filterRange
EventFilterDialog::getVelocity()
{
    EventFilterDialog::filterRange foo;
    foo.first = m_velocityFromSpinBox->value();
    foo.second = m_velocityToSpinBox ->value();
    if (!velocityIsInclusive())
        invert(foo);
    return foo;
}

EventFilterDialog::filterRange
EventFilterDialog::getDuration()
{
    EventFilterDialog::filterRange foo;
    foo.first = getDurationFromIndex(m_noteDurationFromComboBox->currentIndex());
    foo.second = getDurationFromIndex(m_noteDurationToComboBox ->currentIndex());
    if (!durationIsInclusive())
        invert(foo);
    return foo;
}

bool
EventFilterDialog::keepEvent(Event* const &e)
{
    if ((*e).isa(Note::EventType)) {
        long property = 0;

	// pitch
	(*e).get<Int>(BaseProperties::PITCH, property);
	if (!eventInRange(getPitch(), property)) {
	    RG_DEBUG << "EventFilterDialog::keepEvent(): rejecting event; pitch " << property
		     << " out of range." << endl;
	    return false;
	}
	property = 0;

	// velocity
	(*e).get<Int>(BaseProperties::VELOCITY, property);
	if (!EventFilterDialog::eventInRange(getVelocity(), property)) {
	    RG_DEBUG << "EventFilterDialog::keepEvent(): rejecting event; velocity " << property
		     << " out of range." << endl;
	    return false;
	}
	property = 0;

	// duration
	property = (*e).getNotationDuration();
	if (!EventFilterDialog::eventInRange(getDuration(), property)) {
	    RG_DEBUG << "EventFilterDialog::keepEvent(): rejecting event; duration " << property
		     << " out of range." << endl;
	    return false;
	}
	property = 0;
	
	return true;
    }
    return false;
}

}

#include "EventFilterDialog.moc"
