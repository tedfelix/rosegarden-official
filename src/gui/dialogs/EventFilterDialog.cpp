/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

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

#define RG_MODULE_STRING "[EventFilterDialog]"

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
#include "misc/ConfigGroups.h"

#include <QApplication>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
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
    setWindowTitle(tr("Event Filter"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainWidgetLayout = new QVBoxLayout;
    metagrid->addWidget(mainWidget, 0, 0);

    //----------[ Note Filter Widgets ]-------------------------

    // Frame
    QGroupBox* noteFrame = new QGroupBox(tr("Note Events"));
    noteFrame->setContentsMargins(20, 20, 20, 20);
    QGridLayout* noteFrameLayout = new QGridLayout;
    noteFrameLayout->setSpacing(6);
    mainWidgetLayout->addWidget(noteFrame);

    // Labels
    QLabel* pitchFromLabel = new QLabel(tr("lowest:"), noteFrame);
    noteFrameLayout->addWidget(pitchFromLabel, 0, 2);

    QLabel* pitchToLabel = new QLabel(tr("highest:"), noteFrame);
    noteFrameLayout->addWidget(pitchToLabel, 0, 4);

    QLabel* pitchLabel = new QLabel(tr("Pitch:"), noteFrame);
    noteFrameLayout->addWidget(pitchLabel, 1, 1);

    QLabel* velocityLabel = new QLabel(tr("Velocity:"), noteFrame);
    noteFrameLayout->addWidget(velocityLabel, 2, 1);

    QLabel* durationLabel = new QLabel(tr("Duration:"), noteFrame);
    noteFrameLayout->addWidget(durationLabel, 3, 1);

    QSettings settings;
    settings.beginGroup( EventFilterDialogConfigGroup );

    m_useNotationDuration = new QCheckBox(tr("Use notation duration"));
    noteFrameLayout->addWidget(m_useNotationDuration, 4, 2);   // 4, 1
    m_useNotationDuration->setToolTip(tr("Use Notation Editor note durations, "
                                         "not actual"));
    m_useNotationDuration->setChecked(settings.value("usenotationduration",
                                                     "0").toBool());

    m_selectRests = new QCheckBox(tr("Select rests"));
    noteFrameLayout->addWidget(m_selectRests, 4, 4);        // 5, 1
    m_selectRests->setToolTip(tr("Select rests in addition to notes"));
    m_selectRests->setChecked(settings.value("selectrests", "0").toBool());

    // Include Boxes
    m_notePitchIncludeComboBox = new QComboBox(noteFrame);
    m_notePitchIncludeComboBox->setEditable(false);
    m_notePitchIncludeComboBox->addItem(tr("include"));
    m_notePitchIncludeComboBox->addItem(tr("exclude"));

    m_notePitchIncludeComboBox->setCurrentIndex( qStrToBool( settings.value("pitchinclude", "0" ) ) );
    noteFrameLayout->addWidget(m_notePitchIncludeComboBox, 1, 0);

    m_noteVelocityIncludeComboBox = new QComboBox(noteFrame);
    m_noteVelocityIncludeComboBox->setEditable(false);
    m_noteVelocityIncludeComboBox->addItem(tr("include"));
    m_noteVelocityIncludeComboBox->addItem(tr("exclude"));

    m_noteVelocityIncludeComboBox->setCurrentIndex( qStrToBool( settings.value("velocityinclude", "0" ) ) );
    noteFrameLayout->addWidget(m_noteVelocityIncludeComboBox, 2, 0);

    m_noteDurationIncludeComboBox = new QComboBox(noteFrame);
    m_noteDurationIncludeComboBox->setEditable(false);
    m_noteDurationIncludeComboBox->addItem(tr("include"));
    m_noteDurationIncludeComboBox->addItem(tr("exclude"));

    m_noteDurationIncludeComboBox->setCurrentIndex( qStrToBool( settings.value("durationinclude", "0" ) ) );
    noteFrameLayout->addWidget(m_noteDurationIncludeComboBox, 3, 0);

    // Pitch From
    m_pitchFromSpinBox = new QSpinBox(noteFrame);
    m_pitchFromSpinBox->setMaximum(127);

    m_pitchFromSpinBox->setValue( settings.value("pitchfrom", 0).toUInt() );
    noteFrameLayout->addWidget(m_pitchFromSpinBox, 1, 2);
    connect(m_pitchFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPitchFromChanged(int)));

    m_pitchFromChooserButton = new QPushButton(tr("edit"), noteFrame);
    m_pitchFromChooserButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,
                                                        QSizePolicy::Fixed,
                                                        QSizePolicy::PushButton));
    m_pitchFromChooserButton->setToolTip(tr("choose a pitch using a staff"));
    noteFrameLayout->addWidget(m_pitchFromChooserButton, 1, 3);
    connect(m_pitchFromChooserButton, &QAbstractButton::clicked,
            this, &EventFilterDialog::slotPitchFromChooser);

    // Pitch To
    m_pitchToSpinBox = new QSpinBox(noteFrame);
    m_pitchToSpinBox->setMaximum(127);

    m_pitchToSpinBox->setValue( settings.value("pitchto", 127).toUInt() );
    noteFrameLayout->addWidget(m_pitchToSpinBox, 1, 4);
    connect(m_pitchToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPitchToChanged(int)));

    m_pitchToChooserButton = new QPushButton(tr("edit"), noteFrame);
    m_pitchToChooserButton->setToolTip(tr("choose a pitch using a staff"));
    noteFrameLayout->addWidget(m_pitchToChooserButton, 1, 5);
    connect(m_pitchToChooserButton, &QAbstractButton::clicked,
            this, &EventFilterDialog::slotPitchToChooser);

    // Velocity From/To
    m_velocityFromSpinBox = new QSpinBox(noteFrame);
    m_velocityFromSpinBox->setMaximum(127);

    m_velocityFromSpinBox->setValue( settings.value("velocityfrom", 0).toUInt() );
    noteFrameLayout->addWidget(m_velocityFromSpinBox, 2, 2);
    connect(m_velocityFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotVelocityFromChanged(int)));

    m_velocityToSpinBox = new QSpinBox(noteFrame);
    m_velocityToSpinBox->setMaximum(127);

    m_velocityToSpinBox->setValue( settings.value("velocityto", 127).toUInt() );
    noteFrameLayout->addWidget( m_velocityToSpinBox, 2, 4 );
    connect(m_velocityToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotVelocityToChanged(int)));


    // Duration From/To
    m_noteDurationFromComboBox = new QComboBox(noteFrame);
    m_noteDurationFromComboBox->setEditable(false);
    m_noteDurationFromComboBox->addItem(tr("longest"));
    noteFrameLayout->addWidget(m_noteDurationFromComboBox, 3, 2);
    connect(m_noteDurationFromComboBox,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &EventFilterDialog::slotDurationFromChanged);

    m_noteDurationToComboBox = new QComboBox(noteFrame);
    m_noteDurationToComboBox->setEditable(false);
    m_noteDurationToComboBox->addItem(tr("longest"));
    noteFrameLayout->addWidget(m_noteDurationToComboBox, 3, 4);
    connect(m_noteDurationToComboBox,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &EventFilterDialog::slotDurationToChanged);

    populateDurationCombos();


    //---------[ Buttons ]--------------------------------------
    QFrame* privateLayoutWidget = new QFrame(mainWidget);
    privateLayoutWidget->setContentsMargins(20, 20, 20, 20);
    QGridLayout* buttonLayout = new QGridLayout;
    buttonLayout->setSpacing(6);
    mainWidgetLayout->addWidget(privateLayoutWidget);

    m_buttonAll = new QPushButton(tr("Include all"), privateLayoutWidget);
    m_buttonAll->setAutoDefault(true);
    m_buttonAll->setToolTip(tr("Include entire range of values"));
    buttonLayout->addWidget( m_buttonAll, 0, 0 );

    m_buttonNone = new QPushButton(tr("Exclude all"), privateLayoutWidget);
    m_buttonNone->setAutoDefault(true);
    m_buttonNone->setToolTip(tr("Exclude entire range of values"));
    buttonLayout->addWidget( m_buttonNone, 0, 1 );

    connect(m_buttonAll, &QAbstractButton::clicked, this, &EventFilterDialog::slotToggleAll);
    connect(m_buttonNone, &QAbstractButton::clicked, this, &EventFilterDialog::slotToggleNone);

    settings.endGroup();

    privateLayoutWidget->setLayout(buttonLayout);
    noteFrame->setLayout(noteFrameLayout);
    mainWidget->setLayout(mainWidgetLayout);

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void
EventFilterDialog::populateDurationCombos()
{
    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
        timeT time = m_standardQuantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        m_noteDurationFromComboBox->addItem(error ? noMap : pmap, label);
        m_noteDurationToComboBox ->addItem(error ? noMap : pmap, label);
    }
    m_noteDurationFromComboBox->addItem(tr("shortest"));
    m_noteDurationToComboBox->addItem(tr("shortest"));

    m_noteDurationFromComboBox->setMaxVisibleItems(
        m_standardQuantizations.size() + 2);
    m_noteDurationToComboBox->setMaxVisibleItems(
        m_standardQuantizations.size() + 2);

    QSettings settings;
    settings.beginGroup( EventFilterDialogConfigGroup );

    m_noteDurationFromComboBox->setCurrentIndex( settings.value("durationfrom",
        (m_noteDurationToComboBox->count() - 1)).toUInt() );
    m_noteDurationToComboBox->setCurrentIndex( settings.value("durationto",
        0).toUInt());

    settings.endGroup();
}

void
EventFilterDialog::slotToggleAll()
{
    RG_DEBUG << "EventFilterDialog::slotToggleAll()";
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
    RG_DEBUG << "EventFilterDialog::slotToggleNone()";
    m_pitchFromSpinBox ->setValue(0);
    m_pitchToSpinBox ->setValue(0);
    m_velocityFromSpinBox ->setValue(0);
    m_velocityToSpinBox ->setValue(0);
    m_noteDurationFromComboBox ->setCurrentIndex(11);
    m_noteDurationToComboBox ->setCurrentIndex(11);
}

void
EventFilterDialog::accept()
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

    settings.setValue("usenotationduration", m_useNotationDuration->isChecked());
    settings.setValue("selectrests", m_selectRests->isChecked());

    settings.endGroup();

    QDialog::accept();
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
    PitchPickerDialog dialog(this, m_pitchFromSpinBox->value(), tr("Lowest pitch"));

    if (dialog.exec() == QDialog::Accepted) {
        m_pitchFromSpinBox->setValue(dialog.getPitch());
    }
}

void
EventFilterDialog::slotPitchToChooser()
{
    PitchPickerDialog dialog(this, m_pitchToSpinBox->value(), tr("Highest pitch"));

    if (dialog.exec() == QDialog::Accepted) {
        m_pitchToSpinBox->setValue(dialog.getPitch());
    }
}

long
EventFilterDialog::getDurationFromIndex(unsigned index)
{
    if (index == 0) {
        // both combo boxes had addItem("longest") at instantiaion,
        // prior to populateCombos()
        return LONG_MAX;
    }
    else if (index > m_standardQuantizations.size() + 1) {
        // number of combo box items is m_standardQuantizations.size() + 2
        // ("longest" added before, "shortest" after) so this one
        // is "shortest"
        return m_standardQuantizations[m_standardQuantizations.size() - 1];
    }
    else {
        // because index == 0 is "longest", as per above
        return m_standardQuantizations[index - 1];
    }
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
                     << " out of range.";
            return false;
        }
        property = 0;

        // velocity
        (*e).get<Int>(BaseProperties::VELOCITY, property);
        if (!EventFilterDialog::eventInRange(getVelocity(), property)) {
            RG_DEBUG << "EventFilterDialog::keepEvent(): rejecting event; velocity " << property
                     << " out of range.";
            return false;
        }

        // duration
        property = m_useNotationDuration->isChecked() ? (*e).getNotationDuration() : (*e).getDuration();

        if (!EventFilterDialog::eventInRange(getDuration(), property)) {
            RG_DEBUG << "EventFilterDialog::keepEvent(): rejecting event; duration " << property
                     << " out of range.";
            return false;
        }
        property = 0;

        return true;
    } else if ((*e).isa(Note::EventRestType)) {
        if (m_selectRests->isChecked()) {
            long property = m_useNotationDuration->isChecked() ? (*e).getNotationDuration() : (*e).getDuration();

            if (!EventFilterDialog::eventInRange(getDuration(), property)) {
                RG_DEBUG << "EventFilterDialog::keepEvent(): rejecting rest; duration " << property
                         << " out of range.";
                return false;
            }
            return true;
        }
    }
    return false;
}

}
