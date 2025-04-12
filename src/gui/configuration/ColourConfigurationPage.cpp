/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ColourConfigurationPage]"

#include "ColourConfigurationPage.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/ColourMap.h"
#include "commands/segment/SegmentColourMapCommand.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/general/GUIPalette.h"
#include "gui/widgets/ColourTable.h"
#include "TabbedConfigurationPage.h"
#include "gui/widgets/LineEdit.h"
#include "gui/widgets/InputDialog.h"

#include <QColorDialog>
#include <QSettings>
#include <QColor>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QTabBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>
#include <QLayout>
#include <QMessageBox>


namespace Rosegarden
{

ColourConfigurationPage::ColourConfigurationPage(QWidget *parent) :
    TabbedConfigurationPage(parent)
{
    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    m_map = m_doc->getComposition().getSegmentColourMap();

    m_colourtable = new ColourTable(frame, m_map, m_listmap);
    m_colourtable->setFixedHeight(280);

    layout->addWidget(m_colourtable, 0, 0, 0- 0+1, 1- 0+1);

    QPushButton* addColourButton = new QPushButton(tr("Add New Color"),
                                   frame);
    layout->addWidget(addColourButton, 1, 0, Qt::AlignHCenter);

    // disable until we can remove it after release
    addColourButton->setEnabled(false);

    QPushButton* deleteColourButton = new QPushButton(tr("Delete Color"),
                                      frame);
    layout->addWidget(deleteColourButton, 1, 1, Qt::AlignHCenter);

    // disable until we can remove it after release
    deleteColourButton->setEnabled(false);

    connect(addColourButton, &QAbstractButton::clicked,
            this, &ColourConfigurationPage::slotAddNew);

    connect(deleteColourButton, &QAbstractButton::clicked,
            this, &ColourConfigurationPage::slotDelete);

    connect(this, &ColourConfigurationPage::docColoursChanged,
            m_doc, &RosegardenDocument::slotDocColoursChanged);

    connect(m_colourtable, &ColourTable::entryTextChanged,
            this, &ColourConfigurationPage::slotTextChanged);

    connect(m_colourtable, &ColourTable::entryColourChanged,
            this, &ColourConfigurationPage::slotColourChanged);

    addTab(frame, tr("Color Map"));

}

void
ColourConfigurationPage::slotTextChanged(unsigned int index, QString string)
{
    m_map.modifyName(m_listmap[index], std::string(string.toLatin1()));
    m_colourtable->populate_table(m_map, m_listmap);
}

void
ColourConfigurationPage::slotColourChanged(unsigned int index, QColor color)
{
    m_map.modifyColour(m_listmap[index], color);
    m_colourtable->populate_table(m_map, m_listmap);
}

void
ColourConfigurationPage::apply()
{
    SegmentColourMapCommand *command = new SegmentColourMapCommand(m_doc, m_map);
    CommandHistory::getInstance()->addCommand(command);

    RG_DEBUG << "ColourConfigurationPage::apply() emitting docColoursChanged()";
    emit docColoursChanged();
}

void
ColourConfigurationPage::slotAddNew()
{
    QColor temp;

    bool ok = false;

    QString newName = InputDialog::getText(this, tr("New Color Name"),
                                           tr("Enter new name"), LineEdit::Normal,
                                           tr("New"),
                                           &ok);

    if ((ok == true) && (!newName.isEmpty())) {
        //QColorDialog box(this, "", true);

        //int result = box.getColor( temp );
        //QColor col = QColorDialog::getColor();
        // QRgb rgba = QColorDialog::getRgba( 0xFFFFFFFF, &c_ok, 0 );    // 0 == parent
        
        QColor temp2 = temp;
        m_map.addEntry(temp2, qstrtostr(newName));
        m_colourtable->populate_table(m_map, m_listmap);
        // Else we don't do anything as they either didn't give a name
        //  or didn't give a colour
    }

}

void
ColourConfigurationPage::slotDelete()
{
    //old: QTableWidgetSelection temp = m_colourtable->selection(0);
    QList<QTableWidgetItem *> temp = m_colourtable->selectedItems();
    
//    if ((!temp.isActive()) || (temp.topRow() == 0))    //&&& check re-implementation
//        return ;
    
    if( temp.isEmpty() ){
        // fail silently
        return;
    }

    unsigned int toDel = (*temp[0]).row();

    m_map.deleteEntry(m_listmap[toDel]);
    m_colourtable->populate_table(m_map, m_listmap);

}

}
