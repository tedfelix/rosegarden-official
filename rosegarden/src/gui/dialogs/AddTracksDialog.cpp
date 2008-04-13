/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "AddTracksDialog.h"

#include <klocale.h>
#include <kdialogbase.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <kcombobox.h>
#include <qvbox.h>
#include <qwidget.h>
#include <kapp.h>
#include <kconfig.h>

#include "document/ConfigGroups.h"


namespace Rosegarden
{

AddTracksDialog::AddTracksDialog(QWidget *parent, int currentTrack) :
    KDialogBase(parent, 0, true, i18n("Add Tracks"),
                Ok | Cancel),
    m_currentTrack(currentTrack)
{
    QVBox *vBox = makeVBoxMainWidget();

    QHBox *countBox = new QHBox(vBox);
    countBox->setSpacing(4);
    new QLabel(i18n("How many tracks do you want to add?"), countBox);
    m_count = new QSpinBox(countBox);
    m_count->setMinValue(1);
    m_count->setMaxValue(32);
    m_count->setValue(1);

    QHBox *posBox = new QHBox(vBox);
    posBox->setSpacing(4);
    new QLabel(i18n("Add tracks"), posBox);
    m_position = new KComboBox(posBox);
    m_position->insertItem(i18n("At the top"));
    m_position->insertItem(i18n("Above the current selected track"));
    m_position->insertItem(i18n("Below the current selected track"));
    m_position->insertItem(i18n("At the bottom"));

    KConfig *config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);
    m_position->setCurrentItem(config->readUnsignedNumEntry("lastaddtracksposition", 2));
}

int
AddTracksDialog::getTracks()
{
    return m_count->value();
}

int
AddTracksDialog::getInsertPosition()
{
    int opt = m_position->currentItem();

    KConfig *config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);
    config->writeEntry("lastaddtracksposition", opt);

    int pos = 0;

    switch (opt) {
    case 0: // at top
        pos = 0;
        break;
    case 1: // above current track
        pos = m_currentTrack;
        break;
    case 2: // below current track
        pos = m_currentTrack + 1;
        break;
    case 3: // at bottom
        pos = -1;
        break;
    }

    return pos;
}

}
#include "AddTracksDialog.moc"
