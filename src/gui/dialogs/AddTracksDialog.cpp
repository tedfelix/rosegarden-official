/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <kapp.h>
#include <kconfig.h>

#include "document/ConfigGroups.h"


namespace Rosegarden
{

AddTracksDialog::AddTracksDialog(QDialogButtonBox::QWidget *parent, int currentTrack) :
    QDialog(parent),
    m_currentTrack(currentTrack)
{
    setModal(true);
    setWindowTitle(i18n("Add Tracks"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);


    countBoxLayout->setSpacing(4);
    QLabel *child_8 = new QLabel(i18n("How many tracks do you want to add?"), countBox );
    countBoxLayout->addWidget(child_8);
    m_count = new QSpinBox( countBox );
    countBoxLayout->addWidget(m_count);
    countBox->setLayout(countBoxLayout);
    m_count->setMinimum(1);
    m_count->setMaximum(32);
    m_count->setValue(1);

    QWidget *posBox = new QWidget( vBox );
    vBoxLayout->addWidget(posBox);
    vBox->setLayout(vBoxLayout);
    QHBoxLayout *posBoxLayout = new QHBoxLayout;
    posBoxLayout->setSpacing(4);
    QLabel *child_4 = new QLabel(i18n("Add tracks"), posBox );
    posBoxLayout->addWidget(child_4);
    m_position = new QComboBox( posBox );
    posBoxLayout->addWidget(m_position);
    posBox->setLayout(posBoxLayout);
    m_position->addItem(i18n("At the top"));
    m_position->addItem(i18n("Above the current selected track"));
    m_position->addItem(i18n("TicksBelow the current selected track"));
    m_position->addItem(i18n("At the bottom"));

    QSettings config ; // was: confq4
    QSettings config;
    config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
    //  

    m_position->setCurrentIndex( config.value("lastaddtracksposition", 2).toUInt() );
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

int
AddTracksDialog::getTracks()
{
    return m_count->value();
}

int
AddTracksDialog::getInsertPosition()
{
    int opt = m_position->currentIndex();

    QSettings config ; // was: confq4
    QSettings config;
    config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
    //  

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
