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


#include "UnusedAudioSelectionDialog.h"

#include <klocale.h>
#include <kdialogbase.h>
#include <klistview.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

UnusedAudioSelectionDialog::UnusedAudioSelectionDialog(QWidget *parent,
        QString introductoryText,
        std::vector<QString> fileNames,
        bool offerCancel) :
        KDialogBase(parent, 0, true, i18n("Select Unused Audio Files"), (offerCancel ? (Ok | Cancel) : Ok))
{
    QVBox *vbox = makeVBoxMainWidget();
    new QLabel(introductoryText, vbox);

    m_listView = new KListView(vbox);

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
        QListViewItem *item = new KListViewItem
                              (m_listView, fileName, fileSize, fileDate);
    }

    m_listView->setSelectionMode(QListView::Multi);
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
