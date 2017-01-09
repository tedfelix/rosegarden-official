/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TriggerSegmentDialog.h"
#include <QApplication>

#include "base/BaseProperties.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/Composition.h"
#include "base/TriggerSegment.h"
#include <QComboBox>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QLayout>


namespace Rosegarden
{

TriggerSegmentDialog::TriggerSegmentDialog(QWidget *parent,
        Composition *composition) :
        QDialog(parent),
        m_composition(composition)
{
    setModal(true);
    setWindowTitle(tr("Trigger Segment"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);

    QFrame *frame = new QFrame(this);
    metagrid->addWidget(frame, 0, 0);

    frame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(5);


    QLabel *label = new QLabel(tr("Trigger segment: "));
    layout->addWidget(label, 0, 0);

    m_segment = new QComboBox(frame);
    layout->addWidget(m_segment, 0, 1);

    int n = 1;
    for (Composition::triggersegmentcontaineriterator i =
                m_composition->getTriggerSegments().begin();
            i != m_composition->getTriggerSegments().end(); ++i) {
        m_segment->addItem
        (QString("%1. %2").arg(n++).arg(strtoqstr((*i)->getSegment()->getLabel())));
    }

    label = new QLabel(tr("Perform with timing: "));
    layout->addWidget(label, 1, 0);

    m_adjustTime = new QComboBox;
    layout->addWidget(m_adjustTime, 1, 1);

    m_adjustTime->addItem(tr("As stored"));
    m_adjustTime->addItem(tr("Truncate if longer than note"));
    m_adjustTime->addItem(tr("End at same time as note"));
    m_adjustTime->addItem(tr("Stretch or squash segment to note duration"));

    m_retune = new QCheckBox(tr("Adjust pitch to note"));
    m_retune->setChecked(true);

    layout->addWidget(m_retune, 2, 1);
    frame->setLayout(layout);

    setupFromConfig();

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
TriggerSegmentDialog::setupFromConfig()
{
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    int seg = settings.value("triggersegmentlastornament", 0).toInt() ;
    std::string timing = qstrtostr(settings.value("triggersegmenttiming",
            strtoqstr(BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH)).toString());
    bool retune = qStrToBool( settings.value("triggersegmentretune", "true" ) ) ;

    if (seg >= 0 && seg < m_segment->count())
        m_segment->setCurrentIndex(seg);

    if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE) {
        m_adjustTime->setCurrentIndex(0);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) {
        m_adjustTime->setCurrentIndex(3);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START) {
        m_adjustTime->setCurrentIndex(1);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
        m_adjustTime->setCurrentIndex(2);
    }

    m_retune->setChecked(retune);

    settings.endGroup();
}

TriggerSegmentId
TriggerSegmentDialog::getId() const
{
    int ix = m_segment->currentIndex();

    for (Composition::triggersegmentcontaineriterator i =
                m_composition->getTriggerSegments().begin();
            i != m_composition->getTriggerSegments().end(); ++i) {

        if (ix == 0)
            return (*i)->getId();
        --ix;
    }

    return 0;
}

bool
TriggerSegmentDialog::getRetune() const
{
    return m_retune->isChecked();
}

std::string
TriggerSegmentDialog::getTimeAdjust() const
{
    int option = m_adjustTime->currentIndex();

    switch (option) {

    case 0:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;
    case 1:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START;
    case 2:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END;
    case 3:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH;

    default:
        return BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;
    }
}

void
TriggerSegmentDialog::accept()
{
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("triggersegmenttiming", strtoqstr(getTimeAdjust()));
    settings.setValue("triggersegmentretune", m_retune->isChecked());
    settings.setValue("triggersegmentlastornament", m_segment->currentIndex());

    settings.endGroup();

    QDialog::accept();
}

}
