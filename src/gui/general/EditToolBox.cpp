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

#include "klocale.h"
#include "EditToolBox.h"

#include "BaseToolBox.h"
#include "EditTool.h"
#include "EditView.h"
#include <QObject>
#include <QString>
#include <QMessageBox>

namespace Rosegarden
{

EditToolBox::EditToolBox(EditView *parent)
        : BaseToolBox(parent),
        m_parentView(parent)
{
}

EditTool* EditToolBox::getTool(const QString& toolName)
{
    return dynamic_cast<EditTool*>(BaseToolBox::getTool(toolName));
}

EditTool* EditToolBox::createTool(const QString&)
{
    QMessageBox::critical(0, "", i18n("EditToolBox::createTool called - this should never happen"));
    return 0;
}

}
#include "EditToolBox.moc"
