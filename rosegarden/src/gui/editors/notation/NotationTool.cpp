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


#include "NotationTool.h"
#include "misc/Debug.h"

#include "gui/general/EditTool.h"
#include "NotationView.h"
#include <qstring.h>


namespace Rosegarden
{

NotationTool::NotationTool(const QString& menuName, NotationView* view)
        : EditTool(menuName, view),
        m_nParentView(view)
{}

NotationTool::~NotationTool()
{
    NOTATION_DEBUG << "NotationTool::~NotationTool()" << endl;

    //     delete m_menu;
    //     m_parentView->factory()->removeClient(this);
    //     m_instance = 0;
}

void NotationTool::ready()
{
    m_nParentView->setCanvasCursor(Qt::arrowCursor);
    m_nParentView->setHeightTracking(false);
}

}
