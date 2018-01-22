/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PlayListDialog.h"

#include "misc/ConfigGroups.h"
#include "PlayList.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QLayout>
#include <QSettings>

namespace Rosegarden
{

PlayListDialog::PlayListDialog( QString caption,
                               QWidget* parent, const char* name)
            : QDialog(parent),
   m_playList(new PlayList(this))
{
    setObjectName( name );
    setModal(false);
    setWindowTitle( caption );

    QGridLayout *metagrid = new QGridLayout;
    metagrid->addWidget(m_playList, 0, 0);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);

    setLayout(metagrid);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()),
            this, SLOT(slotClose()));

    restore();
}

void PlayListDialog::save()	//@@@ new implementation:
{
//     saveDialogSize(PlayListConfigGroup);
	
	QSettings settings;
	settings.beginGroup(PlayListConfigGroup);
	
	settings.setValue( "geometry", this->saveGeometry() );
	settings.endGroup();
}


void PlayListDialog::restore()	//@@@ new implementation:
{
//     setInitialSize(configDialogSize(PlayListConfigGroup));
	
	QSettings settings;
	settings.beginGroup(PlayListConfigGroup);
	this->restoreGeometry( settings.value("geometry").toByteArray() );
	settings.endGroup();
}


void PlayListDialog::closeEvent(QCloseEvent *e)
{
    save();
    emit closing();
    QDialog::closeEvent(e);
}

void PlayListDialog::slotClose()
{
    save();
    emit closing();
    QDialog::close();
}

}
