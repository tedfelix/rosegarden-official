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

#include "gui/general/EditView.h"
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


//!!! hoist these back up a level to a new intermediate class? EditViewCommandRegistry
void
EditViewCommandRegistry::addAction(QString title,
                                   QString iconName,
                                   const KShortcut &shortcut, 
                                   QString actionName)
{
    bool haveIcon = (iconName != "");
    QIconSet icon;

    std::cerr << "Adding action: " << title << ", " << iconName << ", " << shortcut << ", " << actionName << std::endl;

    if (haveIcon) {
        QString pixmapDir =
            KGlobal::dirs()->findResource("appdata", "pixmaps/");
        QString fileBase = pixmapDir + "/toolbar/";
        fileBase += iconName;
        if (QFile(fileBase + ".png").exists()) {
            icon = QIconSet(QPixmap(fileBase + ".png"));
        } else if (QFile(fileBase + ".xpm").exists()) {
            icon = QIconSet(QPixmap(fileBase + ".xpm"));
        } else {
            haveIcon = false;
        }
    }

    if (haveIcon) {
        new KAction(title,
                    icon,
                    shortcut,
                    this,
                    SLOT(slotInvokeCommand()),
                    m_view->actionCollection(),
                    actionName);
    } else {
        new KAction(title,
                    shortcut,
                    this,
                    SLOT(slotInvokeCommand()),
                    m_view->actionCollection(),
                    actionName);
    }
}    

void
EditViewCommandRegistry::invokeCommand(QString actionName)
{
    EventSelection *selection = m_view->getCurrentSelection();
    if (!selection) {
        std::cerr << "EditViewCommandRegistry::slotInvokeCommand: No selection"
                  << std::endl;
    }

    try {

        KCommand *command = m_builders[actionName]->build
            (actionName, *selection, m_view);

        m_view->addCommandToHistory(command);

        EventSelection *subsequentSelection = 
            m_builders[actionName]->getSubsequentSelection(command);

        if (subsequentSelection) {
            m_view->setCurrentSelection(subsequentSelection);
        }

    } catch (CommandCancelled) {
    } catch (CommandFailed f) {
        KMessageBox::sorry(m_view, strtoqstr(f.getMessage()));
    }
}

}

