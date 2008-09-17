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


#include "ColourConfigurationPage.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "commands/segment/SegmentColourMapCommand.h"
#include "ConfigurationPage.h"
#include "document/RosegardenGUIDoc.h"
#include "document/MultiViewCommandHistory.h"
#include "gui/general/GUIPalette.h"
#include "gui/widgets/ColourTable.h"
#include "TabbedConfigurationPage.h"

#include <QColorDialog>
#include <QSettings>
#include <QInputDialog>
#include <QColor>
#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>


namespace Rosegarden
{

ColourConfigurationPage::ColourConfigurationPage(RosegardenGUIDoc *doc,
        QWidget *parent,
        const char *name)
        : TabbedConfigurationPage(doc, parent, name)
{
    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 2, 2,
                                          10, 5);

    m_map = m_doc->getComposition().getSegmentColourMap();

    m_colourtable = new ColourTable(frame, m_map, m_listmap);
    m_colourtable->setFixedHeight(280);

    layout->addWidget(m_colourtable, 0, 0, 0- 0+1, 1- 1);

    QPushButton* addColourButton = new QPushButton(i18n("Add New Color"),
                                   frame);
    layout->addWidget(addColourButton, 1, 0, Qt::AlignHCenter);

    QPushButton* deleteColourButton = new QPushButton(i18n("Delete Color"),
                                      frame);
    layout->addWidget(deleteColourButton, 1, 1, Qt::AlignHCenter);

    connect(addColourButton, SIGNAL(clicked()),
            this, SLOT(slotAddNew()));

    connect(deleteColourButton, SIGNAL(clicked()),
            this, SLOT(slotDelete()));

    connect(this, SIGNAL(docColoursChanged()),
            m_doc, SLOT(slotDocColoursChanged()));

    connect(m_colourtable, SIGNAL(entryTextChanged(unsigned int, QString)),
            this, SLOT(slotTextChanged(unsigned int, QString)));

    connect(m_colourtable, SIGNAL(entryColourChanged(unsigned int, QColor)),
            this, SLOT(slotColourChanged(unsigned int, QColor)));

    addTab(frame, i18n("Color Map"));

}

void
ColourConfigurationPage::slotTextChanged(unsigned int index, QString string)
{
    m_map.modifyNameByIndex(m_listmap[index], string.ascii());
    m_colourtable->populate_table(m_map, m_listmap);
}

void
ColourConfigurationPage::slotColourChanged(unsigned int index, QColor color)
{
    m_map.modifyColourByIndex(m_listmap[index], GUIPalette::convertColour(color));
    m_colourtable->populate_table(m_map, m_listmap);
}

void
ColourConfigurationPage::apply()
{
    SegmentColourMapCommand *command = new SegmentColourMapCommand(m_doc, m_map);
    m_doc->getCommandHistory()->addCommand(command);

    RG_DEBUG << "ColourConfigurationPage::apply() emitting docColoursChanged()" << endl;
    emit docColoursChanged();
}

void
ColourConfigurationPage::slotAddNew()
{
    QColor temp;

    bool ok = false;

    QString newName = QInputDialog::getText(this, i18n("New Color Name"),
											i18n("Enter new name"), QLineEdit::Normal,
                                            i18n("New"),
                                            &ok, 0);
	
	bool c_ok;
	
    if ((ok == true) && (!newName.isEmpty())) {
        //QColorDialog box(this, "", true);

        //int result = box.getColor( temp );
		//QColor col = QColorDialog::getColor();
		QRgb rgba = QColorDialog::getRgba( 0xFFFFFFFF, &c_ok, 0 );	// 0 == parent
		
		if ( c_ok ) {
            Colour temp2 = GUIPalette::convertColour(temp);
            m_map.addItem(temp2, qstrtostr(newName));
            m_colourtable->populate_table(m_map, m_listmap);
        }
        // Else we don't do anything as they either didn't give a name
        //  or didn't give a colour
    }

}

void
ColourConfigurationPage::slotDelete()
{
    QTableWidgetSelection temp = m_colourtable->selection(0);

    if ((!temp.isActive()) || (temp.topRow() == 0))
        return ;

    unsigned int toDel = temp.topRow();

    m_map.deleteItemByIndex(m_listmap[toDel]);
    m_colourtable->populate_table(m_map, m_listmap);

}

}
#include "ColourConfigurationPage.moc"
