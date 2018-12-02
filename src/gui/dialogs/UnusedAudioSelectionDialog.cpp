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
#include <iostream>


namespace Rosegarden
{


UnusedAudioSelectionDialog::UnusedAudioSelectionDialog(QWidget *parent,
        QString introductoryText,
        std::vector<QString> fileNames
       ) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Select Unused Audio Files"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    layout->addWidget(new QLabel(introductoryText));

    m_listView = new QTableWidget;
    layout->addWidget(m_listView);
    
    m_listView->setColumnCount(3);
    QStringList sl;
    sl << tr("File name") << tr("File size") << tr("Last modified date");
    m_listView->setHorizontalHeaderLabels(sl);
    
    QTableWidgetItem *item = nullptr;
    unsigned int i;
    unsigned int rc;
    for (i = 0; i < fileNames.size(); i++) {
        QString fileName = fileNames[i];
        QFileInfo info(fileName);
        QString fileSize = tr(" (not found) ");
        QString fileDate;
        if (info.exists()) {
            fileSize = QString(" %1 ").arg(info.size());
            fileDate = QString(" %1 ").arg((info.lastModified()).toString(Qt::ISODate));
        }
        rc = m_listView->rowCount();
        m_listView->insertRow(rc);
        
        item = new QTableWidgetItem(fileName);
        m_listView->setItem(rc, 0, item);
        item = new QTableWidgetItem(fileSize);
        m_listView->setItem(rc, 1, item);
        item = new QTableWidgetItem(fileDate);
        m_listView->setItem(rc, 2, item);
        
    }

    m_listView->setSelectionMode(QAbstractItemView::MultiSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
};

std::vector<QString>
UnusedAudioSelectionDialog::getSelectedAudioFileNames() const
{
    std::vector<QString> selectedNames;

    QList<QTableWidgetItem *> sItems = m_listView->selectedItems();
    QTableWidgetItem *item;
    
    for (int i = 0; i < sItems.size(); i++) {
            item = sItems.at(i);
            // Only reutrn items from column 0, which contains the filename
            // we're after:
            if (item->column() == 0) selectedNames.push_back(item->text());
    }

    return selectedNames;
};


}
