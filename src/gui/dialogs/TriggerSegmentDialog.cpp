/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TriggerSegmentDialog.h"
#include <kapplication.h>

#include "base/BaseProperties.h"
#include <klocale.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Composition.h"
#include "base/TriggerSegment.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kdialogbase.h>
#include <qcheckbox.h>
#include <qframe.h>
#include <qlabel.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qlayout.h>


namespace Rosegarden
{

TriggerSegmentDialog::TriggerSegmentDialog(QWidget *parent,
        Composition *composition) :
        KDialogBase(parent, "triggersegmentdialog", true, i18n("Trigger Segment"),
                    Ok | Cancel, Ok),
        m_composition(composition)
{
    QVBox *vbox = makeVBoxMainWidget();

    QFrame *frame = new QFrame(vbox);
    QGridLayout *layout = new QGridLayout(frame, 3, 2, 5, 5);

    QLabel *label = new QLabel(i18n("Trigger segment: "), frame);
    layout->addWidget(label, 0, 0);

    m_segment = new KComboBox(frame);
    layout->addWidget(m_segment, 0, 1);

    int n = 1;
    for (Composition::triggersegmentcontaineriterator i =
                m_composition->getTriggerSegments().begin();
            i != m_composition->getTriggerSegments().end(); ++i) {
        m_segment->insertItem
        (QString("%1. %2").arg(n++).arg(strtoqstr((*i)->getSegment()->getLabel())));
    }

    label = new QLabel(i18n("Perform with timing: "), frame);
    layout->addWidget(label, 1, 0);

    m_adjustTime = new KComboBox(frame);
    layout->addWidget(m_adjustTime, 1, 1);

    m_adjustTime->insertItem(i18n("As stored"));
    m_adjustTime->insertItem(i18n("Truncate if longer than note"));
    m_adjustTime->insertItem(i18n("End at same time as note"));
    m_adjustTime->insertItem(i18n("Stretch or squash segment to note duration"));

    m_retune = new QCheckBox(i18n("Adjust pitch to note"), frame);
    m_retune->setChecked(true);

    layout->addWidget(m_retune, 2, 1);

    setupFromConfig();
}

void
TriggerSegmentDialog::setupFromConfig()
{
    KConfig *config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);

    int seg = config->readNumEntry("triggersegmentlastornament", 0);
    std::string timing = qstrtostr
                         (config->readEntry
                          ("triggersegmenttiming",
                           strtoqstr(BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH)));
    bool retune = config->readBoolEntry("triggersegmentretune", true);

    if (seg >= 0 && seg < m_segment->count())
        m_segment->setCurrentItem(seg);

    if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE) {
        m_adjustTime->setCurrentItem(0);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) {
        m_adjustTime->setCurrentItem(3);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START) {
        m_adjustTime->setCurrentItem(1);
    } else if (timing == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
        m_adjustTime->setCurrentItem(2);
    }

    m_retune->setChecked(retune);
}

TriggerSegmentId
TriggerSegmentDialog::getId() const
{
    int ix = m_segment->currentItem();

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

TriggerSegmentDialog::getTimeAdjust() const
{
    int option = m_adjustTime->currentItem();

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
TriggerSegmentDialog::slotOk()
{
    KConfig *config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);

    config->writeEntry("triggersegmenttiming", strtoqstr(getTimeAdjust()));
    config->writeEntry("triggersegmentretune", m_retune->isChecked());
    config->writeEntry("triggersegmentlastornament", m_segment->currentItem());

    accept();
}

}
#include "TriggerSegmentDialog.moc"
