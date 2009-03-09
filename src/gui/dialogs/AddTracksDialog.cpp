/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddTracksDialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>

#include "document/ConfigGroups.h"


namespace Rosegarden
{

AddTracksDialog::AddTracksDialog(QWidget *parent, int currentTrack) :
    QDialog(parent),
    m_currentTrack(currentTrack)
{
    setModal(true);
    setWindowTitle(tr("Add Tracks"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);

	QVBoxLayout* countBoxLayout;
	countBoxLayout = vBoxLayout;		// assigned alias
//    countBoxLayout->setSpacing(4);
	vBox->setLayout( countBoxLayout );
	
    QLabel *child_8 = new QLabel(tr("How many tracks do you want to add?") );
    countBoxLayout->addWidget(child_8);
	
    m_count = new QSpinBox();
    countBoxLayout->addWidget(m_count);
    
	m_count->setMinimum(1);
    m_count->setMaximum(32);
    m_count->setValue(1);

    QWidget *posBox = new QWidget( vBox );
    vBoxLayout->addWidget(posBox);
    vBox->setLayout(vBoxLayout);
    QHBoxLayout *posBoxLayout = new QHBoxLayout;
    posBoxLayout->setSpacing(4);
    QLabel *child_4 = new QLabel(tr("Add tracks"), posBox );
    posBoxLayout->addWidget(child_4);
    m_position = new QComboBox( posBox );
    posBoxLayout->addWidget(m_position);
    posBox->setLayout(posBoxLayout);
    m_position->addItem(tr("At the top"));
    m_position->addItem(tr("Above the current selected track"));
    m_position->addItem(tr("Below the current selected track"));
    m_position->addItem(tr("At the bottom"));

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    m_position->setCurrentIndex( settings.value("lastaddtracksposition", 2).toUInt() );
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    settings.endGroup();
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

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("lastaddtracksposition", opt);

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

    settings.endGroup();

    return pos;
}

}
#include "AddTracksDialog.moc"
