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


#include "UnusedAudioSelectionDialog.h"

#include <klocale.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListView>
#include <QFileInfo>
#include <QLabel>
#include <QListView>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

UnusedAudioSelectionDialog::UnusedAudioSelectionDialog(QDialogButtonBox::QWidget *parent,
        QString introductoryText,
        std::vector<QString> fileNames,
        bool offerCancel) :
        QDialog(parent) : Ok))
{
    setModal(true);
    setWindowTitle(i18n("Select Unused Audio Files"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    new QLabel(introductoryText, vbox);

    m_listView = new QListView( vbox );
    vboxLayout->addWidget(m_listView);
    vbox->setLayout(vboxLayout);

    m_listView->addColumn(i18n("File name"));
    m_listView->addColumn(i18n("File size"));
    m_listView->addColumn(i18n("Last modified date"));

    for (unsigned int i = 0; i < fileNames.size(); ++i) {
        QString fileName = fileNames[i];
        QFileInfo info(fileName);
        QString fileSize = i18n(" (not found) ");
        QString fileDate;
        if (info.exists()) {
            fileSize = QString(" %1 ").arg(info.size());
            fileDate = QString(" %1 ").arg(info.lastModified().toString());
        }
        QListViewItem *item = new QListViewItem
                              (m_listView, fileName, fileSize, fileDate);
    }

    m_listView->setSelectionMode(QListView::Multi);
    QDialogButtonBox *buttonBox = new QDialogButtonBox((QDialogButtonBox::offerCancel ? (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

std::vector<QString>
UnusedAudioSelectionDialog::getSelectedAudioFileNames() const
{
    std::vector<QString> selectedNames;

    QListViewItem *item = m_listView->firstChild();

    while (item) {

        if (m_listView->isSelected(item)) {
            selectedNames.push_back(item->text(0));
        }

        item = item->nextSibling();
    }

    return selectedNames;
}

}
