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
#include <qtooltip.h>

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
    m_listView->addColumn(i18n("Controller name  "));
    m_listView->addColumn(i18n("Controller type  "));
    m_listView->addColumn(i18n("Controller value  "));
    m_listView->addColumn(i18n("Description  "));
    m_listView->addColumn(i18n("Max  "));
    m_listView->addColumn(i18n("Min  "));
    m_listView->addColumn(i18n("Default  "));
    m_listView->addColumn(i18n("Colour  "));

    // Align centrally
    for (int i = 0; i < 8; ++i)
        m_listView->setColumnAlignment(i, Qt::AlignHCenter);


    QFrame* btnBox = new QFrame(mainFrame);

    btnBox->setSizePolicy(
            QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 4, 10);

    m_addButton = new QPushButton(i18n("Add"), btnBox);
    m_deleteButton = new QPushButton(i18n("Delete"), btnBox);

    m_closeButton = new QPushButton(i18n("Close"), btnBox);

    m_copyButton = new QPushButton(i18n("Copy"), btnBox);
    m_pasteButton = new QPushButton(i18n("Paste"), btnBox);

    QToolTip::add(m_addButton,
                  i18n("Add a Control Parameter to the Studio"));

    QToolTip::add(m_deleteButton,
                  i18n("Delete a Control Parameter from the Studio"));

    QToolTip::add(m_closeButton,
                  i18n("Close the Control Parameter editor"));

    QToolTip::add(m_copyButton,
                  i18n("Copy a Control Parameter"));

    QToolTip::add(m_pasteButton,
                  i18n("Paste a Control Parameter"));

    layout->addStretch(10);
    layout->addWidget(m_addButton);
    layout->addWidget(m_deleteButton);
    layout->addSpacing(30);

    layout->addWidget(m_copyButton);
    layout->addWidget(m_pasteButton);
    layout->addSpacing(15);

    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_addButton, SIGNAL(released()),
            SLOT(slotAdd()));

    connect(m_deleteButton, SIGNAL(released()),
            SLOT(slotDelete()));

    connect(m_closeButton, SIGNAL(released()),
            SLOT(slotClose()));

    connect(m_copyButton, SIGNAL(released()),
            SLOT(slotEditCopy()));

    connect(m_pasteButton, SIGNAL(released()),
            SLOT(slotEditPaste()));
    setupActions();

    m_doc->getCommandHistory()->attachView(actionCollection());
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    connect(m_listView, SIGNAL(doubleClicked(QListViewItem *)),
            SLOT(slotEdit(QListViewItem *)));

    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
    m_listView->setSelectionMode(QListView::Extended);

    initDialog();

    setAutoSaveSettings(ControlEditorConfigGroup, true);
}


ControlEditorDialog::~ControlEditorDialog()
{
    RG_DEBUG << "ControlEditorDialog::~ControlEditorDialog" << endl;

    m_listView->saveLayout(kapp->config(), ControlEditorConfigGroup);

    if (m_doc)
        m_doc->getCommandHistory()->detachView(actionCollection());
}

void 
ControlEditorDialog::initDialog()
{
    RG_DEBUG << "ControlEditorDialog::initDialog" << endl;
    slotUpdate();
}

void
ControlEditorDialog::slotUpdate()
{
    RG_DEBUG << "ControlEditorDialog::slotUpdate" << endl;

    //QPtrList<QListViewItem> selection = m_listView->selectedItems();

    Rosegarden::ControlListConstIterator it = m_studio->beginControllers();
    QListViewItem *item;
    int i = 0;

    m_listView->clear();

    for (; it != m_studio->endControllers(); ++it)
    {
        item = new ControlParameterItem(i++,
                                        m_listView,
                                        strtoqstr((*it)->getName()),
                                        strtoqstr((*it)->getType()),
                                        QString("%1").arg(
                                            int((*it)->getControllerValue())),
                                        strtoqstr((*it)->getDescription()),
                                        QString("%1").arg((*it)->getMin()),
                                        QString("%1").arg((*it)->getMax()),
                                        QString("%1").arg((*it)->getDefault()),
                                        QString("%1").arg((*it)->getColour()));

        m_listView->insertItem(item);
    }
}

void 
ControlEditorDialog::slotEditCopy()
{
    RG_DEBUG << "ControlEditorDialog::slotEditCopy" << endl;
}

void
ControlEditorDialog::slotEditPaste()
{
    RG_DEBUG << "ControlEditorDialog::slotEditPaste" << endl;
}

void
ControlEditorDialog::slotAdd()
{
    RG_DEBUG << "ControlEditorDialog::slotAdd" << endl;

    Rosegarden::ControlParameter *control = new Rosegarden::ControlParameter();

    AddControlParameterCommand *command =
        new AddControlParameterCommand(m_studio, control);

    addCommandToHistory(command);

    slotUpdate();
}


void
ControlEditorDialog::slotDelete()
{
    RG_DEBUG << "ControlEditorDialog::slotDelete" << endl;

    if (!m_listView->currentItem()) return;

    int id = dynamic_cast<ControlParameterItem*>
        (m_listView->currentItem())->getId();

    RemoveControlParameterCommand *command =
        new RemoveControlParameterCommand(m_studio, id);

    addCommandToHistory(command);

    slotUpdate();
}

void
ControlEditorDialog::slotClose()
{
    RG_DEBUG << "ControlEditorDialog::slotClose" << endl;

    if (m_doc) m_doc->getCommandHistory()->detachView(actionCollection());
    m_doc = 0;

    close();
}

void
ControlEditorDialog::setupActions()
{
    KAction* close = KStdAction::close(this,
                                       SLOT(slotClose()),
                                       actionCollection());

    m_closeButton->setText(close->text());
    connect(m_closeButton, SIGNAL(released()), this, SLOT(slotClose()));

    KStdAction::copy   (this, SLOT(slotEditCopy()),     actionCollection());
    KStdAction::paste  (this, SLOT(slotEditPaste()),    actionCollection());

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


void
ControlEditorDialog::slotEdit()
{
}

void
ControlEditorDialog::slotEdit(QListViewItem *i)
{
    RG_DEBUG << "ControlEditorDialog::slotEdit" << endl;

    int id = dynamic_cast<ControlParameterItem*>(i)->getId();

    ControlParameterEditDialog *dialog = 
        new ControlParameterEditDialog::ControlParameterEditDialog(
                this, m_studio->getControlParameter(id));

    if (dialog->exec() == QDialog::Accepted)
    {
        cout << "OK" << endl;
    }

}


const char* const ControlEditorDialog::ControlEditorConfigGroup = "Control Editor";


// ----------------- ControlParameterEditDialog ---------------
//
//

ControlParameterEditDialog::ControlParameterEditDialog(
            QWidget *parent,
            Rosegarden::ControlParameter *control):
    KDialogBase(parent, 0, true,
                i18n("Edit Control Parameter"), Ok | Cancel),
    m_control(control)

{
}

