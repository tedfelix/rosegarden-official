// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <kstdaction.h>
#include <kaction.h>

#include <qvbox.h>
#include <qlayout.h>

#include "controleditor.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"
#include "rosestrings.h"
#include "studiocommands.h"

ControlEditorDialog::ControlEditorDialog(QWidget *parent,
                                         RosegardenGUIDoc *doc):
    KMainWindow(parent, "controleditordialog"),
    m_studio(&doc->getStudio()),
    m_doc(doc),
    m_modified(false)
{
    QVBox* mainFrame = new QVBox(this);
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage Control Parameters"));

    m_listView = new KListView(mainFrame);
    m_listView->addColumn(i18n("Controller name"));
    m_listView->addColumn(i18n("Controller value"));
    m_listView->addColumn(i18n("Max"));
    m_listView->addColumn(i18n("Min"));
    m_listView->addColumn(i18n("Default"));
    m_listView->addColumn(i18n("Colour"));


    QFrame* btnBox = new QFrame(mainFrame);

    btnBox->setSizePolicy(
            QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 4, 10);

    m_addButton = new QPushButton(i18n("Add"), btnBox);
    m_deleteButton = new QPushButton(i18n("Delete"), btnBox);

    m_closeButton = new QPushButton(i18n("Close"), btnBox);
    m_applyButton = new QPushButton(i18n("Apply"), btnBox);
    m_resetButton = new QPushButton(i18n("Reset"), btnBox);

    layout->addStretch(10);
    layout->addWidget(m_addButton);
    layout->addWidget(m_deleteButton);
    layout->addSpacing(30);

    layout->addWidget(m_applyButton);
    layout->addWidget(m_resetButton);
    layout->addSpacing(15);

    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_addButton, SIGNAL(released()),
            SLOT(slotAdd()));

    connect(m_deleteButton, SIGNAL(released()),
            SLOT(slotDelete()));

    connect(m_closeButton, SIGNAL(released()),
            SLOT(slotClose()));

    connect(m_applyButton, SIGNAL(released()),
            SLOT(slotApply()));

    connect(m_resetButton, SIGNAL(released()),
            SLOT(slotReset()));
    setupActions();

    m_listView->setAllColumnsShowFocus(true);

    initDialog();
}


ControlEditorDialog::~ControlEditorDialog()
{
}

void 
ControlEditorDialog::initDialog()
{
    Rosegarden::ControlListConstIterator it = m_studio->beginControllers();
    QListViewItem *item;

    m_listView->clear();

    for (; it != m_studio->endControllers(); ++it)
    {
        item = new QListViewItem(m_listView,
                                 strtoqstr((*it)->getName()),
                                 strtoqstr((*it)->getType()),
                                 strtoqstr((*it)->getDescription()),
                                 QString("%1").arg((*it)->getMin()),
                                 QString("%1").arg((*it)->getMax()),
                                 QString("%1").arg((*it)->getDefault()));

        m_listView->insertItem(item);
    }

}

void 
ControlEditorDialog::slotApply()
{
    RG_DEBUG << "ControlEditorDialog::slotApply" << endl;
}

void 
ControlEditorDialog::slotReset()
{
    RG_DEBUG << "ControlEditorDialog::slotReset" << endl;
}

void
ControlEditorDialog::slotUpdate()
{
    RG_DEBUG << "ControlEditorDialog::slotUpdate" << endl;
}

void
ControlEditorDialog::slotAdd()
{
    RG_DEBUG << "ControlEditorDialog::slotAdd" << endl;

    Rosegarden::ControlParameter *control = new Rosegarden::ControlParameter();

    AddControlParameterCommand *command =
        new AddControlParameterCommand(m_studio, control);

    addCommandToHistory(command);

    initDialog();
}

void
ControlEditorDialog::slotDelete()
{
    if (!m_listView->currentItem()) return;

    RG_DEBUG << "ControlEditorDialog::slotDelete" << endl;
}

void
ControlEditorDialog::slotClose()
{
    RG_DEBUG << "ControlEditorDialog::slotClose" << endl;

    m_doc->getCommandHistory()->detachView(actionCollection());
    m_doc = 0;

    close();
}

void
ControlEditorDialog::setupActions()
{
    KStdAction::copy     (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste    (this, SLOT(slotEditPaste()),      actionCollection());

    // some adjustments
    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStdAccel::key(KStdAccel::Undo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStdAccel::key(KStdAccel::Redo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Redo));

    createGUI("controleditor.rc");
}

void 
ControlEditorDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}

MultiViewCommandHistory* 
ControlEditorDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}


void
ControlEditorDialog::setModified(bool modified)
{
    RG_DEBUG << "ControlEditorDialog::setModified(" << modified << ")" << endl;

    if (modified)
    {
    }
    else
    {
    }

    m_modified = modified;
}

void
ControlEditorDialog::checkModified()
{
    RG_DEBUG << "ControlEditorDialog::checkModified(" << m_modified << ")" 
             << endl;

}

