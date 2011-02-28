/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "UnusedAudioSelectionDialog.h"


#include <QDialog>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QFileInfo>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QDateTime>


namespace Rosegarden
{

UnusedAudioSelectionDialog::UnusedAudioSelectionDialog(QWidget *parent,
        QString introductoryText,
        std::vector<QString> fileNames,
        bool offerCancel 
		) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Select Unused Audio Files"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    new QLabel(introductoryText, vbox);

    m_listView = new QTableWidget( vbox );
    vboxLayout->addWidget(m_listView);
    vbox->setLayout(vboxLayout);
	
	m_listView->setColumnCount( 3 );
	QStringList sl;
	sl << tr("File name") << tr("File size") << tr("Last modified date");
	m_listView->setHorizontalHeaderLabels( sl );
	
//     m_listView->addColumn(tr("File name"));
//     m_listView->addColumn(tr("File size"));
//     m_listView->addColumn(tr("Last modified date"));
	QTableWidgetItem *item = 0;
	unsigned int i;
	unsigned int rc;
	for (i=0; i < fileNames.size(); ++i ) {
        QString fileName = fileNames[i];
        QFileInfo info(fileName);
        QString fileSize = tr(" (not found) ");
        QString fileDate;
        if (info.exists()) {
            fileSize = QString(" %1 ").arg(info.size());
			fileDate = QString(" %1 ").arg( (info.lastModified()).toString(Qt::ISODate) );
        }
		//QTableWidgetItem *item = new QTableWidgetItem(m_listView, fileName, fileSize, fileDate);
		rc = m_listView->rowCount();
		m_listView->insertRow( rc );
		
		item = new QTableWidgetItem(fileName);
		m_listView->setItem( rc, i+0, item );
		item = new QTableWidgetItem(fileSize);
		m_listView->setItem( rc, i+1, item );
		item = new QTableWidgetItem(fileDate);
		m_listView->setItem( rc, i+2, item );
		
	}

	m_listView->setSelectionMode( QAbstractItemView::MultiSelection );
	m_listView->setSelectionBehavior( QAbstractItemView::SelectItems ); // or QAbstractItemView::SelectRows
	
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
};

std::vector<QString>
UnusedAudioSelectionDialog::getSelectedAudioFileNames() const
{
    std::vector<QString> selectedNames;

	QList<QTableWidgetItem *> sItems = m_listView->selectedItems();
	QTableWidgetItem *item;
	
	for( int i=0; i < sItems.size(); i++ ){
        //if (m_listView->isSelected(item)) {
			item = sItems.at(i);
            selectedNames.push_back( item->text() );
        //}

		//item = m_listView->next();
    }

    return selectedNames;
};

}
