/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#include "EditViewCommandRegistry.h"

#include <klocale.h>
#include <QMessageBox>
#include <QInputDialog>
#include <QFile>
#include <QMenu>

#include "gui/general/EditView.h"
#include "gui/general/IconLoader.h"
#include "misc/Strings.h"

namespace Rosegarden
{

EditViewCommandRegistry::EditViewCommandRegistry(EditView *v) :
    m_view(v)
{
}

EditViewCommandRegistry::~EditViewCommandRegistry()
{
}


void
EditViewCommandRegistry::addAction(QString title,
                                   QString iconName,
                                   QString shortcut, 
                                   QString actionName,
                                   QString menuTitle,
                                   QString menuName)
{
//!!! Trim this drastically (to just actionName and menuName) if the dynamic
//!!! gui stuff pans out OK

    bool haveIcon = (iconName != "");
    QIcon icon;

    std::cerr << "Adding action: " << title << ", " << ((iconName != "") ? iconName : "(no icon)") << ", " << ((shortcut != "") ? shortcut : "(no shortcut)") << ", " << actionName << std::endl;

    if (haveIcon) {
        IconLoader il;
        icon = il.load(iconName);
        haveIcon = !icon.isNull();
    }

    QMenu *menu = 0;

    if (menuName != "") {

        menu = m_view->findChild<QMenu *>(menuName);

        if (!menu) {
            menu = new QMenu(menuTitle, m_view);
            menu->setObjectName(menuName);
            //!!! do we need to do something further with the menu here?
            // do any commands actually use this menu name field?
        }
    }

    QAction *action = 0;

    action = new QAction(m_view);
    action->setObjectName(actionName);
    action->setText(title);

    if (shortcut != "") {
        action->setShortcut(shortcut);
    }

    if (haveIcon) {
        action->setIcon(icon);
    }

    connect(action, SIGNAL(triggered()), this, SLOT(slotInvokeCommand()));

    if (menu) menu->addAction(action);
}    

class EditViewCommandArgumentQuerier : public CommandArgumentQuerier
{
public:
    EditViewCommandArgumentQuerier(EditView *view) : m_view(view) { }
    QString getText(QString message, bool *ok) {
        return QInputDialog::getText(m_view,
                                     i18n("Rosegarden - Query"),
                                     message, QLineEdit::Normal, "", ok);
    }

protected:
    EditView *m_view;
};

void
EditViewCommandRegistry::invokeCommand(QString actionName)
{
    EventSelection *selection = m_view->getCurrentSelection();
    if (!selection) {
        std::cerr << "EditViewCommandRegistry::slotInvokeCommand: No selection"
                  << std::endl;
    }

    try {

        EditViewCommandArgumentQuerier querier(m_view);

        Command *command = m_builders[actionName]->build
            (actionName, *selection, querier);

        m_view->addCommandToHistory(command);

        EventSelection *subsequentSelection = 
            m_builders[actionName]->getSubsequentSelection(command);

        if (subsequentSelection) {
            m_view->setCurrentSelection(subsequentSelection);
        }

    } catch (CommandCancelled) {
    } catch (CommandFailed f) {

        QMessageBox::warning(m_view,
                             i18n("Rosegarden - Warning"),
                             strtoqstr(f.getMessage()),
                             QMessageBox::Ok);
    }
}

}

