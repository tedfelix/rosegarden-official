// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>

#include "editview.h"
#include "edittool.h"
#include "rosegardenguidoc.h"
#include "ktmpstatusmsg.h"

//----------------------------------------------------------------------
const unsigned int EditView::ID_STATUS_MSG = 1;

EditView::EditView(RosegardenGUIDoc *doc,
                   std::vector<Rosegarden::Segment *> segments,
                   QWidget *parent)
    : KMainWindow(parent),
      m_config(kapp->config()),
      m_document(doc),
      m_tool(0),
      m_toolBox(0)

{
}

EditView::~EditView()
{
}

void EditView::readjustViewSize(QSize requestedSize)
{
    QSize currentSize = getViewSize();
    
    int requestedWidth = requestedSize.width(),
        requestedHeight = requestedSize.height(),
        currentWidth = currentSize.width(),
        currentHeight = currentSize.height();

    QSize newSize = requestedSize;

    if ((requestedWidth < currentWidth) &&
        ((requestedWidth / currentWidth) < 0.75))

        newSize.setWidth(requestedWidth);

    else // requestedWidth >= currentWidth
        newSize.setWidth(requestedWidth + requestedWidth / 2);

    if ((requestedHeight < currentHeight) &&
        ((requestedHeight / currentHeight) < 0.75))

        newSize.setHeight(requestedHeight);

    else // requestedHeight >= currentHeight
        newSize.setHeight(requestedHeight + requestedHeight / 2);

    setViewSize(newSize);
}

void EditView::setTool(EditTool* tool)
{
    if (m_tool)
        m_tool->stow();

    m_tool = tool;

    if (m_tool)
        m_tool->ready();

}

//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

void EditView::closeWindow()
{
    close();
}

//
// Toolbar and statusbar toggling
//
void EditView::slotToggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), statusBar());

    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();
}

void EditView::slotToggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), statusBar());

    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();
}

//
// Status messages
//
void EditView::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void EditView::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}
