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


#include "RescaleDialog.h"

#include <klocale.h>
#include "document/ConfigGroups.h"
#include "base/Composition.h"
#include "gui/widgets/TimeWidget.h"
#include <kconfig.h>
#include <kdialogbase.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>
#include <kapplication.h>


namespace Rosegarden
{

RescaleDialog::RescaleDialog(QWidget *parent,
                             Composition *composition,
                             timeT startTime,
                             timeT originalDuration,
                             bool showCloseGapOption) :
        KDialogBase(parent, 0, true, i18n("Rescale"), User1 | Ok | Cancel)
{
    QVBox *vbox = makeVBoxMainWidget();

    m_newDuration = new TimeWidget
                    (i18n("Duration of selection"), vbox, composition,
                     startTime, originalDuration, true);

    if (showCloseGapOption) {
        QGroupBox *optionBox = new QGroupBox(1, Horizontal, i18n("Options"), vbox);
        m_closeGap = new QCheckBox(i18n("Adjust times of following events accordingly"),
                                   optionBox);
        KConfig *config = kapp->config();
        config->setGroup(GeneralOptionsConfigGroup);
        m_closeGap->setChecked
        (config->readBoolEntry("rescaledialogadjusttimes", true));
    } else {
        m_closeGap = 0;
    }

    setButtonText(User1, i18n("Reset"));
    connect(this, SIGNAL(user1Clicked()),
            m_newDuration, SLOT(slotResetToDefault()));
}

timeT
RescaleDialog::getNewDuration()
{
    return m_newDuration->getTime();
}

bool
RescaleDialog::shouldCloseGap()
{
    if (m_closeGap) {
        KConfig *config = kapp->config();
        config->setGroup(GeneralOptionsConfigGroup);
        config->writeEntry("rescaledialogadjusttimes", m_closeGap->isChecked());
        return m_closeGap->isChecked();
    } else {
        return true;
    }
}

/*
int
RescaleDialog::getMultiplier()
{
    return m_to;
}

int
RescaleDialog::getDivisor()
{
    return m_from;
}

void
RescaleDialog::slotFromChanged(int i)
{
    m_from = i + 1;
    int perTenThou = m_to * 10000 / m_from;
    m_percent->setText(QString("%1.%2%").
                       arg(perTenThou / 100).
                       arg(perTenThou % 100));
}

void
RescaleDialog::slotToChanged(int i)
{
    m_to = i + 1;
    int perTenThou = m_to * 10000 / m_from;
    m_percent->setText(QString("%1.%2%").
                       arg(perTenThou / 100).
                       arg(perTenThou % 100));
}
*/

}
#include "RescaleDialog.moc"
