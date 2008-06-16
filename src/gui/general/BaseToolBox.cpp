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

    connect(tool, SIGNAL(showContextHelp(const QString &)),
            this, SIGNAL(showContextHelp(const QString &)));
    
    return tool;
}

}
#include "BaseToolBox.moc"
