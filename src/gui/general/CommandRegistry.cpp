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

#include "CommandRegistry.h"
#include "EditView.h"

#include <qiconset.h>
#include <qpixmap.h>
#include <qfile.h>

#include <kglobal.h>

namespace Rosegarden {

//CommandRegistry::ViewRegistrarMap
//CommandRegistry::m_registrars;

CommandRegistry::CommandRegistry(EditView *view) :
    m_view(view)
{
    std::cerr << "CommandRegistry: view name \"" << m_view->name() << "\"" << std::endl;
//    RegistrarList &registrars = m_registrars[m_view->name()];
//    for (RegistrarList::iterator i = registrars.begin();
//         i != registrars.end(); ++i) {
//        std::cerr << "CommandRegistry: Registering command" << std::endl;
//        (*i)->registerCommand(this);
//    }
}

CommandRegistry::~CommandRegistry()
{
    for (ActionBuilderMap::iterator i = m_builders.begin();
         i != m_builders.end(); ++i) {
        delete i->second;
    }
}

//void
//CommandRegistry::addRegistrar(AbstractCommandRegistrar *registrar)
//{
//    std::cerr << "CommandRegistry::addRegistrar for view name \""
//              << registrar->getViewName() << "\"" << std::endl;
//    m_registrars[registrar->getViewName()].push_back(registrar);
//}

void
CommandRegistry::addAction(QString title,
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
CommandRegistry::slotInvokeCommand()
{
    const QObject *s = sender();
    QString actionName = s->name();
    
    if (m_builders.find(actionName) == m_builders.end()) {
        std::cerr << "CommandRegistry::slotInvokeCommand: Unknown actionName \""
                  << actionName << "\"" << std::endl;
        return;
    }

    EventSelection *selection = m_view->getCurrentSelection();
    if (!selection) {
        std::cerr << "CommandRegistry::slotInvokeCommand: No selection"
                  << std::endl;
    }

    m_view->addCommandToHistory(m_builders[actionName]->build(actionName,
                                                              *selection));
}

}

#include "CommandRegistry.moc"

