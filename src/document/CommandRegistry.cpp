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

#include <QIcon>
#include <QPixmap>
#include <QFile>
#include <QAction>

#include "document/Command.h"
#include <misc/Strings.h>
#include "gui/general/IconLoader.h"
#include <iostream>

namespace Rosegarden {

AbstractCommandBuilder::~AbstractCommandBuilder()
{
}

CommandRegistry::CommandRegistry()
{
}

CommandRegistry::~CommandRegistry()
{
    for (ActionBuilderMap::iterator i = m_builders.begin();
         i != m_builders.end(); ++i) {
        delete i->second;
    }
}

void
CommandRegistry::slotInvokeCommand()
{
    const QObject *s = sender();
    QString actionName = s->objectName();
    
    if (m_builders.find(actionName) == m_builders.end()) {
        std::cerr << "CommandRegistry::slotInvokeCommand: Unknown actionName \""
                  << qStrToStrLocal8(actionName) << "\"" << std::endl;
        return;
    }

    invokeCommand(actionName);
}

CommandArgumentQuerier::~CommandArgumentQuerier()
{
}

}


