/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[CommandRegistry]"
#define RG_NO_DEBUG_PRINT

#include "CommandRegistry.h"

#include "Command.h"

#include "misc/Strings.h"
#include "misc/Debug.h"

#include <QIcon>
#include <QPixmap>
#include <QFile>
#include <QAction>


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
        RG_WARNING << "slotInvokeCommand(): Unknown actionName" << actionName;
        return;
    }

    invokeCommand(actionName);
}

CommandArgumentQuerier::~CommandArgumentQuerier()
{
}


}
