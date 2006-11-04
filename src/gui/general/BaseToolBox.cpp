/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "BaseToolBox.h"

#include "BaseTool.h"
#include <qobject.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

BaseToolBox::BaseToolBox(QWidget* parent)
        : QObject(parent),
        m_tools(17,  // default size, from the Qt docs
                false) // but we want it to be case insensitive
{
    //m_tools.setAutoDelete(true);
}

BaseTool* BaseToolBox::getTool(const QString& toolName)
{
    BaseTool* tool = m_tools[toolName];

    if (!tool) tool = createTool(toolName);
    
    return tool;
}

}
#include "BaseToolBox.moc"
