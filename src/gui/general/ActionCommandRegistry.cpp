/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ActionCommandRegistry]"

#include "ActionCommandRegistry.h"
#include "ActionFileClient.h"
#include "SelectionManager.h"

#include "document/CommandHistory.h"
#include "gui/widgets/InputDialog.h"
#include "gui/widgets/LineEdit.h"

#include <QMessageBox>
#include <QFile>
#include <QMenu>
#include <QWidget>

#include "misc/Strings.h"
#include "misc/Debug.h"

#include <QCoreApplication>

namespace Rosegarden
{

ActionCommandRegistry::ActionCommandRegistry(ActionFileClient *c) :
    m_client(c)
{
}

ActionCommandRegistry::~ActionCommandRegistry()
{
}


void
ActionCommandRegistry::addAction(QString actionName)
{
    m_client->createAction(actionName, this, SLOT(slotInvokeCommand()));
}    

class ActionCommandArgumentQuerier : public CommandArgumentQuerier
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ActionCommandArgumentQuerier)

public:
    ActionCommandArgumentQuerier(QWidget *widget) : m_widget(widget) { }
    QString getText(QString message, bool *ok) {
        if (!m_widget) return "";
        return InputDialog::getText(m_widget,
                                    tr("Rosegarden - Query"),
                                    message, LineEdit::Normal, "", ok);
    }

protected:
    QWidget *m_widget;
};

void
ActionCommandRegistry::invokeCommand(QString actionName)
{
    EventSelection *selection = 0;

    SelectionManager *sm = dynamic_cast<SelectionManager *>(m_client);

    if (sm) {
        selection = sm->getSelection();
    } else {
        RG_WARNING << "ActionCommandRegistry::slotInvokeCommand: Action file client is not a SelectionManager";
    }

    if (!selection) {
        RG_WARNING << "ActionCommandRegistry::slotInvokeCommand: No selection";
        return;
    }

    QWidget *widget = dynamic_cast<QWidget *>(m_client);
    if (!widget) {
        RG_WARNING << "ActionCommandRegistry::slotInvokeCommand: Action file client is not a widget";
    }

    try {

        ActionCommandArgumentQuerier querier(widget);

        Command *command = m_builders[actionName]->build
            (actionName, *selection, querier);

        CommandHistory::getInstance()->addCommand(command);

        EventSelection *subsequentSelection = 
            m_builders[actionName]->getSubsequentSelection(command);

        if (subsequentSelection && sm) {
            sm->setSelection(subsequentSelection, false);
        }

    } catch (CommandCancelled) {
    } catch (CommandFailed f) {

        QMessageBox::warning(widget,
                             tr("Rosegarden - Warning"),
                             strtoqstr(f.getMessage()),
                             QMessageBox::Ok);
    }
}

}

