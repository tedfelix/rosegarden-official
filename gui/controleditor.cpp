// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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


#include <qvbox.h>

#include "controleditor.h"
#include "rosegardenguidoc.h"

ControlEditorDialog::ControlEditorDialog(QWidget *parent,
                                         RosegardenGUIDoc *doc):
    KMainWindow(parent, "controleditordialog"),
    m_studio(&doc->getStudio()),
    m_doc(doc)
{
    QVBox* mainFrame = new QVBox(this);
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage Control Parameters"));

    initDialog();
}


ControlEditorDialog::~ControlEditorDialog()
{
}

void 
ControlEditorDialog::initDialog()
{
}

void 
ControlEditorDialog::slotApply()
{
}

void 
ControlEditorDialog::slotReset()
{
}

void
ControlEditorDialog::slotUpdate()
{
}

void
ControlEditorDialog::setupActions()
{
}

